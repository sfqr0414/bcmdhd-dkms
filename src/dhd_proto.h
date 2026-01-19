/*
 * Header file describing the internal (inter-module) DHD interfaces.
 *
 * Provides type definitions and function prototypes used to link the
 * DHD OS, bus, and protocol modules.
 *
 * Copyright (C) 2022, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id$
 */

#ifndef _dhd_proto_h_
#define _dhd_proto_h_

#include <dhdioctl.h>
#include <wlioctl.h>
#ifdef BCMPCIE
#include <dhd_flowring.h>
#endif

/* Bring in types used by protocol structure */
#include <dhd.h>
#include <bcmmsgbuf.h>
#include <dhd_bus.h>
#include <dhd_flowring.h>
#include <bcmcdc.h>

/* Ring name length used by msgbuf_ring_t */
#ifndef RING_NAME_MAX_LENGTH
#define RING_NAME_MAX_LENGTH	24
#endif

/* Fallback SDIO alignment and margins (if not defined elsewhere) */
#ifndef DHD_SDALIGN
#define DHD_SDALIGN 32
#endif
#ifndef BUS_HEADER_LEN
#define BUS_HEADER_LEN (24 + DHD_SDALIGN)
#endif
#ifndef ROUND_UP_MARGIN
#define ROUND_UP_MARGIN 2048
#endif


/* DHD DMA loopback structure */
typedef struct dhd_dmaxfer {
	dhd_dma_buf_t srcmem;
	dhd_dma_buf_t dstmem;
	uint32        srcdelay;
	uint32        destdelay;
	uint32        len;
	bool          in_progress;
	uint64        start_usec;
	uint64        time_taken;
	uint32        d11_lpbk;
	int           status;
} dhd_dmaxfer_t;



/* msgbuf_ring : This object manages the host side ring that includes a DMA-able
 * buffer, the WR and RD indices, ring parameters such as max number of items
 * an length of each items, and other miscellaneous runtime state.
 * A msgbuf_ring may be used to represent a H2D or D2H common ring or a
 * H2D TxPost ring as specified in the PCIE FullDongle Spec.
 * Ring parameters are conveyed to the dongle, which maintains its own peer end
 * ring state. Depending on whether the DMA Indices feature is supported, the
 * host will update the WR/RD index in the DMA indices array in host memory or
 * directly in dongle memory.
 */
typedef struct msgbuf_ring {
	bool           inited;
	uint16         idx;       /* ring id */
	uint16         rd;        /* read index */
	uint16         curr_rd;   /* read index for debug */
	uint16         wr;        /* write index */
	uint16         max_items; /* maximum number of items in ring */
	uint16         item_len;  /* length of each item in the ring */
	sh_addr_t      base_addr; /* LITTLE ENDIAN formatted: base address */
	dhd_dma_buf_t  dma_buf;   /* DMA-able buffer: pa, va, len, dmah, secdma */
	uint32         seqnum;    /* next expected item's sequence number */
/* Our ring write batching helpers */
	void           *start_addr;
	/* # of messages on ring not yet announced to dongle */
	uint16         pend_items_count;
#ifdef AGG_H2D_DB
	osl_atomic_t	inflight;
#endif /* AGG_H2D_DB */

	uint8           ring_type;
	uint8           n_completion_ids;
	bool            create_pending;
	uint16          create_req_id;
	uint8           current_phase;
	uint16	        compeltion_ring_ids[MAX_COMPLETION_RING_IDS_ASSOCIATED];
	uchar		name[RING_NAME_MAX_LENGTH];
	uint32		ring_mem_allocated;
	void	        *ring_lock;
	struct msgbuf_ring *linked_ring; /* Ring Associated to metadata ring */
} msgbuf_ring_t;

/* Callback used to sync on D2H DMA completion (seqnum or xorcsum) */
typedef uint8 (* d2h_sync_cb_t)(dhd_pub_t *dhd, msgbuf_ring_t *ring,
	volatile cmn_msg_hdr_t *cmn_hdr, int item_len);

