/*
 * lights.c — Lights HAL for LG E450g (LP5521 RGB LED controller)
 *
 * Uses the LP5521 blink node directly, matching LG's stock HAL behavior.
 */
#define LOG_TAG "lights"

#include <cutils/log.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <hardware/lights.h>

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

#define LCD_BL  "/sys/class/leds/lcd-backlight/brightness"
#define BTN_BL  "/sys/class/leds/button-backlight/brightness"
#define R_BRT   "/sys/class/leds/R/brightness"
#define G_BRT   "/sys/class/leds/G/brightness"
#define B_BRT   "/sys/class/leds/B/brightness"
#define LP5521_BLINK   "/sys/bus/i2c/drivers/lp5521/1-0032/led_blink"
#define LP5521_PATTERN "/sys/bus/i2c/drivers/lp5521/1-0032/led_pattern"

#define LP5521_PATTERN_OFF             0
#define LP5521_PATTERN_POWER_ON        1
#define LP5521_PATTERN_LCD_ON          2
#define LP5521_PATTERN_CHARGING_LOW    3
#define LP5521_PATTERN_CHARGING_FULL   4
#define LP5521_PATTERN_CHARGING        5
#define LP5521_PATTERN_POWER_OFF       6
#define LP5521_PATTERN_MISSED_NOTI     7
#define LP5521_PATTERN_FAVORITE_NOTI   14

typedef struct {
    int color;
    int flashMode;
    int onMs;
    int offMs;
} rgb_state_t;

static rgb_state_t g_battery_state;
static rgb_state_t g_notification_state;
static rgb_state_t g_attention_state;
static int g_lcd_on = 1;

static void apply_rgb_locked(void);

static int write_int(const char *path, int val)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", val);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        ALOGE("open %s: %s", path, strerror(errno));
        return -1;
    }
    ssize_t n = write(fd, buf, strlen(buf));
    close(fd);
    return (n < 0) ? -1 : 0;
}

static int write_str(const char *path, const char *val)
{
    char buf[48];
    int len;
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        ALOGE("open %s: %s", path, strerror(errno));
        return -1;
    }
    len = snprintf(buf, sizeof(buf), "%s\n", val);
    ssize_t n = write(fd, buf, len);
    close(fd);
    return (n < 0) ? -1 : 0;
}

static void rgb_off(void)
{
    write_str(LP5521_BLINK, "0");
    write_str(LP5521_PATTERN, "0");
    write_int(R_BRT, 0);
    write_int(G_BRT, 0);
    write_int(B_BRT, 0);
}

static void rgb_solid(int r, int g, int b)
{
    write_str(LP5521_BLINK, "0");
    write_str(LP5521_PATTERN, "0");
    write_int(R_BRT, r);
    write_int(G_BRT, g);
    write_int(B_BRT, b);
}

static void rgb_blink(int color, int r, int g, int b, int onMs, int offMs)
{
    char pattern[40];

    rgb_off();
    snprintf(pattern, sizeof(pattern), "0x%08x %d %d", color, onMs, offMs);
    write_str(LP5521_BLINK, pattern);
}

static void rgb_pattern(int pattern)
{
    write_str(LP5521_BLINK, "0");
    write_int(R_BRT, 0);
    write_int(G_BRT, 0);
    write_int(B_BRT, 0);
    write_int(LP5521_PATTERN, pattern);
}

static int stock_battery_pattern(int r, int g, int b)
{
    if (r > g && r > b)
        return LP5521_PATTERN_CHARGING_LOW;

    if (g > 0 && r == 0 && b == 0)
        return LP5521_PATTERN_CHARGING;

    return LP5521_PATTERN_CHARGING;
}

static int is_green_notification(int r, int g, int b)
{
    return g > 0 && g >= r && g >= b;
}

static int set_light_backlight(struct light_device_t *dev,
        const struct light_state_t *state)
{
    int brt = (state->color >> 16) & 0xFF;
    int ret = write_int(LCD_BL, brt);

    pthread_mutex_lock(&g_lock);
    g_lcd_on = brt > 0;
    apply_rgb_locked();
    pthread_mutex_unlock(&g_lock);

    return ret;
}

static int set_light_buttons(struct light_device_t *dev,
        const struct light_state_t *state)
{
    return write_int(BTN_BL, (state->color & 0xFFFFFF) ? 255 : 0);
}

static int rgb_state_is_lit(const rgb_state_t *state)
{
    return (state->color & 0x00FFFFFF) != 0;
}

