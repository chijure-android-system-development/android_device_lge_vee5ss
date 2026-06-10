# TWRP device tree - LG Optimus L5 II

**Device**: LG-E450g  
**Codename**: vee5ss  
**SoC**: MediaTek MT6575  
**CPU**: ARM Cortex-A9  
**Android base**: 4.1.2 (JZO54K)  
**Stock product**: `lge/vee5ss_sca_com_g/vee5ss`

## Partitions

Partition data was taken from `/proc/dumchar_info` and checked against the
running device.

| Partition | Device | Size |
|-----------|--------|------|
| bootimg | `/dev/bootimg` | 6 MiB |
| recovery | `/dev/recovery` | 7 MiB |
| system | `/dev/block/mmcblk0p8` | 1 GiB |
| cache | `/dev/block/mmcblk0p9` | ~303 MiB |
| usrdata | `/dev/block/mmcblk0p10` | ~2 GiB |
| fat | `/dev/block/mmcblk0p11` | 10.5 MiB |
| external SD | `/dev/block/mmcblk1p1` | variable |

## Notes

- The prebuilt kernel was extracted from `CWM_6.0.4.4.img`.
- The kernel already contains the 512-byte MTK `KERNEL` header.
- Recovery images are packed with the local `mtkbootimg` host tool.
- `mtkbootimg --mtk 1` adds MTK headers only when missing, so the kernel header
  is not duplicated and the generated ramdisk gets a `RECOVERY` header.
- Boot image layout uses `base=0x10000000`, `pagesize=2048`,
  `ramdisk_offset=0x01000000`, and `tags_offset=0x00000100`.
- Internal storage is handled as `/data/media` with
  `RECOVERY_SDCARD_ON_DATA := true`; there is no separate `datamedia` fstab
  entry because this TWRP 4.4 tree does not parse `datamedia` as a filesystem.
- `TW_NO_REBOOT_BOOTLOADER := true` is set because the LG-E450g does not expose
  standard fastboot.

## Build

The Android 4.4 tree expects an old Java toolchain, so the known-good build path
is the Docker builder container.

```bash
docker exec twrp-4.4-builder bash -lc '
cd /home/builder/twrp-4.4 &&
source build/envsetup.sh &&
lunch omni_vee5ss-eng &&
make -j4 recoveryimage
'
```

Output:

```text
/home/builder/out/twrp-4.4/target/product/vee5ss/recovery.img
```

## Flash

The device does not support standard fastboot flashing. Use `dd` from Android
with root or from an existing recovery.

```bash
adb push recovery.img /sdcard/recovery_twrp.img
adb shell "su -c 'dd if=/dev/recovery of=/sdcard/recovery_backup_before_twrp.img bs=4096 count=1792; sync'"
adb shell "su -c 'dd if=/sdcard/recovery_twrp.img of=/dev/recovery bs=4096; sync'"
```

For host-side verification, read back the written byte count and compare:

```bash
adb shell "su -c 'dd if=/dev/recovery of=/sdcard/recovery_twrp_verify.img bs=4096 count=1592'"
adb pull /sdcard/recovery_twrp_verify.img
cmp recovery.img recovery_twrp_verify.img
```