/* DHD protocol handle. Is an opaque type to other DHD software layers. */
typedef struct dhd_prot {
	osl_t *osh;		/* OSL handle */
	/* CDC/BDC protocol fields */
	uint16 reqid;
	uint8 pending;
	uint32 lastcmd;
	uint8 bus_header[BUS_HEADER_LEN];
	cdc_ioctl_t msg;
	unsigned char buf[WLC_IOCTL_MAXLEN + ROUND_UP_MARGIN];
	uint16 rxbufpost_sz; 		/* Size of rx buffer posted to dongle */
	uint16 rxbufpost_alloc_sz; 	/* Actual rx buffer packet allocated in the host */
	uint16 rxbufpost;
	uint16 rx_buf_burst;
	uint16 rx_bufpost_threshold;
	uint16 max_rxbufpost;
	uint32 tot_rxbufpost;
	uint32 tot_rxcpl;
	uint16 max_eventbufpost;
	uint16 max_ioctlrespbufpost;
	uint16 max_tsbufpost;
	uint16 max_infobufpost;
	uint16 infobufpost;
	uint16 cur_event_bufs_posted;
	uint16 cur_ioctlresp_bufs_posted;
	uint16 cur_ts_bufs_posted;

	/* Flow control mechanism based on active transmits pending */
	osl_atomic_t active_tx_count; /* increments/decrements on every packet tx/tx_status */
	uint16 h2d_max_txpost;
	uint16 h2d_htput_max_txpost;
	uint16 txp_threshold;  /* optimization to write "n" tx items at a time to ring */

	/* MsgBuf Ring info: has a dhd_dma_buf that is dynamically allocated */
	msgbuf_ring_t h2dring_ctrl_subn; /* H2D ctrl message submission ring */
	msgbuf_ring_t h2dring_rxp_subn; /* H2D RxBuf post ring */
	msgbuf_ring_t d2hring_ctrl_cpln; /* D2H ctrl completion ring */
	msgbuf_ring_t d2hring_tx_cpln; /* D2H Tx complete message ring */
	msgbuf_ring_t d2hring_rx_cpln; /* D2H Rx complete message ring */
	msgbuf_ring_t *h2dring_info_subn; /* H2D info submission ring */
	msgbuf_ring_t *d2hring_info_cpln; /* D2H info completion ring */
	msgbuf_ring_t *d2hring_edl; /* D2H Enhanced Debug Lane (EDL) ring */

	msgbuf_ring_t *h2d_flowrings_pool; /* Pool of preallocated flowings */
	dhd_dma_buf_t flowrings_dma_buf; /* Contiguous DMA buffer for flowrings */
	uint16        h2d_rings_total; /* total H2D (common rings + flowrings) */

	uint32		rx_dataoffset;

#ifdef BCMPCIE
	/* Mailbox/doorbell hooks (PCIE only) */
	dhd_mb_ring_t	mb_ring_fn;	/* called when dongle needs to be notified of new msg */
	dhd_mb_ring_2_t	mb_2_ring_fn;	/* called when dongle needs to be notified of new msg */
#endif /* BCMPCIE */

	/* ioctl related resources */
	uint8 ioctl_state;
	int16 ioctl_status; 		/* status returned from dongle */
	uint16 ioctl_resplen;
	dhd_ioctl_recieved_status_t ioctl_received;
	uint curr_ioctl_cmd;
	dhd_dma_buf_t	retbuf; 	/* For holding ioctl response */
	dhd_dma_buf_t	ioctbuf; 	/* For holding ioctl request */

	dhd_dma_buf_t	d2h_dma_scratch_buf; 	/* For holding d2h scratch */

	/* DMA-able arrays for holding WR and RD indices */
	uint32          rw_index_sz; /* Size of a RD or WR index in dongle */
	dhd_dma_buf_t   h2d_dma_indx_wr_buf; 	/* Array of H2D WR indices */
	dhd_dma_buf_t	h2d_dma_indx_rd_buf; 	/* Array of H2D RD indices */
	dhd_dma_buf_t	d2h_dma_indx_wr_buf; 	/* Array of D2H WR indices */
	dhd_dma_buf_t	d2h_dma_indx_rd_buf; 	/* Array of D2H RD indices */
	dhd_dma_buf_t h2d_ifrm_indx_wr_buf; 	/* Array of H2D WR indices for ifrm */

	dhd_dma_buf_t	host_bus_throughput_buf; /* bus throughput measure buffer */

	dhd_dma_buf_t   *flowring_buf;    /* pool of flow ring buf */
#ifdef DHD_DMA_INDICES_SEQNUM
	char *h2d_dma_indx_rd_copy_buf; /* Local copy of H2D WR indices array */
	char *d2h_dma_indx_wr_copy_buf; /* Local copy of D2H WR indices array */
	uint32 h2d_dma_indx_rd_copy_bufsz; /* H2D WR indices array size */
	uint32 d2h_dma_indx_wr_copy_bufsz; /* D2H WR indices array size */
	uint32 host_seqnum; 	/* Seqence number for D2H DMA Indices sync */
#endif /* DHD_DMA_INDICES_SEQNUM */
	uint32			flowring_num;

	d2h_sync_cb_t d2h_sync_cb; /* Sync on D2H DMA done: SEQNUM or XORCSUM */
#ifdef EWP_EDL
	d2h_edl_sync_cb_t d2h_edl_sync_cb; /* Sync on EDL D2H DMA done: SEQNUM or XORCSUM */
#endif /* EWP_EDL */
	ulong d2h_sync_wait_max; /* max number of wait loops to receive one msg */
	ulong d2h_sync_wait_tot; /* total wait loops */

	dhd_dmaxfer_t	dmaxfer; /* for test/DMA loopback */

	uint16		ioctl_seq_no;
	uint16		data_seq_no;  /* XXX this field is obsolete */
	uint16		ioctl_trans_id;
	void		*pktid_ctrl_map; /* a pktid maps to a packet and its metadata */
	void		*pktid_rx_map; 	/* pktid map for rx path */
	void		*pktid_tx_map; 	/* pktid map for tx path */
	bool		metadata_dbg;
	void		*pktid_map_handle_ioctl;
#ifdef DHD_MAP_PKTID_LOGGING
	void		*pktid_dma_map; 	/* pktid map for DMA MAP */
	void		*pktid_dma_unmap; /* pktid map for DMA UNMAP */
#endif /* DHD_MAP_PKTID_LOGGING */
	uint32		pktid_depleted_cnt; 	/* pktid depleted count */
	/* netif tx queue stop count */
	uint8		pktid_txq_stop_cnt;
	/* netif tx queue start count */
	uint8		pktid_txq_start_cnt;
	uint64		ioctl_fillup_time; 	/* timestamp for ioctl fillup */
	uint64		ioctl_ack_time; 		/* timestamp for ioctl ack */
	uint64		ioctl_cmplt_time; 	/* timestamp for ioctl completion */

	/* Applications/utilities can read tx and rx metadata using IOVARs */
	uint16		rx_metadata_offset;
	uint16		tx_metadata_offset;

#if (defined(BCM_ROUTER_DHD) && defined(HNDCTF))
	rxchain_info_t	rxchain; 	/* chain of rx packets */
#endif

#if defined(DHD_D2H_SOFT_DOORBELL_SUPPORT)
	/* Host's soft doorbell configuration */
	bcmpcie_soft_doorbell_t soft_doorbell[BCMPCIE_D2H_COMMON_MSGRINGS];
#endif /* DHD_D2H_SOFT_DOORBELL_SUPPORT */

	/* Work Queues to be used by the producer and the consumer, and threshold
	 * when the WRITE index must be synced to consumer's workq
	 */
	dhd_dma_buf_t	fw_trap_buf; /* firmware trap buffer */
#ifdef FLOW_RING_PREALLOC
	/* pre-allocation htput ring buffer */
	dhd_dma_buf_t	htput_ring_buf[HTPUT_TOTAL_FLOW_RINGS];
	/* pre-allocation folw ring(non htput rings) */
	dhd_dma_buf_t	flow_ring_buf[MAX_FLOW_RINGS];
#endif /* FLOW_RING_PREALLOC */
	uint32  host_ipc_version; /* Host sypported IPC rev */
	uint32  device_ipc_version; /* FW supported IPC rev */
	uint32  active_ipc_version; /* Host advertised IPC rev */
	dhd_dma_buf_t   hostts_req_buf; /* For holding host timestamp request buf */
	bool    hostts_req_buf_inuse;
	bool    rx_ts_log_enabled;
	bool    tx_ts_log_enabled;

#ifdef DHD_HMAPTEST
	uint32 hmaptest_rx_active;
	uint32 hmaptest_rx_pktid;
	char *hmap_rx_buf_va;
	dmaaddr_t hmap_rx_buf_pa;
	uint32 hmap_rx_buf_len;

	uint32 hmaptest_tx_active;
	uint32 hmaptest_tx_pktid;
	char *hmap_tx_buf_va;
	dmaaddr_t hmap_tx_buf_pa;
	uint32	  hmap_tx_buf_len;
	dhd_hmaptest_t	hmaptest; /* for hmaptest */
	bool hmap_enabled; /* TRUE = hmap is enabled */
#endif /* DHD_HMAPTEST */
	bool no_retry;
	bool no_aggr;
	bool fixed_rate;
	dhd_dma_buf_t	host_scb_buf; /* scb host offload buffer */
	bool no_tx_resource;
	uint32 txcpl_db_cnt;
#ifdef AGG_H2D_DB
	agg_h2d_db_info_t agg_h2d_db_info;
#endif /* AGG_H2D_DB */
	uint64 tx_h2d_db_cnt;
uint8 ctrl_cpl_snapshot[D2HRING_CTRL_CMPLT_ITEMSIZE];
	uint32 event_wakeup_pkt; /* Number of event wakeup packet rcvd */
	uint32 rx_wakeup_pkt;    /* Number of Rx wakeup packet rcvd */
	uint32 info_wakeup_pkt;  /* Number of info cpl wakeup packet rcvd */
	msgbuf_ring_t *d2hring_md_cpl; /* D2H metadata completion ring */
	/* no. which controls how many rx cpl/post items are processed per dpc */
	uint32 rx_cpl_post_bound;
	/*
	 * no. which controls how many tx post items are processed per dpc,
	 * i.e, how many tx pkts are posted to flowring from the bkp queue
	 * from dpc context
	 */
	uint32 tx_post_bound;
	/* no. which controls how many tx cpl items are processed per dpc */
	uint32 tx_cpl_bound;
	/* no. which controls how many ctrl cpl/post items are processed per dpc */
	uint32 ctrl_cpl_post_bound;
} dhd_prot_t;

