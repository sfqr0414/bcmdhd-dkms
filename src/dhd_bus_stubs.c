/* Minimal bus/proto stubs for non-PCIE builds to satisfy link-time references
 * during mainline porting iterations. These are no-op placeholders and should be
 * replaced by real bus implementations (sdio/usb/pcie) or upstreamed stubs where
 * appropriate before production use.
 */

#include <linux/types.h>
#include <linux/errno.h>

/* Pull canonical prototypes so stub implementations match authoritative
 * signatures and avoid duplicating conflicting declarations. These headers
 * are deliberately small and stable for prototype visibility.
 */
#include "dhd_stub_protos.h"

/* Use forward declarations to avoid pulling heavy kernel/driver headers into
 * this small stub translation unit which is intended as a build-time placeholder.
 */
typedef struct dhd_pub dhd_pub_t;
typedef struct bcmstrbuf bcmstrbuf;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned char uint8;


/* Fallback implementations are provided *only* when no real bus backend is built.
 * If BCMSDIO/BCMDBUS/BCMPCIE are defined, the corresponding real implementations will be
 * compiled elsewhere and the fallbacks must not be present to avoid duplicate symbols.
 */

/* Protocol fallbacks (only when no bus backend is compiled) */
#if !defined(BCMSDIO) && !defined(BCMDBUS) && !defined(BCMPCIE)
void dhd_prot_stop(dhd_pub_t *dhd)
{
	/* No-op fallback for builds without any bus implementation */
}

int dhd_prot_ioctl(dhd_pub_t *dhd, int ifidx, void * ioc, void * buf, int len)
{
	return -ENODEV;
}

int dhd_prot_init(dhd_pub_t *dhd)
{
	return -ENODEV;
}

void dhd_prot_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf)
{
	/* No-op */
}

void dhd_prot_dstats(dhd_pub_t *dhdp)
{
	/* No-op */
}

uint dhd_prot_hdrlen(dhd_pub_t *dhd, void *txp)
{
	return 0;
}

int dhd_prot_iovar_op(dhd_pub_t *dhdp, const char *name,
			void *params, uint plen, void *arg, uint len, bool set)
{
	return -ENODEV;
}

int dhd_prot_hdrpull(dhd_pub_t *dhd, int *ifidx, void *rxp, uchar *buf, uint *len)
{
	return -ENODATA;
}
#endif /* no bus backends */

/* Bus fallbacks (only when no bus backend is compiled) */
#if !defined(BCMSDIO) && !defined(BCMDBUS) && !defined(BCMPCIE)
int dhd_bus_init(dhd_pub_t *dhdp, bool enforce_mutex)
{
	/* Not supported in this build; return failure so callers can handle gracefully */
	return -ENODEV;
}

void dhd_bus_unregister(void)
{
	/* No-op */
}

bool dhd_bus_dpc(struct dhd_bus *bus)
{
	return false;
}

uint dhd_bus_chip_id(dhd_pub_t *dhdp)
{
	return 0;
}

int dhd_bus_iovar_op(dhd_pub_t *dhdp, const char *name,
                            void *params, uint plen, void *arg, uint len, bool set)
{
	return -ENODEV;
}

int dhd_bus_txdata(struct dhd_bus *bus, void *txp)
{
	return -ENODEV;
}

int dhd_bus_console_in(dhd_pub_t *dhd, uchar *msg, uint msglen)
{
	return -ENODEV;
}

void dhd_bus_clearcounts(dhd_pub_t *dhdp)
{
	/* No-op */
}

void dhd_bus_dump_trap_info(struct dhd_bus *bus, struct bcmstrbuf *b)
{
	/* No-op */
}

/* Missing earlier: add a simple bus dump and counters fallback so symbols are present */
void dhd_bus_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf)
{
	/* No-op */
}

void dhd_bus_counters(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf)
{
	/* No-op */
}

int dhd_prot_attach(dhd_pub_t *dhdp)
{
	return -ENODEV;
}

void dhd_prot_detach(dhd_pub_t *dhdp)
{
	/* No-op */
}

void dhd_prot_hdrpush(dhd_pub_t *dhd, int ifidx, void *txp)
{
	/* No-op */
}
int dhd_dongle_ramsize = 0;

uint dhd_bus_chip(struct dhd_bus *bus)
{
	return 0;
}

uint dhd_bus_chiprev(struct dhd_bus *bus)
{
	return 0;
}

int dhd_sync_with_dongle(dhd_pub_t *dhd)
{
	return -ENODEV;
}

int dhd_bus_download_firmware(struct dhd_bus *bus, void *osh,
	char *fw_path, char *nv_path, char *clm_path, char *conf_path)
{
	return -ENODEV;
}

bool dhd_bus_watchdog(dhd_pub_t *dhd)
{
	return false;
}

int dhd_net_bus_devreset(struct net_device *dev, uint8 flag)
{
	return -ENODEV;
}

int dhd_socram_dump(struct dhd_bus *bus)
{
	return -ENODEV;
}

void dhd_bus_set_signature_path(struct dhd_bus *bus, char *sig_path)
{
	/* No-op */
}

void dhd_bus_stop(struct dhd_bus *bus, bool enforce_mutex)
{
	/* No-op */
}
#endif /* no bus backends */
