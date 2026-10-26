#!/sbin/sh
# The LG bootloader passes no androidboot.serialno, so ro.serialno is empty
# and the USB gadget keeps the kernel's 0123456789ABCDEF: both phones showed
# up in adb with the same serial. Use the serial LG's stock init reads, the
# same one CM and stock report, then bring up adb.
serial=$(cat /sys/sys_info/serial_number 2>/dev/null)
[ -n "$serial" ] && echo "$serial" > /sys/class/android_usb/android0/iSerial
setprop sys.usb.config adb
# TWRP's init may already have enumerated USB with the old serial and
# started adbd; restart it so the host takes the new one instead of
# showing the phone offline.
sleep 1
stop adbd
start adbd
