
#include <osl.h>
#include <dhd_linux.h>
#include <dhd_dbg.h>
#include <linux/gpio.h>
/* Linux 6.18.3+ GPIO descriptor API */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
#include <linux/gpio/consumer.h>
#else
/* Fallback for older kernels - will use legacy GPIO API */
#warning "GPIO descriptor API not available, falling back to legacy GPIO API"
#endif
#include <linux/delay.h>

#ifdef BCMDHD_DTS
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#endif
#ifdef BCMDHD_PLATDEV
#include <linux/platform_device.h>
#endif
/* Rockchip-specific rfkill removed; use standard RFKill in cfg80211 */

/* Legacy GPIO API compatibility shim for kernels < 4.5.0 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0)
/* Provide stub implementations that return -ENOTSUPP for old kernels */
static inline struct gpio_desc *devm_gpiod_get_optional(struct device *dev,
		const char *con_id, enum gpiod_flags flags)
{
	return ERR_PTR(-ENOTSUPP);
}

static inline void gpiod_set_value_cansleep(struct gpio_desc *desc, int value)
{
	/* No-op for compatibility */
}

static inline int gpiod_to_irq(const struct gpio_desc *desc)
{
	return -ENOTSUPP;
}

static inline int desc_to_gpio(const struct gpio_desc *desc)
{
	return -EINVAL;
}

static inline void gpiod_set_consumer_name(struct gpio_desc *desc, const char *name)
{
	/* No-op for compatibility */
}

/* Define GPIOD flags for compilation */
#ifndef GPIOD_OUT_LOW
#define GPIOD_OUT_LOW 0
#endif
#ifndef GPIOD_IN
#define GPIOD_IN 0
#endif
#ifndef GPIOD_ASIS
#define GPIOD_ASIS 0
#endif
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0) */

#ifdef CONFIG_DHD_USE_STATIC_BUF
/* use canonical prototype: void *dhd_wlan_mem_prealloc( ... ) declared in src/dhd.h */
#endif /* CONFIG_DHD_USE_STATIC_BUF */
#ifdef CUSTOMER_HW_ROCKCHIP
#ifdef BCMPCIE
//extern void rk_pcie_power_on_atu_fixup(void);
#endif
#endif

#ifdef BCMDHD_DTS
/* This is sample code in dts file.
bcmdhd_wlan {
    compatible = "android,bcmdhd_wlan";
    wl_reg_on-gpios   = <&gpio2 RK_PC5 GPIO_ACTIVE_HIGH>;
    wl_host_wake-gpios = <&gpio0 RK_PB0 GPIO_ACTIVE_HIGH>;
};
*/
#define DHD_DT_COMPAT_ENTRY     "android,bcmdhd_wlan"
/* Connection IDs for gpiod API. 
  Matches properties with "-gpios" suffix in DTS.
  Legacy names without "-gpios" are deprecated. 
*/
#define GPIO_WL_REG_ON_PROPNAME      "wl_reg_on"
#define GPIO_WL_HOST_WAKE_PROPNAME   "wl_host_wake"
#endif



static int
dhd_wlan_set_power(int on, wifi_adapter_info_t *adapter)
{
	int ret = 0;

	if (!adapter || !adapter->gpiod_wl_reg_on) {
		DHD_ERROR(("%s: invalid adapter or WL_REG_ON GPIO not available\n", __FUNCTION__));
		return -ENODEV;
	}

	DHD_ERROR(("%s: Power %s via gpiod_set_value_cansleep\n", __FUNCTION__, on ? "ON" : "OFF"));

	gpiod_set_value_cansleep(adapter->gpiod_wl_reg_on, on ? 1 : 0);

	/* Hardware timing requirement for power-on */
	if (on) {
		usleep_range(200, 300);
	}

	DHD_ERROR(("%s: WL_REG_ON set to %d successfully\n", __FUNCTION__, on));
	return 0;
}

