#!/bin/bash
# Aplica parches específicos de vee5ss (MT6620) al árbol CM10.
# Ejecutar desde la raíz del árbol antes de compilar:
#   . device/lge/vee5ss/patch-rom.sh
#
# Los parches son idempotentes: si ya están aplicados, se omiten.

DEVICE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROM_ROOT="$(cd "$DEVICE_DIR/../../.." && pwd)"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

apply_patch() {
    local repo_path="$1"       # relativo a ROM_ROOT
    local patch_file="$2"      # ruta absoluta al .patch
    local description="$3"

    local abs_repo="$ROM_ROOT/$repo_path"

    if ! git -C "$abs_repo" apply --check "$patch_file" 2>/dev/null; then
        echo -e "${YELLOW}[SKIP]${NC} $description (ya aplicado o no aplica)"
        return 0
    fi

    if git -C "$abs_repo" apply "$patch_file"; then
        echo -e "${GREEN}[OK]${NC}   $description"
    else
        echo -e "${RED}[FAIL]${NC} $description"
        return 1
    fi
}

echo "=== vee5ss: aplicando parches MT6620 ==="

apply_patch "frameworks/base" \
    "$DEVICE_DIR/patches/frameworks_base/0001-BluetoothA2dp-fix-EventLoop-PopLocalFrame-on-EINTR.patch" \
    "BluetoothA2dpService: fix A2DP EventLoop crash on SIGQUIT (EINTR + PopLocalFrame)"

apply_patch "packages/apps/Bluetooth" \
    "$DEVICE_DIR/patches/packages_apps_Bluetooth/0001-BluetoothMasService-break-on-accept-IOException.patch" \
    "BluetoothMasService: break on accept() IOException (previene spin loop)"

apply_patch "packages/apps/Bluetooth" \
    "$DEVICE_DIR/patches/packages_apps_Bluetooth/0002-BluetoothOppRfcommListener-break-on-accept-IOException.patch" \
    "BluetoothOppRfcommListener: break on accept() IOException (previene spin loop)"

apply_patch "frameworks/base" \
    "$DEVICE_DIR/patches/frameworks_base/0002-UsbDeviceManager-skip-RNDIS-ethaddr-if-no-f_rndis.patch" \
    "UsbDeviceManager: skip RNDIS ethaddr write if f_rndis sysfs node absent"

apply_patch "packages/apps/Camera" \
    "$DEVICE_DIR/patches/packages_apps_Camera/0001-DisableCameraReceiver-retry-on-zero-cameras.patch" \
    "DisableCameraReceiver: retry getNumberOfCameras if HAL not ready at BOOT_COMPLETED"

apply_patch "packages/apps/Torch" \
    "$DEVICE_DIR/patches/packages_apps_Torch/0001-TorchService-do-not-restart-after-stop.patch" \
    "TorchService: do not restart after user stops flashlight"

echo "=== listo ==="