/* Canonical protocol prototypes moved here to reduce -Wmissing-prototypes warnings
 * and serve as the single authoritative declarations during the port.
 */
extern void dhd_set_host_cap(dhd_pub_t *dhd);
extern void dhd_prot_clearcounts(dhd_pub_t *dhd);
extern void dhd_prot_update_rings_size(dhd_prot_t *prot);
extern int dhd_prepare_schedule_dmaxfer_free(dhd_pub_t *dhdp);
extern void dhd_prot_clean_flow_ring(dhd_pub_t *dhd, void *msgbuf_flow_info);
extern void dhd_msgbuf_delay_post_ts_bufs(dhd_pub_t *dhd);

/* RX emerge helpers (canonicalized) */
extern void dhd_rx_emerge_enqueue(dhd_pub_t *dhdp, void *pkt);
extern void *dhd_rx_emerge_dequeue(dhd_pub_t *dhdp);

/* SDIO / bus helper prototypes (canonicalized) */
extern bool dhdsdio_is_dataok(struct dhd_bus *bus);
extern uint8 dhdsdio_get_databufcnt(struct dhd_bus *bus);
/* Canonical SDIO helpers */
extern void dhdsdio_isr(void *arg);
extern int dhdsdio_downloadvars(struct dhd_bus *bus, void *arg, int len);
extern uint8 dhdsdio_devcap_get(struct dhd_bus *bus);
extern int dhd_sr_config(dhd_pub_t *dhd, bool on);
extern void dhd_enable_oob_intr(struct dhd_bus *bus, bool enable);
extern void dhd_bus_check_srmemsize(dhd_pub_t *dhdp);

