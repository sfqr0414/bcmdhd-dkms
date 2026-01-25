
#include <osl.h>
#include <dhd_linux.h>
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
	gpio_wl_reg_on = <&gpio GPIOH_4 GPIO_ACTIVE_HIGH>;
	gpio_wl_host_wake = <&gpio GPIOZ_15 GPIO_ACTIVE_HIGH>;
};
*/
#define DHD_DT_COMPAT_ENTRY		"android,bcmdhd_wlan"
#define GPIO_WL_REG_ON_PROPNAME		"gpio_wl_reg_on"
#define GPIO_WL_HOST_WAKE_PROPNAME	"gpio_wl_host_wake"
#endif

static int
dhd_wlan_set_power(int on, wifi_adapter_info_t *adapter)
{
	struct gpio_desc *gpiod_wl_reg_on = adapter->gpiod_wl_reg_on;
	int err = 0;

	if (on) {
		printf("======== PULL WL_REG_ON HIGH! ========\n");
		if (gpiod_wl_reg_on) {
			gpiod_set_value_cansleep(gpiod_wl_reg_on, 1);
		}
		/* Standardized power sequence: wait for module to power up */
		mdelay(20);
#ifdef BUS_POWER_RESTORE
#ifdef BCMPCIE
		if (adapter->pci_dev) {
			mdelay(100);
			printf("======== pci_set_power_state PCI_D0! ========\n");
			pci_set_power_state(adapter->pci_dev, PCI_D0);
			if (adapter->pci_saved_state)
				pci_load_and_free_saved_state(adapter->pci_dev, &adapter->pci_saved_state);
			pci_restore_state(adapter->pci_dev);
			err = pci_enable_device(adapter->pci_dev);
			if (err < 0)
				printf("%s: PCI enable device failed", __FUNCTION__);
			pci_set_master(adapter->pci_dev);
		}
#endif /* BCMPCIE */
#endif /* BUS_POWER_RESTORE */
		/* Lets customer power to get stable */
	} else {
#ifdef BUS_POWER_RESTORE
#ifdef BCMPCIE
		if (adapter->pci_dev) {
			printf("======== pci_set_power_state PCI_D3hot! ========\n");
			pci_save_state(adapter->pci_dev);
			adapter->pci_saved_state = pci_store_saved_state(adapter->pci_dev);
			if (pci_is_enabled(adapter->pci_dev))
				pci_disable_device(adapter->pci_dev);
			pci_set_power_state(adapter->pci_dev, PCI_D3hot);
		}
#endif /* BCMPCIE */
#endif /* BUS_POWER_RESTORE */
		printf("======== PULL WL_REG_ON LOW! ========\n");
		if (gpiod_wl_reg_on) {
			gpiod_set_value_cansleep(gpiod_wl_reg_on, 0);
		}
		/* Standardized power sequence: wait for module to power down */
		mdelay(20);
	}

	return err;
}

static int
dhd_wlan_set_reset(int onoff)
{
	return 0;
}