static int
dhd_wlan_set_reset(int onoff, wifi_adapter_info_t *adapter)
{
	int ret = 0;

	if (!adapter || !adapter->gpiod_wl_reg_on) {
		DHD_ERROR(("%s: invalid adapter or WL_REG_ON GPIO not available\n", __FUNCTION__));
		return -ENODEV;
	}

	DHD_ERROR(("%s: Reset %s via gpiod_set_value_cansleep\n", __FUNCTION__, onoff ? "ON" : "OFF"));

	gpiod_set_value_cansleep(adapter->gpiod_wl_reg_on, onoff ? 1 : 0);

	/* Hardware timing requirement for reset */
	usleep_range(100, 200);

	DHD_ERROR(("%s: WL_REG_ON set to %d successfully\n", __FUNCTION__, onoff));
	return 0;
}

static int
dhd_wlan_get_mac_addr(unsigned char *buf, int ifidx)
{
	int err = -1;

	if (ifidx == 1) {
#ifdef EXAMPLE_GET_MAC
		struct ether_addr ea_example = {{0x00, 0x11, 0x22, 0x33, 0x44, 0xFF}};
		bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
#endif /* EXAMPLE_GET_MAC */
	} else {
#ifdef EXAMPLE_GET_MAC
		struct ether_addr ea_example = {{0x02, 0x11, 0x22, 0x33, 0x44, 0x55}};
		bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
#endif /* EXAMPLE_GET_MAC */
/* BCMDHD_PLATDEV MAC address retrieval disabled: adapter parameter not available in callback context.
 * This should be implemented via platform data registration, not here.
 */
#if 0 && defined(BCMDHD_PLATDEV)
		if (adapter->pdev && adapter->pdev->dev.of_node) {
			const void *mac_addr;
			int mac_len;
			mac_addr = of_get_property(adapter->pdev->dev.of_node, "mac-address", &mac_len);
			if (mac_addr && mac_len >= 6) {
				memcpy(buf, mac_addr, 6);
				err = 0;
			}
		}
#endif
	}

#ifdef EXAMPLE_GET_MAC_VER2
	/* EXAMPLE code */
	{
		char macpad[56]= {
		0x00,0xaa,0x9c,0x84,0xc7,0xbc,0x9b,0xf6,
		0x02,0x33,0xa9,0x4d,0x5c,0xb4,0x0a,0x5d,
		0xa8,0xef,0xb0,0xcf,0x8e,0xbf,0x24,0x8a,
		0x87,0x0f,0x6f,0x0d,0xeb,0x83,0x6a,0x70,
		0x4a,0xeb,0xf6,0xe6,0x3c,0xe7,0x5f,0xfc,
		0x0e,0xa7,0xb3,0x0f,0x00,0xe4,0x4a,0xaf,
		0x87,0x08,0x16,0x6d,0x3a,0xe3,0xc7,0x80};
		bcopy(macpad, buf+6, sizeof(macpad));
	}
#endif /* EXAMPLE_GET_MAC_VER2 */

	DHD_ERROR(("%s err=%d\n", __FUNCTION__, err));

	return err;
}

static struct cntry_locales_custom brcm_wlan_translate_custom_table[] = {
	/* Table should be filled out based on custom platform regulatory requirement */
#ifdef EXAMPLE_TABLE
	{"",   "XT", 49},  /* Universal if Country code is unknown or empty */
	{"US", "US", 0},
#endif /* EXMAPLE_TABLE */
};

#ifdef CUSTOM_FORCE_NODFS_FLAG
struct cntry_locales_custom brcm_wlan_translate_nodfs_table[] = {
#ifdef EXAMPLE_TABLE
	{"",   "XT", 50},  /* Universal if Country code is unknown or empty */
	{"US", "US", 0},
#endif /* EXMAPLE_TABLE */
};
#endif

