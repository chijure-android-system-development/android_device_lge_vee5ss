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

BOARD_KERNEL_BASE    := 0x10000000
BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_CMDLINE := androidboot.hardware=mt6575
BOARD_MKBOOTIMG_ARGS := --ramdisk_offset 0x01000000 --tags_offset 0x00000100

# mtkbootimg: genera headers MTK antes del kernel y ramdisk
BOARD_CUSTOM_BOOTIMG    := true
BOARD_CUSTOM_MKBOOTIMG  := $(HOST_OUT_EXECUTABLES)/mtkbootimg$(HOST_EXECUTABLE_SUFFIX)
BOARD_CUSTOM_BOOTIMG_MK := device/lge/vee5ss/bootimg.mk

# ---- Particiones (/proc/dumchar_info) ----
BOARD_BOOTIMAGE_PARTITION_SIZE     := 6291456
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 7340032
BOARD_SYSTEMIMAGE_PARTITION_SIZE   := 1073741824
BOARD_USERDATAIMAGE_PARTITION_SIZE := 2216689664
BOARD_FLASH_BLOCK_SIZE             := 131072

TARGET_USERIMAGES_USE_EXT4 := true

# ---- Recovery ----
TARGET_RECOVERY_FSTAB := device/lge/vee5ss/rootdir/recovery.fstab
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_HAS_NO_MISC_PARTITION := true
BOARD_SUPPRESS_SECURE_ERASE := true
BOARD_SUPPRESS_EMMC_WIPE := true
BOARD_USES_MMCUTILS := true
TARGET_RECOVERY_PIXEL_FORMAT := "RGBA_8888"

# ---- OTA assert ----
TARGET_OTA_ASSERT_DEVICE := vee5ss,E450g,LG-E450g

# ---- ADB root (útil durante el desarrollo del port) ----
ADDITIONAL_DEFAULT_PROPERTIES += ro.secure=0

# ---- Charger mode (MTK detecta vía androidboot.mode=charger) ----
BOARD_GLOBAL_CFLAGS += -DCHARGERMODE_CMDLINE_NAME='"androidboot.mode"' \
                       -DCHARGERMODE_CMDLINE_VALUE='"charger"'

# ---- Filesystem ----
BOARD_HAS_LARGE_FILESYSTEM := true
BOARD_VOLD_MAX_PARTITIONS   := 30