static void apply_rgb_locked(void)
{
    const rgb_state_t *state;
    int color;
    int r, g, b;
    int onMs, offMs;

    if (rgb_state_is_lit(&g_attention_state)) {
        state = &g_attention_state;
    } else if (rgb_state_is_lit(&g_notification_state)) {
        state = &g_notification_state;
    } else if (!g_lcd_on && rgb_state_is_lit(&g_battery_state)) {
        state = &g_battery_state;
    } else {
        state = NULL;
    }

    color = state ? state->color : 0;
    r = state ? ((state->color >> 16) & 0xFF) : 0;
    g = state ? ((state->color >> 8)  & 0xFF) : 0;
    b = state ? ( state->color        & 0xFF) : 0;
    onMs  = (state && state->flashMode == LIGHT_FLASH_TIMED) ? state->onMs  : 0;
    offMs = (state && state->flashMode == LIGHT_FLASH_TIMED) ? state->offMs : 0;

    ALOGD("apply_rgb lcd_on=%d r=%d g=%d b=%d on=%d off=%d",
            g_lcd_on, r, g, b, onMs, offMs);

    if (state == &g_battery_state && !g_lcd_on) {
        int pattern = stock_battery_pattern(r, g, b);
        ALOGD("apply_rgb stock battery pattern=%d", pattern);
        rgb_pattern(pattern);
        return;
    }

    if (state == &g_attention_state) {
        ALOGD("apply_rgb stock attention pattern=%d", LP5521_PATTERN_FAVORITE_NOTI);
        rgb_pattern(LP5521_PATTERN_FAVORITE_NOTI);
        return;
    }

    if (state == &g_notification_state && is_green_notification(r, g, b)) {
        ALOGD("apply_rgb stock notification pattern=%d", LP5521_PATTERN_MISSED_NOTI);
        rgb_pattern(LP5521_PATTERN_MISSED_NOTI);
        return;
    }

    if (!r && !g && !b) {
        rgb_off();
    } else if (onMs > 0 && offMs > 0) {
        rgb_blink(color, r, g, b, onMs, offMs);
    } else {
        rgb_solid(r, g, b);
    }
}

static int set_light_rgb_state(rgb_state_t *dst, const char *name,
        const struct light_state_t *state)
{
    ALOGD("%s color=%08x flash=%d on=%d off=%d",
            name, state->color, state->flashMode, state->flashOnMS, state->flashOffMS);

    pthread_mutex_lock(&g_lock);
    dst->color = state->color;
    dst->flashMode = state->flashMode;
    dst->onMs = state->flashOnMS;
    dst->offMs = state->flashOffMS;
    apply_rgb_locked();
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int set_light_battery(struct light_device_t *dev,
        const struct light_state_t *state)
{
    return set_light_rgb_state(&g_battery_state, "battery", state);
}

static int set_light_notifications(struct light_device_t *dev,
        const struct light_state_t *state)
{
    return set_light_rgb_state(&g_notification_state, "notifications", state);
}

static int set_light_attention(struct light_device_t *dev,
        const struct light_state_t *state)
{
    return set_light_rgb_state(&g_attention_state, "attention", state);
}

static int open_lights(const struct hw_module_t *module, const char *name,
        struct hw_device_t **device)
{
    int (*fn)(struct light_device_t *, const struct light_state_t *);

    if      (!strcmp(name, LIGHT_ID_BACKLIGHT))     fn = set_light_backlight;
    else if (!strcmp(name, LIGHT_ID_BUTTONS))       fn = set_light_buttons;
    else if (!strcmp(name, LIGHT_ID_BATTERY))       fn = set_light_battery;
    else if (!strcmp(name, LIGHT_ID_NOTIFICATIONS)) fn = set_light_notifications;
    else if (!strcmp(name, "lg_notifications"))    fn = set_light_notifications;
    else if (!strcmp(name, LIGHT_ID_ATTENTION))     fn = set_light_attention;
    else return -EINVAL;

    struct light_device_t *dev = calloc(1, sizeof(*dev));
    if (!dev) return -ENOMEM;

    dev->common.tag     = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module  = (struct hw_module_t *)module;
    dev->common.close   = (int (*)(struct hw_device_t *))free;
    dev->set_light      = fn;

    *device = (struct hw_device_t *)dev;
    return 0;
}

static struct hw_module_methods_t lights_methods = {
    .open = open_lights,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag           = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id            = LIGHTS_HARDWARE_MODULE_ID,
    .name          = "vee5ss LP5521 lights HAL",
    .author        = "CM10 vee5ss",
    .methods       = &lights_methods,
};
