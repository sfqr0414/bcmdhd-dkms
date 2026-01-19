
#ifndef _wl_event_
#define _wl_event_
typedef enum WL_EVENT_PRIO {
	PRIO_EVENT_IAPSTA,
	PRIO_EVENT_ESCAN,
	PRIO_EVENT_WEXT
}wl_event_prio_t;
s32 wl_ext_event_attach(struct net_device *net);
void wl_ext_event_dettach(dhd_pub_t *dhdp);
int wl_ext_event_attach_netdev(struct net_device *net, int ifidx, uint8 bssidx);
int wl_ext_event_dettach_netdev(struct net_device *net, int ifidx);
int wl_ext_event_register(struct net_device *dev, dhd_pub_t *dhd,
	uint32 event, void *cb_func, void *data, wl_event_prio_t prio);
void wl_ext_event_deregister(struct net_device *dev, dhd_pub_t *dhd,
	uint32 event, void *cb_func);
void wl_ext_event_send(void *params, const wl_event_msg_t * e, void *data);

/* Canonical WL helpers - expose prototypes to avoid TU-only missing prototypes */
extern uint wl_get_port_num(wl_io_pport_t *io_pport);
extern int wl_event_process_default(wl_event_msg_t *event, struct wl_evt_pport *evt_pport);
extern int wl_event_process(dhd_pub_t *dhd_pub, int *ifidx, void *pktdata,
	uint pktlen, void **data_ptr, void *raw_event);
#endif
