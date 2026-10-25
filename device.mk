$(call inherit-product, device/lge/mt6575-common/mt6575-common.mk)

PRODUCT_DEVICE       := vee5ss
PRODUCT_NAME         := cm_vee5ss
PRODUCT_BRAND        := lge
PRODUCT_MODEL        := LG-E450g
PRODUCT_MANUFACTURER := LGE

# ---- Pantalla 480x800 WVGA, 240 dpi; 512 MB RAM ----
$(call inherit-product, frameworks/native/build/phone-hdpi-512-dalvik-heap.mk)

PRODUCT_AAPT_CONFIG      := normal hdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi

PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=240 \
    persist.sys.usb.config=mtp,adb

# ---- Init: init.mt6575.rc imports the common one ----
PRODUCT_COPY_FILES += \
    device/lge/vee5ss/rootdir/init.mt6575.rc:root/init.mt6575.rc \
    device/lge/vee5ss/rootdir/init.mt6575.usb.rc:root/init.mt6575.usb.rc

# ---- Media: MTK hardware codecs (common media_codecs.xml), 640x480 ----
# Each device picks its media_codecs.xml: a product's PRODUCT_COPY_FILES
# do not override an inherited one with the same destination.
PRODUCT_COPY_FILES += \
    device/lge/mt6575-common/rootdir/system/etc/media_codecs.xml:system/etc/media_codecs.xml \
    device/lge/vee5ss/media_profiles.xml:system/etc/media_profiles.xml

# ---- Blobs propios del E450g ----
PRODUCT_PACKAGES += \
    libcamalgo \
    libcameracustom \
    libcameraprofile

# ---- Codificadores y decodificadores de video por hardware MTK (omx/) ----
PRODUCT_PACKAGES += \
    libstagefrighthw \
    libstagefrighthw_mtk \
    libMtkOmxCore \
    libMtkOmxVenc \
    libMtkOmxVdec \
    libvcodecdrv \
    libvcodec_utility \
    libvcodec_oal \
    libstagefright_memutil \
    libsched \
    libh264dec_xa.ca9 \
    libh264dec_customize \
    libmp4dec_sa.ca9 \
    libmp4dec_customize \
    libvp8dec_xa.ca9 \
    libmp4enc_xa.ca9 \
    libh264enc_sa.ca9
