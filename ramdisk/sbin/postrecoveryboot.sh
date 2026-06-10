#!/sbin/sh
# Disable button-backlight blink (MT6575 kernel sets timer trigger by default)
echo none > /sys/class/leds/button-backlight/trigger
echo 0    > /sys/class/leds/button-backlight/brightness
# Reset LP5521 engine state (Android leaves engines latched; load→disabled clears it)
for e in engine1_mode engine2_mode engine3_mode; do
  echo load     > /sys/class/leds/G/device/$e
  echo disabled > /sys/class/leds/G/device/$e
done
echo 0 > /sys/class/leds/R/brightness
echo 0 > /sys/class/leds/G/brightness
echo 0 > /sys/class/leds/B/brightness
