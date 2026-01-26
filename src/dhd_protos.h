/* Lightweight header to bring canonical driver types into small-helpers during iterative builds.
 * Avoid duplicating prototypes here; include canonical headers which declare correct prototypes/types.
 */

#ifndef _DHD_PROTOS_H_
#define _DHD_PROTOS_H_

#include <dhd.h>
#include <wlioctl.h>
#include <linuxver.h>

/* Small helper prototypes and platform hooks to satisfy -Wmissing-prototypes */
#include <linux/sched.h>
#include <dhd_linux.h>
#include <dhd_plat.h>

/* Scheduler helpers */
int setScheduler(struct task_struct *p, int policy, struct sched_param *param);
int get_scheduler_policy(struct task_struct *p);

/* Platform device helpers are declared in canonical headers like dhd_linux.h/dhd_plat.h; avoid duplicating here. */

/* Forward declarations for types used in the prototypes below. These are lightweight
 * placeholders so this helper header can be included from many translation units
 * without pulling in large PCIE-specific headers. When possible move prototypes to
 * their canonical headers and remove the forward declarations.
 */
struct dhd_bus;
struct dhdpcie_info;
struct dhd_prot;
struct dhd_info;
typedef struct dhd_info dhd_info_t;
typedef struct dhdpcie_info dhdpcie_info_t;
typedef struct dhd_prot dhd_prot_t;
typedef struct dhd_bus dhd_bus_t;
typedef int dhd_pcie_link_state_type_t;


/* Additional prototypes added to satisfy -Wmissing-prototypes during iterative mainline porting.
 * These are intended as short-term placeholders. When possible move declarations into their
 * canonical headers (e.g. dhd_*.h, dhd_linux.h, dhd_pcie.h) before final upstreaming.
 */

/* STA management */
/* moved to dhd_linux.h (canonical) */

/* BSS/Address helpers */
/* dhd_bssidx2bssid moved to dhd_linux.h (canonical) */


/* Bus and device lifecycle — canonical prototypes are declared in dhd_bus.h/dhd_pcie.h */

/* Capability and utility helpers — canonical prototypes are declared in dhd_pcie.h/dhd_config.h */

/* Configuration — canonical prototypes are declared in dhd_config.h */

/* Counters / stats - canonicalized into src/dhd.h */
/* dhd_create_ecounters_params moved to src/dhd.h */

/* Device IOCTLs - canonicalized into src/dhd_linux.h */
/* dhd_dev_init_ioctl moved to src/dhd_linux.h */
/* dhd_download_2_dongle: canonical prototype moved to src/dhd.h */

/* DPC/tasklet control - canonicalized into src/dhd_pcie.h where appropriate */
/* dhd_dpc_enable / dhd_dpc_tasklet_kill moved to src/dhd_pcie.h */

/* Dump helpers — canonical prototypes are declared in dhd.h */

/* STA find — internal helpers are defined in src/dhd_linux.c (made static where appropriate) */


/* Failure collection — internal helpers are defined in src/dhd_linux.c (made static where appropriate) */

/* Configuration access */
/* dhd_get_conf moved to dhd_linux.h (canonical) */
/* dhd_get_device_dt_name: weak implementation lives in src/dhd_linux_platdev.c; no canonical prototype needed */
/* Flowring helpers (canonical): see src/dhd_flowring.h */

/* Link recovery */
/* dhd_host_recover_link: canonical prototype moved to src/dhd_linux.h */

/* DPC histos — canonical prototype in dhd.h */

/* IOCTL entry — canonical prototype in dhd_linux.h */

/* SDIO helpers — canonical prototypes are in dhd_proto.h (dhdsdio_*) and dhd_sdio.c; dhd_sr_config is declared in dhd_proto.h */

/* Static netdev test — internal to src/dhd_linux.c (made static) */

/* Load-balance stats — internal helpers are defined static in src/dhd_linux_lb.c */


/* Legacy/optimised preinit */
/* dhd_legacy_preinit_ioctls/dhd_optimised_preinit_ioctls: now static in src/dhd_linux.c */
/* dhd_log_dump_get_timestamp: canonical prototype in include/linuxver.h / src/dhd_log_dump.h */
/* dhd_mem_debug: internal to src/dhd_common.c (made static) */

/* Open/Init helpers — canonical prototypes are declared in src/dhd.h */

/* PCIE helpers moved to dhd_pcie.h (canonical) */

/* DMAXFER/free helper — canonical prototype in dhd_proto.h */

/* IRQ/KIRQS */
/* dhd_print_kirqstats: internal to src/dhd_linux.c (made static) */

/* Prot helpers — canonical prototypes are declared in dhd_proto.h */

/* Role / interface mapping */
/* dhd_role_to_nl80211_iftype: canonical prototype moved to src/dhd_linux.h */

/* RX emerge helpers */
/* dhd_rx_emerge_enqueue / dequeue: canonical prototypes are in src/dhd_proto.h */

/* Sendup / packet helpers */
/* dhd_sendup moved to dhd_linux.h (canonical) */
/* dhd_set_bus_params: canonical prototype in dhd_bus.h */
/* _dhd_set_mac_address: internal helper made static in src/dhd_linux.c */
/* dhd_set_packet_filter: canonical prototype moved to src/dhd_linux.h */
/* dhd_set_scb_probe: canonical prototype moved to src/dhd_linux.h */
/* _dhd_tdls_enable: internal helper made static in src/dhd_linux.c */
/* PCIE helpers — canonical prototypes are declared in src/dhd_pcie.h */
/* dmaxfer_free_dmaaddr_handler: internal helper made static in src/dhd_linux.c */
/* is_tdls_destination: canonical prototype in src/dhd_flowring.h */
/* net_os_wake_lock_ctrl_timeout_enable: canonical prototype moved to src/dhd_linux.h */
/* pattern_atoh_len: canonical prototype moved to src/dhd.h */

/* WL / cfg80211 helpers — canonical prototypes are declared in WL headers (e.g. src/wl_cfg80211.h) */

/* File helpers */
/* write_dump_to_file/write_file: canonical prototypes are declared in src/dhd_log_dump.h */

#endif /* _DHD_PROTOS_H_ */

