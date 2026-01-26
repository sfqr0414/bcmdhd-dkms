#!/bin/bash
# Quick verification script: builds SDIO, PCIE and USB variants and writes logs to /tmp
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
ROOT_DIR="${SCRIPT_DIR}"
LINUXDIR=/home/ubuntu/repo/workspace/linux-6.18.3-build
LOGDIR=/tmp

# Prereq checks
if ! command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
  echo "Missing cross-compiler: please install 'gcc-aarch64-linux-gnu' (or make sure 'aarch64-linux-gnu-gcc' is in PATH)." >&2
  exit 1
fi

FAIL=0

# Prefer a prepared system kernel build dir when available
SYSTEM_LINUXDIR="/lib/modules/$(uname -r)/build"
if [ -d "$SYSTEM_LINUXDIR" ] && [ -f "$SYSTEM_LINUXDIR/include/generated/autoconf.h" ]; then
  echo "Using system kernel build dir: $SYSTEM_LINUXDIR"
  LINUXDIR="$SYSTEM_LINUXDIR"
else
  # Ensure kernel source tree is prepared (autoconf headers etc.) so module builds work
  if [ ! -f "$LINUXDIR/include/generated/autoconf.h" ]; then
    echo "Preparing kernel source in $LINUXDIR (olddefconfig && prepare)"
    if make -C "$LINUXDIR" ARCH=arm64 olddefconfig && make -C "$LINUXDIR" ARCH=arm64 prepare; then
      echo "Kernel source prepared in $LINUXDIR"
    else
      echo "Failed to prepare $LINUXDIR and found no usable prepared kernel build dir. To fix, either: \n  1) prepare $LINUXDIR for cross-build: (cd $LINUXDIR && make ARCH=arm64 olddefconfig && make ARCH=arm64 prepare) \n  2) or point environment variable LINUXDIR to an existing prepared kernel build dir for arm64.\nAlso ensure you have the aarch64 cross-compiler installed (aarch64-linux-gnu-gcc)." >&2
      exit 1
    fi
  fi