static int
dhd_wlan_set_carddetect(int present)
{
	int err = 0;

	if (present) {
#if defined(BCMSDIO)
		printf("======== Card detection to detect SDIO card! ========\n");
#ifdef CUSTOMER_HW_PLATFORM
		err = sdhci_force_presence_change(&sdmmc_channel, 1);
#endif /* CUSTOMER_HW_PLATFORM */
/* platform-specific card-detect removed; rely on standard sdhci presence change */
#elif defined(BCMPCIE)
		printf("======== Card detection to detect PCIE card! ========\n");
#endif
	} else {
#if defined(BCMSDIO)
		printf("======== Card detection to remove SDIO card! ========\n");
#ifdef CUSTOMER_HW_PLATFORM
		err = sdhci_force_presence_change(&sdmmc_channel, 0);
#endif /* CUSTOMER_HW_PLATFORM */
/* platform-specific card-detect removed; rely on standard sdhci presence change */
#elif defined(BCMPCIE)
		printf("======== Card detection to remove PCIE card! ========\n");
#endif
	}

	return err;
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

	printf("======== %s err=%d ========\n", __FUNCTION__, err);

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
	int err = 0;
	struct gpio_desc *gpiod_wl_reg_on = NULL;
#ifdef CUSTOMER_OOB
	struct gpio_desc *gpiod_wl_host_wake = NULL;
	int host_oob_irq = -1;
	uint host_oob_irq_flags = 0;
#ifdef CUSTOMER_HW_ROCKCHIP
#ifdef HW_OOB
	int irq_flags = -1;
#endif
#endif
#endif
	struct device *dev = NULL;

	/* Initialize legacy GPIO numbers to invalid */
	adapter->gpio_wl_reg_on = -1;
#ifdef CUSTOMER_OOB
	adapter->gpio_wl_host_wake = -1;
#endif

	/* Please check your schematic and fill right GPIO number which connected to
	* WL_REG_ON and WL_HOST_WAKE.
	*/
#ifdef BCMDHD_DTS
#ifdef BCMDHD_PLATDEV
	if (adapter->pdev) {
		dev = &adapter->pdev->dev;
		root_node = adapter->pdev->dev.of_node;
		if (root_node) {
			if (strscpy(wlan_node, root_node->name, sizeof(wlan_node)) < 0) {
				printf("%s: wlan_node name too long\n", __FUNCTION__);
				return -1;
			}
		} else {
			printf("%s: root_node is NULL\n", __FUNCTION__);
			return -1;
		}
	} else {
		printf("%s: adapter->pdev is NULL\n", __FUNCTION__);
		return -1;
	}
#else
	strscpy(wlan_node, DHD_DT_COMPAT_ENTRY, sizeof(wlan_node));
	root_node = of_find_compatible_node(NULL, NULL, wlan_node);
#endif
	printf("======== Get GPIO from DTS(%s) ========\n", wlan_node);
	
	/* Get GPIO descriptors using gpiod API */
	if (dev && root_node) {
		gpiod_wl_reg_on = devm_gpiod_get_optional(dev, "wl_reg_on", GPIOD_OUT_LOW);
		if (IS_ERR(gpiod_wl_reg_on)) {
			err = PTR_ERR(gpiod_wl_reg_on);
			printf("%s: Failed to get WL_REG_ON GPIO descriptor: %d\n", __FUNCTION__, err);
			gpiod_wl_reg_on = NULL;
		} else if (gpiod_wl_reg_on) {
			/* Set descriptor name for debugging */
			gpiod_set_consumer_name(gpiod_wl_reg_on, "WL_REG_ON");
			/* Store legacy GPIO number for compatibility */
			adapter->gpio_wl_reg_on = desc_to_gpio(gpiod_wl_reg_on);
		}
#ifdef CUSTOMER_OOB
		gpiod_wl_host_wake = devm_gpiod_get_optional(dev, "wl_host_wake", GPIOD_IN);
		if (IS_ERR(gpiod_wl_host_wake)) {
			err = PTR_ERR(gpiod_wl_host_wake);
			printf("%s: Failed to get WL_HOST_WAKE GPIO descriptor: %d\n", __FUNCTION__, err);
			gpiod_wl_host_wake = NULL;
		} else if (gpiod_wl_host_wake) {
			/* Set descriptor name for debugging */
			gpiod_set_consumer_name(gpiod_wl_host_wake, "WL_HOST_WAKE");
			/* Store legacy GPIO number for compatibility */
			adapter->gpio_wl_host_wake = desc_to_gpio(gpiod_wl_host_wake);
		}
#endif
	}
#endif /* BCMDHD_DTS */

	adapter->gpiod_wl_reg_on = gpiod_wl_reg_on;

#ifdef CUSTOMER_OOB
	adapter->gpiod_wl_host_wake = NULL;
	if (gpiod_wl_host_wake) {
		adapter->gpiod_wl_host_wake = gpiod_wl_host_wake;
		host_oob_irq = gpiod_to_irq(gpiod_wl_host_wake);
		if (host_oob_irq < 0) {
			printf("%s: gpiod_to_irq for WL_HOST_WAKE failed %d\n",
				__FUNCTION__, host_oob_irq);
			return -1;
		}
	}
/* No platform-specific override: use gpiod_to_irq() result when available */

#ifdef CUSTOMER_HW_ROCKCHIP
	host_oob_irq = rockchip_wifi_get_oob_irq();
#endif

#ifdef HW_OOB
#ifdef HW_OOB_LOW_LEVEL
	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL | IORESOURCE_IRQ_SHAREABLE;
#else
	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE;
#endif
#ifdef CUSTOMER_HW_ROCKCHIP
	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_SHAREABLE;
	irq_flags = rockchip_wifi_get_oob_irq_flag();
	if (irq_flags == 1)
		host_oob_irq_flags |= IORESOURCE_IRQ_HIGHLEVEL;
	else if (irq_flags == 0)
		host_oob_irq_flags |= IORESOURCE_IRQ_LOWLEVEL;
	else
		pr_warn("%s: unknown oob irqflags !\n", __func__);
#endif
#else
	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE | IORESOURCE_IRQ_SHAREABLE;
#endif
	host_oob_irq_flags &= IRQF_TRIGGER_MASK;

	adapter->irq_num = host_oob_irq;
	adapter->intr_flags = host_oob_irq_flags;
	printf("%s: WL_HOST_WAKE descriptor=%p, oob_irq=%d, oob_irq_flags=0x%x\n", __FUNCTION__,
		gpiod_wl_host_wake, host_oob_irq, host_oob_irq_flags);
#endif /* CUSTOMER_OOB */
	printf("%s: WL_REG_ON descriptor=%p\n", __FUNCTION__, gpiod_wl_reg_on);

	return 0;
}

static void
dhd_wlan_deinit_gpio(wifi_adapter_info_t *adapter)
{
	/* GPIOs obtained via devm_gpiod_get_optional are automatically freed
	 * when the device is unbound. We just need to clear our references.
	 */
	if (adapter->gpiod_wl_reg_on) {
		printf("%s: Clearing WL_REG_ON GPIO descriptor\n", __FUNCTION__);
		adapter->gpiod_wl_reg_on = NULL;
	}
	adapter->gpio_wl_reg_on = -1;

#ifdef CUSTOMER_OOB
	if (adapter->gpiod_wl_host_wake) {
		printf("%s: Clearing WL_HOST_WAKE GPIO descriptor\n", __FUNCTION__);
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
	printf("bus_type=%d, bus_num=%d, slot_num=%d\n",
		adapter->bus_type, adapter->bus_num, adapter->slot_num);
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

	if (strcmp(gpio_name, "wl_reg_on") == 0) {
		return adapter->gpio_wl_reg_on;
	}
#ifdef CUSTOMER_OOB
	else if (strcmp(gpio_name, "wl_host_wake") == 0) {
		return adapter->gpio_wl_host_wake;
	}
#endif
	
	return -ENOTSUPP;
}