#define DEFAULT_IOCTL_RESP_TIMEOUT	(5 * 1000) /* 5 seconds */
#ifndef IOCTL_RESP_TIMEOUT
#if defined(BCMQT_HW)
#define IOCTL_RESP_TIMEOUT  (600 * 1000) /* 600 sec in real time */
#elif defined(BCMFPGA_HW)
#define IOCTL_RESP_TIMEOUT  (60 * 1000) /* 60 sec in real time */
#else
/* In milli second default value for Production FW */
#define IOCTL_RESP_TIMEOUT  DEFAULT_IOCTL_RESP_TIMEOUT
#endif /* BCMQT */
#endif /* IOCTL_RESP_TIMEOUT */

#if defined(BCMQT_HW)
#define IOCTL_DMAXFER_TIMEOUT  (260 * 1000) /* 260 seconds second */
#elif defined(BCMFPGA_HW)
#define IOCTL_DMAXFER_TIMEOUT  (120 * 1000) /* 120 seconds */
#else
/* In milli second default value for Production FW */
#define IOCTL_DMAXFER_TIMEOUT  (15 * 1000) /* 15 seconds for Production FW */
#endif /* BCMQT */

#ifndef MFG_IOCTL_RESP_TIMEOUT
#define MFG_IOCTL_RESP_TIMEOUT  20000  /* In milli second default value for MFG FW */
#endif /* MFG_IOCTL_RESP_TIMEOUT */

