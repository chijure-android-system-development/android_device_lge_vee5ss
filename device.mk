PRODUCT_DEVICE := vee5ss
PRODUCT_NAME := omni_vee5ss
PRODUCT_BRAND := lge
PRODUCT_MODEL := LG-E450g
PRODUCT_MANUFACTURER := LGE

PRODUCT_COPY_FILES += \
    device/lge/vee5ss/ramdisk/init.recovery.mt6575.rc:root/init.recovery.mt6575.rc \
    device/lge/vee5ss/ramdisk/sbin/postrecoveryboot.sh:root/sbin/postrecoveryboot.sh \
    device/lge/vee5ss/ramdisk/sbin/postscreenblank.sh:root/sbin/postscreenblank.sh \
    device/lge/vee5ss/ramdisk/sbin/postscreenunblank.sh:root/sbin/postscreenunblank.sh
