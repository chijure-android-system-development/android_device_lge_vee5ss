include device/lge/mt6575-common/BoardConfigCommon.mk

TARGET_BOOTLOADER_BOARD_NAME := vee5ss

# ---- Kernel prebuilt ----
TARGET_PREBUILT_KERNEL := device/lge/vee5ss/kernel

# ---- Particiones (/proc/dumchar_info) ----
BOARD_BOOTIMAGE_PARTITION_SIZE     := 6291456
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 7340032
BOARD_SYSTEMIMAGE_PARTITION_SIZE   := 1073741824
BOARD_USERDATAIMAGE_PARTITION_SIZE := 2216689664

# ---- Recovery ----
TARGET_RECOVERY_FSTAB := device/lge/vee5ss/rootdir/recovery.fstab

# ---- OTA assert ----
TARGET_OTA_ASSERT_DEVICE := vee5ss,E450g,LG-E450g