static void *dhd_wlan_get_country_code(char *ccode
#ifdef CUSTOM_FORCE_NODFS_FLAG
	, u32 flags
#endif
)
{
	struct cntry_locales_custom *locales;
	int size;
	int i;

	if (!ccode)
		return NULL;

#ifdef CUSTOM_FORCE_NODFS_FLAG
	if (flags & WLAN_PLAT_NODFS_FLAG) {
		locales = brcm_wlan_translate_nodfs_table;
		size = ARRAY_SIZE(brcm_wlan_translate_nodfs_table);
	} else {
#endif
		locales = brcm_wlan_translate_custom_table;
		size = ARRAY_SIZE(brcm_wlan_translate_custom_table);
#ifdef CUSTOM_FORCE_NODFS_FLAG
	}
#endif

	for (i = 0; i < size; i++)
		if (strcmp(ccode, locales[i].iso_abbrev) == 0)
			return &locales[i];
	return NULL;
}

/* Forward declaration: dhd_wlan_set_carddetect implemented in dhd_linux_platdev.c */
extern int dhd_wlan_set_carddetect(int present, wifi_adapter_info_t *adapter);

struct wifi_platform_data dhd_wlan_control = {
	.set_power	= dhd_wlan_set_power,
	.set_reset	= dhd_wlan_set_reset,
	.set_carddetect	= dhd_wlan_set_carddetect,
	.get_mac_addr	= dhd_wlan_get_mac_addr,
#ifdef CONFIG_DHD_USE_STATIC_BUF
	.mem_prealloc	= dhd_wlan_mem_prealloc,
#endif /* CONFIG_DHD_USE_STATIC_BUF */
	.get_country_code = dhd_wlan_get_country_code,
};

