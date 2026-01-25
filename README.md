# Introduction

This repo provides dkms source code for wireless adapters based on the Broadcom ap6xxx chipset.

Forked from BCMDHD 101.10.591.52.27 and adapted for the Rockchip platform.

## Linux 6.18.3 Kernel Compatibility

This driver has been adapted for Linux kernel 6.18.3 with the following key changes:

### API Updates
- **netif_rx_ni() → netif_rx()**: Updated to use the modern netif_rx() API which became context-safe in kernel 5.18
- **strlcpy() → strscpy()**: Compatibility layer added for kernel 6.7+ where strlcpy was removed
- **do_gettimeofday()**: Uses ktime_get_real_ts64() for kernel 5.0+ via wrapper functions

### Performance Optimizations
- **GPIO Descriptor Caching**: GPIO descriptors are cached during initialization to avoid repeated device tree queries in IO paths
- **gpiod API**: Uses modern GPIO descriptor API (gpiod_*) instead of legacy integer GPIO numbers

### Stability Improvements
- **Platform Shutdown Hook**: Ensures WiFi chip is properly reset during system reboot by pulling WL_REG_ON low

### Deprecated Files
- `dhd_custom_hikey.c`: Board-specific file present but not compiled (not referenced in Makefile)