#define DEFAULT_D3_ACK_RESP_TIMEOUT	2000
#ifndef D3_ACK_RESP_TIMEOUT
#define D3_ACK_RESP_TIMEOUT		DEFAULT_D3_ACK_RESP_TIMEOUT
#endif /* D3_ACK_RESP_TIMEOUT */

#define DEFAULT_DHD_BUS_BUSY_TIMEOUT	(IOCTL_RESP_TIMEOUT + 1000)
#ifndef DHD_BUS_BUSY_TIMEOUT
#define DHD_BUS_BUSY_TIMEOUT	DEFAULT_DHD_BUS_BUSY_TIMEOUT
#endif /* DEFAULT_DHD_BUS_BUSY_TIMEOUT */

#define DS_EXIT_TIMEOUT	1000 /* In ms */
#define DS_ENTER_TIMEOUT 1000 /* In ms */

#define IOCTL_DISABLE_TIMEOUT 0

/*
 * Exported from the dhd protocol module (dhd_cdc, dhd_rndis)
 */

/* Linkage, sets prot link and updates hdrlen in pub */
extern int dhd_prot_attach(dhd_pub_t *dhdp);

/* Initilizes the index block for dma'ing indices */
extern int dhd_prot_dma_indx_init(dhd_pub_t *dhdp, uint32 rw_index_sz,
	uint8 type, uint32 length);
#ifdef DHD_DMA_INDICES_SEQNUM
extern int dhd_prot_dma_indx_copybuf_init(dhd_pub_t *dhd, uint32 buf_sz,
	uint8 type);
extern uint32 dhd_prot_read_seqnum(dhd_pub_t *dhd, bool host);
extern void dhd_prot_write_host_seqnum(dhd_pub_t *dhd, uint32 seq_num);
extern void dhd_prot_save_dmaidx(dhd_pub_t *dhd);
#endif /* DHD_DMA_INDICES_SEQNUM */
/* Unlink, frees allocated protocol memory (including dhd_prot) */
extern void dhd_prot_detach(dhd_pub_t *dhdp);

