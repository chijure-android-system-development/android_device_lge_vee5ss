# Full Android base platform (includes core.mk + fonts + librs_jni + audio HALs + keyboards)
$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_no_telephony.mk)

# Runtime libraries needed for Android boot (not in generic_no_telephony.mk)
PRODUCT_PACKAGES += \
    libkeystore_client \
    sensorservice

PRODUCT_DEVICE       := vee5ss
PRODUCT_NAME         := cm_vee5ss
PRODUCT_BRAND        := lge
PRODUCT_MODEL        := LG-E450g
PRODUCT_MANUFACTURER := LGE

# ---- Dalvik heap para dispositivo hdpi 512 MB ----
$(call inherit-product, frameworks/native/build/phone-hdpi-512-dalvik-heap.mk)

# ---- AAPT ----
PRODUCT_AAPT_CONFIG      := normal hdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi

# ---- Init y ramdisk ----
PRODUCT_COPY_FILES += \
    device/lge/vee5ss/rootdir/fstab.mt6575:root/fstab.mt6575 \
    device/lge/vee5ss/rootdir/init.mt6575.rc:root/init.mt6575.rc \
    device/lge/vee5ss/rootdir/init.mt6575.usb.rc:root/init.mt6575.usb.rc \
    device/lge/vee5ss/rootdir/ueventd.mt6575.rc:root/ueventd.mt6575.rc \
    system/core/rootdir/init.usb.rc:root/init.usb.rc \
    system/core/rootdir/init.trace.rc:root/init.trace.rc

# ---- Permisos de hardware ----
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml

# ---- Propiedades del sistema ----
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=240 \
    ro.opengles.version=131072 \
    wifi.interface=wlan0 \
    rild.libpath=/system/lib/mtk-ril.so \
    telephony.lteOnGsmDevice=0 \
    ro.com.android.dataroaming=false \
    ro.config.low_ram=true \
    config.disable_atlas=true \
    persist.service.adb.enable=1 \
    persist.sys.usb.config=mtp,adb

PRODUCT_TAGS += dalvik.gc.type-precise

# ---- Paquetes ----
PRODUCT_PACKAGES += \
    lights.mt6575 \
    power.mt6575 \
    rild \
    libril
