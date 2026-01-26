#!/bin/bash
# Quick verification script: builds SDIO, PCIE and USB variants and writes logs to /tmp
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
ROOT_DIR="${SCRIPT_DIR}"
LINUXDIR=/home/ubuntu/repo/workspace/linux-6.18.3-build
LOGDIR=/tmp
FAIL=0

_build_variant() {
  VARIANT=$1
  CONFIG_ARG=$2
  LOGFILE="$LOGDIR/bcmdhd-modbuild-${VARIANT}.log"
  echo "== Building ${VARIANT} -> ${LOGFILE} =="

  # Determine parallel jobs
  JOBS=${MAKE_JOBS:-$(nproc)}

  if [ "${FAST:-0}" -eq 1 ]; then
    # Quick-build mode: only compile a small set of objects relevant to common prototype checks
    case "$VARIANT" in
      sdio)
        # limit quick targets to the per-port SDIO files to minimize deps
        TARGETS="dhd_sdio.o bcmsdh_sdmmc.o bcmsdh.o" ;;
      pcie)
        TARGETS="dhd_pcie.o dhd_msgbuf.o dhd_flowring.o" ;;
      usb)
        TARGETS="dhd_cdc.o" ;;
      *)
        TARGETS="" ;;
    esac
    if [ -n "$TARGETS" ]; then
      if make -C "$LINUXDIR" M="$ROOT_DIR" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- $TARGETS CONFIG_BCMDHD=m ${CONFIG_ARG} KBUILD_MODPOST_WARN=1 -j${JOBS} 2>&1 | tee "$LOGFILE"; then
        echo "${VARIANT} quick build: OK"
      else
        echo "${VARIANT} quick build: FAILED" >&2
        FAIL=1
      fi
    else
      echo "No quick targets for ${VARIANT}, skipping" | tee "$LOGFILE"
    fi
  else
    # Full module build (parallel)
    if make -C "$LINUXDIR" M="$ROOT_DIR" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules CONFIG_BCMDHD=m ${CONFIG_ARG} KBUILD_MODPOST_WARN=1 -j${JOBS} 2>&1 | tee "$LOGFILE"; then
      echo "${VARIANT} build: OK"
    else
      echo "${VARIANT} build: FAILED" >&2
      FAIL=1
    fi
  fi
}

_build_variant sdio "CONFIG_BCMDHD_SDIO=y"
_build_variant pcie "CONFIG_BCMDHD_PCIE=y"
_build_variant usb "CONFIG_BCMDHD_USB=y"

if [ "$FAIL" -ne 0 ]; then
  echo "One or more builds failed. See logs in $LOGDIR for details." >&2
  exit 1
fi

echo "All variants built successfully. Logs: $LOGDIR/bcmdhd-modbuild-*.log"
exit 0