/* Initialize protocol: sync w/dongle state.
 * Sets dongle media info (iswl, drv_version, mac address).
 */
extern int dhd_sync_with_dongle(dhd_pub_t *dhdp);

/* Protocol initialization needed for IOCTL/IOVAR path */
extern int dhd_prot_init(dhd_pub_t *dhd);

/* Stop protocol: sync w/dongle state. */
extern void dhd_prot_stop(dhd_pub_t *dhdp);

/* Add any protocol-specific data header.
 * Caller must reserve prot_hdrlen prepend space.
 */
extern void dhd_prot_hdrpush(dhd_pub_t *, int ifidx, void *txp);
extern uint dhd_prot_hdrlen(dhd_pub_t *, void *txp);

/* Remove any protocol-specific data header. */
extern int dhd_prot_hdrpull(dhd_pub_t *, int *ifidx, void *rxp, uchar *buf, uint *len);

#ifdef DHD_LOSSLESS_ROAMING
extern int dhd_update_sdio_data_prio_map(dhd_pub_t *dhdp);
#endif // DHD_LOSSLESS_ROAMING

/* Use protocol to issue ioctl to dongle */
extern int dhd_prot_ioctl(dhd_pub_t *dhd, int ifidx, wl_ioctl_t * ioc, void * buf, int len);

/* Handles a protocol control response asynchronously */
extern int dhd_prot_ctl_complete(dhd_pub_t *dhd);

/* Check for and handle local prot-specific iovar commands */
extern int dhd_prot_iovar_op(dhd_pub_t *dhdp, const char *name,
                             void *params, int plen, void *arg, int len, bool set);

/* Add prot dump output to a buffer */
extern void dhd_prot_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf);
extern void dhd_prot_counters(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf,
	bool print_ringinfo, bool print_pktidinfo);

/* Dump extended trap data */
extern int dhd_prot_dump_extended_trap(dhd_pub_t *dhdp, struct bcmstrbuf *b, bool raw);

/* Update local copy of dongle statistics */
extern void dhd_prot_dstats(dhd_pub_t *dhdp);

extern int dhd_ioctl(dhd_pub_t * dhd_pub, dhd_ioctl_t *ioc, void * buf, uint buflen);

extern int dhd_preinit_ioctls(dhd_pub_t *dhd);

extern int dhd_process_pkt_reorder_info(dhd_pub_t *dhd, uchar *reorder_info_buf,
	uint reorder_info_len, void **pkt, uint32 *free_buf_count);

#ifdef BCMPCIE
extern bool dhd_prot_process_msgbuf_txcpl(dhd_pub_t *dhd, int ringtype, uint32 *txcpl_items);
extern bool dhd_prot_process_msgbuf_rxcpl(dhd_pub_t *dhd, int ringtype,	uint32 *rxcpl_items);
extern bool dhd_prot_process_msgbuf_infocpl(dhd_pub_t *dhd, uint bound,
	uint32 *evtlog_items);
uint32 dhd_prot_get_tx_post_bound(dhd_pub_t *dhd);
uint32 dhd_prot_get_ctrl_cpl_post_bound(dhd_pub_t *dhd);
uint32 dhd_prot_get_tx_cpl_bound(dhd_pub_t *dhd);
uint32 dhd_prot_get_rx_cpl_post_bound(dhd_pub_t *dhd);
void dhd_prot_set_tx_cpl_bound(dhd_pub_t *dhd, uint32 val);
void dhd_prot_set_rx_cpl_post_bound(dhd_pub_t *dhd, uint32 val);
void dhd_prot_set_tx_post_bound(dhd_pub_t *dhd, uint32 val);
void dhd_prot_set_ctrl_cpl_post_bound(dhd_pub_t *dhd, uint32 val);
extern bool dhd_prot_process_ctrlbuf(dhd_pub_t * dhd, uint32 *ctrlcpl_items);
extern int dhd_prot_process_trapbuf(dhd_pub_t * dhd);
extern bool dhd_prot_dtohsplit(dhd_pub_t * dhd);
extern int dhd_post_dummy_msg(dhd_pub_t *dhd);
extern int dhdmsgbuf_lpbk_req(dhd_pub_t *dhd, uint len);
extern void dhd_prot_rx_dataoffset(dhd_pub_t *dhd, uint32 offset);
extern int dhd_prot_txdata(dhd_pub_t *dhd, void *p, uint8 ifidx);
extern void dhd_prot_schedule_aggregate_h2d_db(dhd_pub_t *dhd, uint16 flow_id);
extern int dhdmsgbuf_dmaxfer_req(dhd_pub_t *dhd,
	uint len, uint srcdelay, uint destdelay, uint d11_lpbk, uint core_num,
	uint32 mem_addr);