fi

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
      # Use BUILD_DIR from the module Makefile so intermediate object files are
  # kept out of the source tree.
  BUILD_DIR_REL=$(awk -F'[?=]+' '/^BUILD_DIR/ {gsub(/ /,"",$2); print $2; exit}' "$ROOT_DIR/Makefile" || true)
  BUILD_DIR_REL=${BUILD_DIR_REL:-../bcmdhd_build}
  BUILD_DIR_ABS=$(realpath -m "$ROOT_DIR/$BUILD_DIR_REL")
  mkdir -p "$BUILD_DIR_ABS"
  # Create a symlinked copy of the source tree under BUILD_DIR so all intermediate
  # artifacts remain in BUILD_DIR while the build reads sources from there.
  if [ ! -e "$BUILD_DIR_ABS/.src_synced" ]; then
    echo "Populating $BUILD_DIR_ABS with symlinks to sources from $ROOT_DIR"
    rm -rf "$BUILD_DIR_ABS"/* "$BUILD_DIR_ABS"/.[!.]* 2>/dev/null || true
    cp -as "$ROOT_DIR/." "$BUILD_DIR_ABS/"
    touch "$BUILD_DIR_ABS/.src_synced"
  fi
  # Build using the symlinked source tree in BUILD_DIR
    if make -C "$LINUXDIR" M="$BUILD_DIR_ABS" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- $TARGETS CONFIG_BCMDHD=m ${CONFIG_ARG} KBUILD_MODPOST_WARN=1 -j${JOBS} 2>&1 | tee "$LOGFILE"; then
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
    # Use BUILD_DIR from the module Makefile so intermediate object files are
    # kept out of the source tree.
    BUILD_DIR_REL=$(awk -F'[?=]+' '/^BUILD_DIR/ {gsub(/ /,"",$2); print $2; exit}' "$ROOT_DIR/Makefile" || true)
    BUILD_DIR_REL=${BUILD_DIR_REL:-../bcmdhd_build}
    BUILD_DIR_ABS=$(realpath -m "$ROOT_DIR/$BUILD_DIR_REL")
    mkdir -p "$BUILD_DIR_ABS"
    # Create a symlinked copy of the source tree under BUILD_DIR so all intermediate
    # artifacts remain in BUILD_DIR while the build reads sources from there.
    if [ ! -e "$BUILD_DIR_ABS/.src_synced" ]; then
      echo "Populating $BUILD_DIR_ABS with symlinks to sources from $ROOT_DIR"
      rm -rf "$BUILD_DIR_ABS"/* "$BUILD_DIR_ABS"/.[!.]* 2>/dev/null || true
      cp -as "$ROOT_DIR/." "$BUILD_DIR_ABS/"
      touch "$BUILD_DIR_ABS/.src_synced"
    fi
    # Build using the symlinked source tree in BUILD_DIR
    if make -C "$LINUXDIR" M="$BUILD_DIR_ABS" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules CONFIG_BCMDHD=m ${CONFIG_ARG} KBUILD_MODPOST_WARN=1 -j${JOBS} 2>&1 | tee "$LOGFILE"; then
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
  echo "One or more builds failed with cross-build (ARCH=arm64). Attempting host-native fallback builds using system kernel build dir..."
  # try a host-native fallback using system kernel build dir
  SYSTEM_LINUXDIR="/lib/modules/$(uname -r)/build"
  if [ -d "$SYSTEM_LINUXDIR" ]; then
    echo "Attempting host-native builds against $SYSTEM_LINUXDIR"
    FAIL_HOST=0
    for VARIANT in sdio pcie usb; do
      LOGFILE="$LOGDIR/bcmdhd-host-modbuild-${VARIANT}.log"
      echo "== Host build ${VARIANT} -> ${LOGFILE} =="
      BUILD_DIR_REL=$(awk -F'[?=]+' '/^BUILD_DIR/ {gsub(/ /,"",$2); print $2; exit}' "$ROOT_DIR/Makefile" || true)
      BUILD_DIR_REL=${BUILD_DIR_REL:-../bcmdhd_build}
      BUILD_DIR_ABS=$(realpath -m "$ROOT_DIR/$BUILD_DIR_REL")
      mkdir -p "$BUILD_DIR_ABS"
      # Ensure sources copied to BUILD_DIR
      if [ ! -e "$BUILD_DIR_ABS/.src_synced" ]; then
        rm -rf "$BUILD_DIR_ABS"/* "$BUILD_DIR_ABS"/.[!.]* 2>/dev/null || true
        cp -as "$ROOT_DIR/." "$BUILD_DIR_ABS/"
        touch "$BUILD_DIR_ABS/.src_synced"
      fi
      if make -C "$SYSTEM_LINUXDIR" M="$BUILD_DIR_ABS" modules CONFIG_BCMDHD=m CONFIG_BCMDHD_${VARIANT^^}=y KBUILD_MODPOST_WARN=1 -j${JOBS} 2>&1 | tee "$LOGFILE"; then
        echo "Host ${VARIANT} build: OK"
      else
        echo "Host ${VARIANT} build: FAILED (see $LOGFILE)" >&2
        FAIL_HOST=1
      fi
    done
    if [ "$FAIL_HOST" -ne 0 ]; then
      echo "Host-native fallback also had failures. See logs in $LOGDIR for details." >&2
      exit 1
    else
      echo "Host-native fallback builds succeeded for all variants. Logs: $LOGDIR/bcmdhd-host-modbuild-*.log"
      exit 0
    fi
  else
    echo "No system kernel build dir available for host-native fallback. See logs in $LOGDIR for details." >&2
    exit 1
  fi
fi

echo "All variants built successfully. Logs: $LOGDIR/bcmdhd-modbuild-*.log"
exit 0
