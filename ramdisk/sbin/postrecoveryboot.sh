#!/sbin/sh
# Stop kernel LED blink trigger on button-backlight (MT6575 sets timer by default)
echo none  > /sys/class/leds/button-backlight/trigger
echo 0     > /sys/class/leds/button-backlight/brightness