extern int dhdmsgbuf_dmaxfer_status(dhd_pub_t *dhd, dma_xfer_info_t *result);

extern void dhd_dma_buf_init(dhd_pub_t *dhd, void *dma_buf,
	void *va, uint32 len, dmaaddr_t pa, void *dmah, void *secdma);
extern void dhd_prot_flowrings_pool_release(dhd_pub_t *dhd,
	uint16 flowid, void *msgbuf_ring);
extern int dhd_prot_flow_ring_create(dhd_pub_t *dhd, flow_ring_node_t *flow_ring_node);
extern int dhd_post_tx_ring_item(dhd_pub_t *dhd, void *PKTBUF, uint8 ifindex);
extern int dhd_prot_flow_ring_delete(dhd_pub_t *dhd, flow_ring_node_t *flow_ring_node);
extern int dhd_prot_flow_ring_flush(dhd_pub_t *dhd, flow_ring_node_t *flow_ring_node);
extern int dhd_prot_ringupd_dump(dhd_pub_t *dhd, struct bcmstrbuf *b);
extern uint32 dhd_prot_metadata_dbg_set(dhd_pub_t *dhd, bool val);
extern uint32 dhd_prot_metadata_dbg_get(dhd_pub_t *dhd);
extern uint32 dhd_prot_metadatalen_set(dhd_pub_t *dhd, uint32 val, bool rx);
extern uint32 dhd_prot_metadatalen_get(dhd_pub_t *dhd, bool rx);
extern void dhd_prot_print_flow_ring(dhd_pub_t *dhd, void *msgbuf_flow_info, bool h2d,
	struct bcmstrbuf *strbuf, const char * fmt);
extern void dhd_prot_print_info(dhd_pub_t *dhd, struct bcmstrbuf *strbuf);
extern bool dhd_prot_update_txflowring(dhd_pub_t *dhdp, uint16 flow_id, void *msgring_info);
extern void dhd_prot_txdata_write_flush(dhd_pub_t *dhd, uint16 flow_id);
extern uint32 dhd_prot_txp_threshold(dhd_pub_t *dhd, bool set, uint32 val);
extern void dhd_prot_reset(dhd_pub_t *dhd);
extern uint16 dhd_get_max_flow_rings(dhd_pub_t *dhd);

#ifdef IDLE_TX_FLOW_MGMT
extern int dhd_prot_flow_ring_batch_suspend_request(dhd_pub_t *dhd, uint16 *ringid, uint16 count);
extern int dhd_prot_flow_ring_resume(dhd_pub_t *dhd, flow_ring_node_t *flow_ring_node);
#endif /* IDLE_TX_FLOW_MGMT */
extern int dhd_prot_init_info_rings(dhd_pub_t *dhd);
extern int dhd_prot_init_md_rings(dhd_pub_t *dhd);
extern int dhd_prot_check_tx_resource(dhd_pub_t *dhd);
#else /* BCMPCIE */
static INLINE uint32 dhd_prot_get_tx_post_bound(dhd_pub_t *dhd) { return 0; }
static INLINE uint32 dhd_prot_get_ctrl_cpl_post_bound(dhd_pub_t *dhd) { return 0; }
static INLINE uint32 dhd_prot_get_tx_cpl_bound(dhd_pub_t *dhd) { return 0; }
static INLINE uint32 dhd_prot_get_rx_cpl_post_bound(dhd_pub_t *dhd) { return 0; }
static INLINE void dhd_prot_set_tx_cpl_bound(dhd_pub_t *dhd, uint32 val) { }
static INLINE void dhd_prot_set_rx_cpl_post_bound(dhd_pub_t *dhd, uint32 val) { }
static INLINE void dhd_prot_set_tx_post_bound(dhd_pub_t *dhd, uint32 val) { }
static INLINE void dhd_prot_set_ctrl_cpl_post_bound(dhd_pub_t *dhd, uint32 val) { }
#endif /* BCMPCIE */

