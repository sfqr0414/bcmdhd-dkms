/* Minimal prototype header for dhd_bus_stubs.c
 * Provides forward declarations and prototypes for stub implementations
 * without pulling the full heavy canonical headers.
 */
#ifndef _DHD_STUB_PROTOS_H_
#define _DHD_STUB_PROTOS_H_

#include <linux/types.h>

/* Minimal forward typedefs used by the stubs */
typedef struct dhd_pub dhd_pub_t;
typedef struct dhd_bus dhd_bus_t;
typedef struct bcmstrbuf bcmstrbuf;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned char uint8;
typedef void osl_t; /* opaque for stub builds */
struct net_device; /* forward */

/* Protocol fallbacks */
void dhd_prot_stop(dhd_pub_t *dhd);
int dhd_prot_ioctl(dhd_pub_t *dhd, int ifidx, void *ioc, void *buf, int len);
int dhd_prot_init(dhd_pub_t *dhd);
void dhd_prot_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf);
void dhd_prot_dstats(dhd_pub_t *dhdp);
uint dhd_prot_hdrlen(dhd_pub_t *dhd, void *txp);
int dhd_prot_iovar_op(dhd_pub_t *dhdp, const char *name, void *params, uint plen, void *arg, uint len, bool set);
int dhd_prot_hdrpull(dhd_pub_t *dhd, int *ifidx, void *rxp, uchar *buf, uint *len);
int dhd_prot_attach(dhd_pub_t *dhdp);
void dhd_prot_detach(dhd_pub_t *dhdp);
void dhd_prot_hdrpush(dhd_pub_t *dhd, int ifidx, void *txp);

/* Bus fallbacks */
int dhd_bus_init(dhd_pub_t *dhdp, bool enforce_mutex);
void dhd_bus_unregister(void);
bool dhd_bus_dpc(struct dhd_bus *bus);
uint dhd_bus_chip_id(dhd_pub_t *dhdp);
int dhd_bus_iovar_op(dhd_pub_t *dhdp, const char *name, void *params, uint plen, void *arg, uint len, bool set);
int dhd_bus_txdata(struct dhd_bus *bus, void *txp);
int dhd_bus_console_in(dhd_pub_t *dhd, uchar *msg, uint msglen);
void dhd_bus_clearcounts(dhd_pub_t *dhdp);
void dhd_bus_dump_trap_info(struct dhd_bus *bus, struct bcmstrbuf *b);
void dhd_bus_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf);
int dhd_bus_download_firmware(struct dhd_bus *bus, void *osh, char *fw_path, char *nv_path, char *clm_path, char *conf_path);
bool dhd_bus_watchdog(dhd_pub_t *dhd);
int dhd_net_bus_devreset(struct net_device *dev, uint8 flag);
int dhd_socram_dump(struct dhd_bus *bus);
int dhd_sync_with_dongle(dhd_pub_t *dhd);
void dhd_bus_set_signature_path(struct dhd_bus *bus, char *sig_path);
void dhd_bus_stop(struct dhd_bus *bus, bool enforce_mutex);
uint dhd_bus_chip(struct dhd_bus *bus);
uint dhd_bus_chiprev(struct dhd_bus *bus);

#endif /* _DHD_STUB_PROTOS_H_ */
