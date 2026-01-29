# bcmdhd Driver Modifications for Linux 6.18.3

## Quick Summary

This PR modifies the bcmdhd wireless driver to support Linux 6.18.3 with the following key improvements:

1. **Independent WiFi DT Node Support**: WiFi node can be placed at root level without parent dependency
2. **Automatic MMC/SDIO Controller Matching**: Auto-detects RK3588 WiFi SDIO controller by DT characteristics
3. **mmc-pwrseq Compatible GPIO Handling**: WL_REG_ON GPIO parsed with GPIOD_ASIS, controlled via sysfs
4. **Functional SDIO Card Detection**: Actually calls sdhci_force_presence_change instead of just logging

## Files Modified

- `src/dhd_gpio.c` - GPIO hardware control layer
- `src/dhd_linux_platdev.c` - Platform device and MMC matching layer  
- `src/dhd_plat.h` - Function signature updates
- `src/dhd_linux.h` - Structure member restoration
- `src/bcmsdh.c` - GPIO caching preservation

## Key Technical Changes

### dhd_gpio.c
- Sysfs-based GPIO control with retry mechanism
- GPIOD_ASIS parsing (no request, no conflict)
- Updated set_power/set_reset with adapter parameter
- Removed mm_segment_t (deprecated in kernel 5.10+)

### dhd_linux_platdev.c
- MMC controller auto-matching by DT properties
- Implemented dhd_wlan_set_carddetect with actual sdhci call
- Support for standalone WiFi DT nodes
- Removed parent node dependencies

## Compilation

```bash
cd src
BUILD_DIR=/tmp/bcmdhd_build make
```

## Testing Requirements

1. Device tree must have WiFi node with `compatible = "android,bcmdhd_wlan"`
2. SDIO controller must have: cap-sdio-irq, no-sd, no-mmc, bus-width=4
3. WL_REG_ON GPIO must be defined in WiFi node

## Compatibility

- **Kernel**: Linux 4.5+ (primary target: 6.18.3)
- **Architecture**: arm64 (RK3588 focus)
- **Bus**: SDIO (BCMSDIO mode)
- **DT**: Required (BCMDHD_DTS mode)

## Documentation

See `/tmp/MODIFICATIONS_SUMMARY.md` for detailed technical documentation.

## Status

✅ All modifications complete
✅ Compilation tested
⏳ Runtime testing pending on target hardware