#ifdef DHD_LB
extern void dhd_lb_tx_compl_handler(unsigned long data);
extern void dhd_lb_rx_compl_handler(unsigned long data);
extern void dhd_lb_rx_process_handler(unsigned long data);
#endif /* DHD_LB */
extern int dhd_prot_h2d_mbdata_send_ctrlmsg(dhd_pub_t *dhd, uint32 mb_data);

#ifdef BCMPCIE
extern int dhd_prot_send_host_timestamp(dhd_pub_t *dhdp, uchar *tlv, uint16 tlv_len,
	uint16 seq, uint16 xt_id);
extern bool dhd_prot_data_path_tx_timestamp_logging(dhd_pub_t *dhd,  bool enable, bool set);
extern bool dhd_prot_data_path_rx_timestamp_logging(dhd_pub_t *dhd,  bool enable, bool set);
extern bool dhd_prot_pkt_noretry(dhd_pub_t *dhd, bool enable, bool set);
extern bool dhd_prot_pkt_noaggr(dhd_pub_t *dhd, bool enable, bool set);
extern bool dhd_prot_pkt_fixed_rate(dhd_pub_t *dhd, bool enable, bool set);
#else /* BCMPCIE */
#define dhd_prot_send_host_timestamp(a, b, c, d, e)		0
#define dhd_prot_data_path_tx_timestamp_logging(a, b, c)	0
#define dhd_prot_data_path_rx_timestamp_logging(a, b, c)	0
#endif /* BCMPCIE */

extern void dhd_prot_dma_indx_free(dhd_pub_t *dhd);

#ifdef EWP_EDL
int dhd_prot_init_edl_rings(dhd_pub_t *dhd);
bool dhd_prot_process_msgbuf_edl(dhd_pub_t *dhd, uint32 *evtlog_items);
int dhd_prot_process_edl_complete(dhd_pub_t *dhd, void *evt_decode_data);
#endif /* EWP_EDL  */

/* APIs for managing a DMA-able buffer */
int  dhd_dma_buf_alloc(dhd_pub_t *dhd, dhd_dma_buf_t *dma_buf, uint32 buf_len);
void dhd_dma_buf_free(dhd_pub_t *dhd, dhd_dma_buf_t *dma_buf);
void dhd_local_buf_reset(char *buf, uint32 len);

/********************************
 * For version-string expansion *
 */
#if defined(BDC)
#define DHD_PROTOCOL "bdc"
#elif defined(CDC)
#define DHD_PROTOCOL "cdc"
#else
#define DHD_PROTOCOL "unknown"
#endif /* proto */

int dhd_get_hscb_info(dhd_pub_t *dhd, void ** va, uint32 *len);
int dhd_get_hscb_buff(dhd_pub_t *dhd, uint32 offset, uint32 length, void * buff);

extern int dhd_prot_mdring_link_unlink(dhd_pub_t *dhd, int idx, bool link);
extern int dhd_prot_mdring_linked_ring(dhd_pub_t *dhd);

#ifdef DHD_MAP_LOGGING
extern void dhd_prot_smmu_fault_dump(dhd_pub_t *dhdp);
#endif /* DHD_MAP_LOGGING */

void dhd_prot_set_ring_size_ver(dhd_pub_t *dhd, int version);
#endif /* _dhd_proto_h_ */
