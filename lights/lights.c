/*
 * lights.c — Lights HAL for LG E450g (LP5521 RGB LED controller)
 *
 * Uses userspace blink thread instead of kernel timer trigger to avoid:
 *   1. Permission denied on leds/R|G|B/trigger (not chowned in init.rc)
 *   2. delay_on/delay_off appearing only after trigger=timer (dynamic sysfs)
 *   3. Kernel blink_brightness persisting after brightness=0 (stock HAL bug)
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
static pthread_t       g_blink_thread;
static volatile int    g_blink_active = 0;

#define LCD_BL  "/sys/class/leds/lcd-backlight/brightness"
#define BTN_BL  "/sys/class/leds/button-backlight/brightness"
#define R_BRT   "/sys/class/leds/R/brightness"
#define G_BRT   "/sys/class/leds/G/brightness"
#define B_BRT   "/sys/class/leds/B/brightness"

typedef struct {
    int r, g, b;
    int onMs, offMs;
} blink_args_t;

static blink_args_t g_blink_args;

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

static void rgb_off(void)
{
    write_int(R_BRT, 0);
    write_int(G_BRT, 0);
    write_int(B_BRT, 0);
}

static void rgb_solid(int r, int g, int b)
{
    write_int(R_BRT, r);
    write_int(G_BRT, g);
    write_int(B_BRT, b);
}

static void *blink_thread_fn(void *arg)
{
    blink_args_t a = *(blink_args_t *)arg;
    while (g_blink_active) {
        rgb_solid(a.r, a.g, a.b);
        usleep((useconds_t)a.onMs * 1000);
        if (!g_blink_active) break;
        rgb_off();
        usleep((useconds_t)a.offMs * 1000);
    }
    rgb_off();
    return NULL;
}

static void stop_blink(void)
{
    if (g_blink_active) {
        g_blink_active = 0;
        pthread_join(g_blink_thread, NULL);
    }
}

static void start_blink(int r, int g, int b, int onMs, int offMs)
{
    stop_blink();
    g_blink_args = (blink_args_t){ r, g, b, onMs, offMs };
    g_blink_active = 1;
    pthread_create(&g_blink_thread, NULL, blink_thread_fn, &g_blink_args);
}

static int set_light_backlight(struct light_device_t *dev,
        const struct light_state_t *state)
{
    int brt = (state->color >> 16) & 0xFF;
    return write_int(LCD_BL, brt);
}

static int set_light_buttons(struct light_device_t *dev,
        const struct light_state_t *state)
{
    return write_int(BTN_BL, (state->color & 0xFFFFFF) ? 255 : 0);
}

static int set_light_rgb(struct light_device_t *dev,
        const struct light_state_t *state)
{
    int r = (state->color >> 16) & 0xFF;
    int g = (state->color >> 8)  & 0xFF;
    int b =  state->color        & 0xFF;
    int onMs  = (state->flashMode == LIGHT_FLASH_TIMED) ? state->flashOnMS  : 0;
    int offMs = (state->flashMode == LIGHT_FLASH_TIMED) ? state->flashOffMS : 0;

    ALOGD("set_light_rgb r=%d g=%d b=%d on=%d off=%d", r, g, b, onMs, offMs);

    pthread_mutex_lock(&g_lock);
    stop_blink();
    if (!r && !g && !b) {
        rgb_off();
    } else if (onMs > 0 && offMs > 0) {
        start_blink(r, g, b, onMs, offMs);
    } else {
        rgb_solid(r, g, b);
    }
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int open_lights(const struct hw_module_t *module, const char *name,
        struct hw_device_t **device)
{
    int (*fn)(struct light_device_t *, const struct light_state_t *);

    if      (!strcmp(name, LIGHT_ID_BACKLIGHT))     fn = set_light_backlight;
    else if (!strcmp(name, LIGHT_ID_BUTTONS))       fn = set_light_buttons;
    else if (!strcmp(name, LIGHT_ID_BATTERY))       fn = set_light_rgb;
    else if (!strcmp(name, LIGHT_ID_NOTIFICATIONS)) fn = set_light_rgb;
    else if (!strcmp(name, LIGHT_ID_ATTENTION))     fn = set_light_rgb;
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
