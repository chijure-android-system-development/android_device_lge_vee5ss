PRODUCT_DEVICE := vee5ss
PRODUCT_NAME := omni_vee5ss
PRODUCT_BRAND := lge
PRODUCT_MODEL := LG-E450g
PRODUCT_MANUFACTURER := LGE

PRODUCT_COPY_FILES += \
    device/lge/vee5ss/ramdisk/init.recovery.mt6575.rc:root/init.recovery.mt6575.rc \
    device/lge/vee5ss/ramdisk/sbin/postrecoveryboot.sh:root/sbin/postrecoveryboot.sh

PRODUCT_PACKAGES += \
    adbd

# Recovery ADB defaults. ro.secure=0 and ro.debuggable=1 already come from
# the omni_vee5ss-eng build; these are the device-specific USB/ADB defaults.
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    ro.adb.secure=0 \
    persist.service.adb.enable=1 \
    service.adb.root=1
