$(call inherit-product, device/lge/vee5ss/device.mk)
$(call inherit-product-if-exists, vendor/lge/vee5ss/vee5ss-vendor-blobs.mk)
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

PRODUCT_NAME     := cm_vee5ss
PRODUCT_DEVICE   := vee5ss
PRODUCT_BRAND    := lge
PRODUCT_MODEL    := LG-E450g
PRODUCT_MANUFACTURER := LGE
PRODUCT_RELEASE_NAME := L5 II

PRODUCT_BUILD_PROP_OVERRIDES += \
    BUILD_FINGERPRINT="lge/vee5ss/vee5ss:4.1.2/JZO54K/E450g10b:user/release-keys" \
    PRIVATE_BUILD_DESC="vee5ss-user 4.1.2 JZO54K E450g10b release-keys" \
    BUILD_NUMBER=E450g10b
