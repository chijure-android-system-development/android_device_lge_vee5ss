# ---- SoC / CPU (MediaTek MT6575) ----
TARGET_BOARD_PLATFORM := mt6575
TARGET_BOOTLOADER_BOARD_NAME := vee5ss
BOARD_VENDOR := lge

TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_VARIANT := cortex-a9
TARGET_ARCH_LOWMEM := true

TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true

# ---- Kernel prebuilt ----
TARGET_PREBUILT_KERNEL := device/lge/vee5ss/kernel

# MTK boot image: base=0x10000000 → kernel@0x10008000, ramdisk@0x11000000
BOARD_KERNEL_BASE := 0x10000000
BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_CMDLINE :=
BOARD_MKBOOTIMG_ARGS := --ramdisk_offset 0x01000000 --tags_offset 0x00000100

# mtkbootimg: custom mkbootimg que añade headers MTK al kernel y ramdisk
BOARD_CUSTOM_BOOTIMG := true
BOARD_CUSTOM_MKBOOTIMG := $(HOST_OUT_EXECUTABLES)/mtkbootimg$(HOST_EXECUTABLE_SUFFIX)
BOARD_CUSTOM_BOOTIMG_MK := device/lge/vee5ss/bootimg.mk

# Partition sizes (de /proc/dumchar_info del dispositivo)
BOARD_BOOTIMAGE_PARTITION_SIZE    := 6291456
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 7340032
BOARD_SYSTEMIMAGE_PARTITION_SIZE  := 1073741824
BOARD_USERDATAIMAGE_PARTITION_SIZE := 2216689664
BOARD_FLASH_BLOCK_SIZE := 131072

TARGET_USERIMAGES_USE_EXT4 := true

# Recovery
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_HAS_NO_MISC_PARTITION := true
BOARD_SUPPRESS_SECURE_ERASE := true
BOARD_SUPPRESS_EMMC_WIPE := true
BOARD_USES_MMCUTILS := true
BOARD_RECOVERY_SWIPE := true
TARGET_RECOVERY_FSTAB := device/lge/vee5ss/ramdisk/recovery/twrp.fstab

# /proc/cpuinfo's Serial field is all zeros on this MT6575, so TWRP's default
# device_id (used for the backup folder name) comes out as "0000000000000000".
# Use the product model instead (-> "LG-E450g"). In theory this should also
# append cpuinfo's Hardware field (-> "LG-E450g_MT6575"), but
# DataManager::get_device_id() in bootable/recovery/data.cpp opens
# "proc_cpuinfo.txt" (a relative path, not "/proc/cpuinfo") for that field —
# the fopen fails silently and the suffix never gets appended. Confirmed on
# the sibling vee4ss device tree (2026-07-23): the final name is just
# "LG-E440g", still clean — not worth patching TWRP's shared code for this.
TW_USE_MODEL_HARDWARE_ID_FOR_DEVICE_ID := true

# TWRP UI: pantalla 480x800 WVGA (lg4573ba_wvga, 32bpp framebuffer)
TW_THEME := portrait_hdpi
TARGET_RECOVERY_PIXEL_FORMAT := "RGBA_8888"

# Almacenamiento interno via datamedia (/data/media)
RECOVERY_SDCARD_ON_DATA := true
TW_INTERNAL_STORAGE_PATH := "/data/media"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
TW_EXTERNAL_STORAGE_PATH := "/external_sd"
TW_EXTERNAL_STORAGE_MOUNT_POINT := "external_sd"

TW_NO_EXFAT := true
TW_DEVICE_VERSION := 0.1
TW_INCLUDE_CRYPTO := false
TW_EXCLUDE_SUPERSU := true
TW_NO_REBOOT_BOOTLOADER := true

TW_BRIGHTNESS_PATH := "/sys/class/leds/lcd-backlight/brightness"
TW_MAX_BRIGHTNESS := 255
TW_DEFAULT_BRIGHTNESS := 128
TW_NO_CPU_TEMP := true

BOARD_HAS_LARGE_FILESYSTEM := true
BOARD_VOLD_MAX_PARTITIONS := 30

# Charger mode: MTK power-off charging detectado via androidboot.mode=charger
BOARD_GLOBAL_CFLAGS += -DCHARGERMODE_CMDLINE_NAME='"androidboot.mode"' -DCHARGERMODE_CMDLINE_VALUE='"charger"'