static int
dhd_wlan_init_gpio(wifi_adapter_info_t *adapter)
{
#ifdef BCMDHD_DTS
	char wlan_node[32];
	struct device_node *root_node = NULL;
#endif
	int ret = 0;
	struct gpio_desc *gpiod_wl_reg_on = NULL;
	struct gpio_desc *gpiod_wl_host_wake = NULL;
	int host_oob_irq = -1;
	uint host_oob_irq_flags = 0;
	#ifdef CUSTOMER_HW_ROCKCHIP
	#ifdef HW_OOB
	int irq_flags = -1;
	#endif
	#endif
	struct device *dev = NULL;

	/* Initialize legacy GPIO numbers to invalid */
	adapter->gpio_wl_reg_on = -1;
	adapter->gpio_wl_host_wake = -1;

	/* Please check your schematic and fill right GPIO number which connected to
	* WL_REG_ON and WL_HOST_WAKE.
	*/
	/* Find the wifi node by compatible string rather than hardcoding GPIO numbers */
	root_node = of_find_compatible_node(NULL, NULL, DHD_DT_COMPAT_ENTRY);
	if (!root_node) {
		DHD_ERROR(("cannot find device node for %s\n", DHD_DT_COMPAT_ENTRY));
		ret = -ENODEV;
		goto out;
	}

	/* Try to recover platform device when adapter->pdev is NULL (non-fatal) */
#ifdef BCMDHD_PLATDEV
	if (adapter->pdev) {
		dev = &adapter->pdev->dev;
	} else {
		struct platform_device *pdev_found = of_find_device_by_node(root_node);
		if (pdev_found) {
			adapter->pdev = pdev_found;
			dev = &pdev_found->dev;
			DHD_ERROR(("recovered platform device from DT node\n"));
		} else {
			dev = NULL;
			DHD_ERROR(("no platform device for node, will use DT-only GPIO lookup\n"));
		}
	}
#else
	dev = NULL;
#endif

	DHD_ERROR(("======== Get GPIO from DTS node %pOF ========\n", root_node));

	/* WL_REG_ON GPIO: Parse with GPIOD_ASIS (no request, no direction set) to avoid conflict with mmc-pwrseq */
	{
		struct gpio_desc *desc = NULL;
		int gpio_num = -1;
		
		/* Try device-managed descriptor with GPIOD_ASIS flag */
		if (dev) {
			desc = devm_gpiod_get_optional(dev, GPIO_WL_REG_ON_PROPNAME, GPIOD_ASIS);
			if (IS_ERR(desc)) {
				int tmp_err = PTR_ERR(desc);
				DHD_ERROR(("devm_gpiod_get_optional(WL_REG_ON) with GPIOD_ASIS failed: %d\n", tmp_err));
				desc = NULL;
			}
		}
		
		/* Fallback to DT lookup using gpio_to_desc */
		if (!desc) {
			gpio_num = of_get_named_gpio(root_node, GPIO_WL_REG_ON_PROPNAME "-gpios", 0);
			if (gpio_is_valid(gpio_num)) {
				desc = gpio_to_desc(gpio_num);
				if (IS_ERR(desc)) {
					int tmp_err = PTR_ERR(desc);
					DHD_ERROR(("gpio_to_desc(wl_reg_on) failed: %d\n", tmp_err));
					desc = NULL;
				} else {
					adapter->gpio_wl_reg_on = gpio_num;
					DHD_ERROR(("WL_REG_ON GPIO number: %d (from DT fallback)\n", gpio_num));
				}
			} else {
				DHD_ERROR(("WL_REG_ON GPIO not found in device tree\n"));
			}
		}
		
		/* Validate and store the descriptor */
		if (desc) {
			/* Stricter IS_ERR/NULL checks, error code differentiation, unified exit */
			if (IS_ERR(desc)) {
				ret = PTR_ERR(desc);
				DHD_ERROR(("WL_REG_ON GPIO descriptor is invalid: %d\n", ret));
				adapter->gpiod_wl_reg_on = NULL;
				goto out;
			}
			if (!desc) {
				ret = -ENODEV;
				DHD_ERROR(("WL_REG_ON GPIO descriptor is NULL\n"));
				adapter->gpiod_wl_reg_on = NULL;
				goto out;
			}

			gpiod_set_consumer_name(desc, "WL_REG_ON");
			adapter->gpiod_wl_reg_on = desc;
			ret = 0;
			DHD_ERROR(("WL_REG_ON GPIO descriptor parsed (GPIOD_ASIS, no request/direction set)\n"));
		} else {
			DHD_ERROR(("WL_REG_ON GPIO descriptor not available; power managed by mmc-pwrseq\n"));
			adapter->gpiod_wl_reg_on = NULL;
		}
	}

	/* WL_HOST_WAKE: try device-managed descriptor first, then DT lookup; ensure we can get a valid IRQ */
	{
		struct gpio_desc *desc = NULL;
		int gpio_num = -1;
		/* Prefer device-managed descriptor when possible (for proper ownership) */
		if (dev) {
			desc = devm_gpiod_get_optional(dev, GPIO_WL_HOST_WAKE_PROPNAME, GPIOD_IN);
			if (IS_ERR(desc)) {
				ret = PTR_ERR(desc);
				DHD_ERROR(("devm_gpiod_get_optional(WL_HOST_WAKE) failed: %d\n", ret));
				desc = NULL;
			}
		}
		/* Fallback to DT lookup using gpio_to_desc */
		if (!desc) {
			gpio_num = of_get_named_gpio(root_node, GPIO_WL_HOST_WAKE_PROPNAME "-gpios", 0);
			if (gpio_is_valid(gpio_num)) {
				desc = gpio_to_desc(gpio_num);
					if (IS_ERR(desc)) {
						ret = PTR_ERR(desc);
						DHD_ERROR(("gpio_to_desc(wl_host_wake) failed: %d\n", ret));
						desc = NULL;
					} else {
						adapter->gpio_wl_host_wake = gpio_num;
					}
			}
		}

		if (desc) {
			gpiod_set_consumer_name(desc, "WL_HOST_WAKE");
			adapter->gpiod_wl_host_wake = desc;
			ret = 0;
			host_oob_irq = gpiod_to_irq(desc);
			if (host_oob_irq < 0) {
				/* Try to obtain IRQ from DT interrupt properties as a fallback */
				int idx = of_property_match_string(root_node, "interrupt-names", "host-wake");
				if (idx >= 0)
					host_oob_irq = of_irq_get(root_node, idx);
			}

			if (host_oob_irq < 0) {
				DHD_ERROR(("failed to get OOB IRQ for WL_HOST_WAKE: %d\n", host_oob_irq));
				ret = -ENODEV;
				goto out;
			}

			/* Log IRQ registration for easier dmesg verification */
			DHD_ERROR(("Registered OOB IRQ %d for WL_HOST_WAKE\n", host_oob_irq));

			/* Default shared/edge flags; platform may override later */
			host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE | IORESOURCE_IRQ_SHAREABLE;
			adapter->irq_num = host_oob_irq;
			adapter->intr_flags = host_oob_irq_flags & IRQF_TRIGGER_MASK;
		} else {
			DHD_ERROR(("WL_HOST_WAKE not defined in device tree\n"));
		}
	}

out:
	if (root_node)
		of_node_put(root_node);
	return ret;
}

