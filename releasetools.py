# MT6575 (vee5ss) OTA release tools override.
#
# The standard WriteRawImage("/boot") generates:
#   package_extract_file("boot.img", "/dev/bootimg")
# but /dev/bootimg does not exist in TWRP on MT6575. The boot partition
# lives at a raw offset on mmcblk0 (block 36864 = 0x1200000 bytes) with
# no named partition node. The package_extract_file call fails silently
# (no assert wrapper in the EMMC path), leaving the boot partition unchanged.
#
# FullOTA_InstallEnd runs after WriteRawImage and flashes boot.img via dd
# to the correct raw offset, overriding whatever the standard line did.

import os
import subprocess
import tempfile

import common


def _read_boot_arg(boot_dir, name):
    path = os.path.join(boot_dir, name)
    if not os.path.exists(path):
        return None
    f = open(path, "r")
    try:
        return f.read().strip()
    finally:
        f.close()


def ProcessBootImage(info, boot_img):
    boot_dir = os.path.join(info.input_tmp, "BOOT")
    kernel = os.path.join(boot_dir, "kernel")
    ramdisk = os.path.join(boot_dir, "ramdisk.img")

    if not os.path.exists(kernel) or not os.path.exists(ramdisk):
        return boot_img

    out_dir = tempfile.mkdtemp()
    out_name = os.path.join(out_dir, "boot.img")

    cmd = [
        "mtkbootimg",
        "--kernel", kernel,
        "--ramdisk", ramdisk,
        "--ramdisk_offset", "0x01000000",
        "--tags_offset", "0x00000100",
        "--mtk", "1",
        "--output", out_name,
    ]

    cmdline = _read_boot_arg(boot_dir, "cmdline")
    if cmdline:
        cmd.extend(["--cmdline", cmdline])

    base = _read_boot_arg(boot_dir, "base")
    if base:
        cmd.extend(["--base", base])

    pagesize = _read_boot_arg(boot_dir, "pagesize")
    if pagesize:
        cmd.extend(["--pagesize", pagesize])

    p = common.Run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    if p.returncode != 0:
        raise common.ExternalError(
            "mtkbootimg failed while rebuilding boot.img:\n%s%s" %
            (stdout, stderr))

    return common.File.FromLocalFile(boot_img.name, out_name)


def FullOTA_InstallEnd(info):
    info.script.AppendExtra('package_extract_file("boot.img", "/tmp/boot.img");')
    info.script.AppendExtra(
        'run_program("/sbin/sh", "-c",'
        ' "dd if=/tmp/boot.img of=/dev/block/mmcblk0 bs=512 seek=36864");')
    info.script.AppendExtra('delete("/tmp/boot.img");')