static void
dhd_wlan_deinit_gpio(wifi_adapter_info_t *adapter)
{
	/* WL_REG_ON: Clear descriptor reference only, no hardware operations */
	if (adapter->gpiod_wl_reg_on) {
		DHD_ERROR(("Clearing WL_REG_ON GPIO descriptor (no hardware operation)\n"));
		adapter->gpiod_wl_reg_on = NULL;
	}
	adapter->gpio_wl_reg_on = -1;

#ifdef CUSTOMER_OOB
	if (adapter->gpiod_wl_host_wake) {
		DHD_ERROR(("Clearing WL_HOST_WAKE GPIO descriptor\n"));
		adapter->gpiod_wl_host_wake = NULL;
	}
	adapter->gpio_wl_host_wake = -1;
#endif /* CUSTOMER_OOB */
}

#if defined(BCMDHD_MDRIVER)
static void
dhd_wlan_init_adapter(wifi_adapter_info_t *adapter)
{
#ifdef ADAPTER_IDX
	if (ADAPTER_IDX == 0) {
		adapter->bus_num = 1;
		adapter->slot_num = 1;
	} else if (ADAPTER_IDX == 1) {
		adapter->bus_num = 2;
		adapter->slot_num = 1;
	}
	adapter->index = ADAPTER_IDX;
#ifdef BCMSDIO
	adapter->bus_type = SDIO_BUS;
#elif defined(BCMPCIE)
	adapter->bus_type = PCI_BUS;
#elif defined(BCMDBUS)
	adapter->bus_type = USB_BUS;
#endif
	DHD_ERROR(("bus_type=%d, bus_num=%d, slot_num=%d\n",
		adapter->bus_type, adapter->bus_num, adapter->slot_num));
#endif /* ADAPTER_IDX */

#ifdef DHD_STATIC_IN_DRIVER
	adapter->index = 0;
#elif !defined(ADAPTER_IDX)
#ifdef BCMSDIO
	adapter->index = 0;
#elif defined(BCMPCIE)
	adapter->index = 1;
#elif defined(BCMDBUS)
	adapter->index = 2;
#endif
#endif /* DHD_STATIC_IN_DRIVER */
}
#endif /* BCMDHD_MDRIVER */

int
dhd_wlan_init_plat_data(wifi_adapter_info_t *adapter)
{
	int err = 0;

#ifdef BCMDHD_MDRIVER
	dhd_wlan_init_adapter(adapter);
#endif /* BCMDHD_MDRIVER */

	err = dhd_wlan_init_gpio(adapter);
	if (err)
		goto exit;

exit:
	return err;
}

void
dhd_wlan_deinit_plat_data(wifi_adapter_info_t *adapter)
{
	dhd_wlan_deinit_gpio(adapter);
}

/* Legacy GPIO number compatibility API - returns -ENOTSUPP if gpiod not available */
int
dhd_wlan_get_gpio_number(wifi_adapter_info_t *adapter, const char *gpio_name)
{
	if (!adapter || !gpio_name)
		return -EINVAL;

	if (strcmp(gpio_name, GPIO_WL_REG_ON_PROPNAME) == 0) {
		return adapter->gpio_wl_reg_on;
	}
#ifdef CUSTOMER_OOB
	else if (strcmp(gpio_name, GPIO_WL_HOST_WAKE_PROPNAME) == 0) {
		return adapter->gpio_wl_host_wake;
	}
#endif
	
	return -ENOTSUPP;
}
