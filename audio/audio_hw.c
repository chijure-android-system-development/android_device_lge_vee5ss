/*
 * MT6575 (vee5ss) primary audio HAL.
 *
 * The MT6575 exposes audio through /dev/eac (misc char device, major 10).
 * There are no standard ALSA cards. The stock HAL uses /dev/eac with
 * proprietary ioctls for routing and mode control, and plain read()/write()
 * for PCM data transfer.
 *
 * This HAL opens /dev/eac for input (recording) and output (playback).
 * Without the proprietary ioctl initialization, the mic may be inactive;
 * in that case in_read returns zeros. The primary goal of this module is
 * to allow AudioFlinger and AudioPolicyService to start cleanly so higher-
 * level audio routing is functional.
 *
 * ro.hardware=mt6575 → AudioFlinger loads audio.primary.mt6575.so first,
 * before the intentionally-broken audio.primary.default.so stub.
 */

#define LOG_TAG "audio_hw_mt6575"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cutils/log.h>
#include <hardware/audio.h>
#include <hardware/hardware.h>

#define EAC_DEV      "/dev/eac"
#define OUT_RATE     44100
#define OUT_CHANNELS 2
#define OUT_FRAME_SZ 4   /* 16-bit stereo */

#define IN_RATE      8000
#define IN_CHANNELS  1
#define IN_FRAME_SZ  2   /* 16-bit mono */
#define IN_BUF_MS    20

/* ------------------------------------------------------------------ */
/* Output stream                                                       */
/* ------------------------------------------------------------------ */

struct mt6575_out {
    struct audio_stream_out stream;
    int                     fd;
    pthread_mutex_t         lock;
};

static uint32_t out_get_sample_rate(const struct audio_stream *s)
{
    (void)s;
    return OUT_RATE;
}

static int out_set_sample_rate(struct audio_stream *s, uint32_t rate)
{
    (void)s; (void)rate;
    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *s)
{
    (void)s;
    return 4096;
}

static uint32_t out_get_channels(const struct audio_stream *s)
{
    (void)s;
    return AUDIO_CHANNEL_OUT_STEREO;
}

static audio_format_t out_get_format(const struct audio_stream *s)
{
    (void)s;
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *s, audio_format_t fmt)
{
    (void)s; (void)fmt;
    return 0;
}

static int out_standby(struct audio_stream *s)
{
    (void)s;
    return 0;
}

static int out_dump(const struct audio_stream *s, int fd)
{
    (void)s; (void)fd;
    return 0;
}

static int out_set_parameters(struct audio_stream *s, const char *kv)
{
    (void)s; (void)kv;
    return 0;
}

static char *out_get_parameters(const struct audio_stream *s, const char *keys)
{
    (void)s; (void)keys;
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *s)
{
    (void)s;
    return 50;
}

static int out_set_volume(struct audio_stream_out *s, float left, float right)
{
    (void)s; (void)left; (void)right;
    return 0;
}

static ssize_t out_write(struct audio_stream_out *s, const void *buf, size_t bytes)
{
    struct mt6575_out *out = (struct mt6575_out *)s;
    ssize_t ret;

    pthread_mutex_lock(&out->lock);
    if (out->fd >= 0) {
        ret = write(out->fd, buf, bytes);
        if (ret < 0) {
            ALOGW("out_write: /dev/eac write failed (%s), discarding", strerror(errno));
            ret = bytes; /* don't stall AudioFlinger */
        }
    } else {
        /* no hw — sleep to avoid busy-spin */
        usleep((uint64_t)bytes * 1000000 /
               (OUT_RATE * OUT_CHANNELS * sizeof(int16_t)));
        ret = bytes;
    }
    pthread_mutex_unlock(&out->lock);
    return ret;
}

static int out_get_render_position(const struct audio_stream_out *s, uint32_t *frames)
{
    (void)s;
    if (frames) *frames = 0;
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *s, effect_handle_t e)
{
    (void)s; (void)e;
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *s, effect_handle_t e)
{
    (void)s; (void)e;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Input stream                                                        */
/* ------------------------------------------------------------------ */

struct mt6575_in {
    struct audio_stream_in stream;
    int                    fd;
    pthread_mutex_t        lock;
    uint32_t               rate;
    audio_channel_mask_t   channels;
};

static uint32_t in_get_sample_rate(const struct audio_stream *s)
{
    struct mt6575_in *in = (struct mt6575_in *)s;
    return in->rate;
}

static int in_set_sample_rate(struct audio_stream *s, uint32_t rate)
{
    (void)s; (void)rate;
    return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *s)
{
    struct mt6575_in *in = (struct mt6575_in *)s;
    size_t frames = (in->rate * IN_BUF_MS) / 1000;
    return frames * IN_FRAME_SZ;
}

static uint32_t in_get_channels(const struct audio_stream *s)
{
    struct mt6575_in *in = (struct mt6575_in *)s;
    return in->channels;
}

static audio_format_t in_get_format(const struct audio_stream *s)
{
    (void)s;
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *s, audio_format_t fmt)
{
    (void)s; (void)fmt;
    return 0;
}

static int in_standby(struct audio_stream *s)
{
    (void)s;
    return 0;
}

static int in_dump(const struct audio_stream *s, int fd)
{
    (void)s; (void)fd;
    return 0;
}

static int in_set_parameters(struct audio_stream *s, const char *kv)
{
    (void)s; (void)kv;
    return 0;
}

static char *in_get_parameters(const struct audio_stream *s, const char *keys)
{
    (void)s; (void)keys;
    return strdup("");
}

static int in_add_audio_effect(const struct audio_stream *s, effect_handle_t e)
{
    (void)s; (void)e;
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *s, effect_handle_t e)
{
    (void)s; (void)e;
    return 0;
}

static int in_set_gain(struct audio_stream_in *s, float gain)
{
    (void)s; (void)gain;
    return 0;
}

static ssize_t in_read(struct audio_stream_in *s, void *buf, size_t bytes)
{
    struct mt6575_in *in = (struct mt6575_in *)s;
    ssize_t ret = 0;

    pthread_mutex_lock(&in->lock);
    if (in->fd >= 0) {
        ret = read(in->fd, buf, bytes);
        if (ret < 0) {
            ALOGW("in_read: /dev/eac read failed (%s), returning silence", strerror(errno));
            memset(buf, 0, bytes);
            ret = bytes;
        }
    } else {
        /* sleep proportional to buffer duration, return silence */
        usleep((uint64_t)bytes * 1000000 /
               (in->rate * IN_FRAME_SZ));
        memset(buf, 0, bytes);
        ret = bytes;
    }
    pthread_mutex_unlock(&in->lock);
    return ret;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *s)
{
    (void)s;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Audio device                                                        */
/* ------------------------------------------------------------------ */

struct mt6575_audio_dev {
    struct audio_hw_device hw;
    pthread_mutex_t        lock;
    int                    mode;
    bool                   mic_mute;
};

static int adev_open_output_stream(struct audio_hw_device *dev,
        audio_io_handle_t handle, audio_devices_t devices,
        audio_output_flags_t flags, struct audio_config *config,
        struct audio_stream_out **stream_out)
{
    (void)handle; (void)devices; (void)flags;
    struct mt6575_out *out;

    out = calloc(1, sizeof(*out));
    if (!out) return -ENOMEM;

    pthread_mutex_init(&out->lock, NULL);

    out->fd = open(EAC_DEV, O_WRONLY);
    if (out->fd < 0) {
        ALOGW("adev_open_output_stream: cannot open %s (%s)", EAC_DEV, strerror(errno));
    }

    out->stream.common.get_sample_rate    = out_get_sample_rate;
    out->stream.common.set_sample_rate    = out_set_sample_rate;
    out->stream.common.get_buffer_size    = out_get_buffer_size;
    out->stream.common.get_channels       = out_get_channels;
    out->stream.common.get_format         = out_get_format;
    out->stream.common.set_format         = out_set_format;
    out->stream.common.standby            = out_standby;
    out->stream.common.dump               = out_dump;
    out->stream.common.set_parameters     = out_set_parameters;
    out->stream.common.get_parameters     = out_get_parameters;
    out->stream.common.add_audio_effect   = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency               = out_get_latency;
    out->stream.set_volume                = out_set_volume;
    out->stream.write                     = out_write;
    out->stream.get_render_position       = out_get_render_position;

    if (config) {
        config->format         = AUDIO_FORMAT_PCM_16_BIT;
        config->channel_mask   = AUDIO_CHANNEL_OUT_STEREO;
        config->sample_rate    = OUT_RATE;
    }

    *stream_out = &out->stream;
    ALOGI("adev_open_output_stream: fd=%d", out->fd);
    return 0;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
        struct audio_stream_out *stream)
{
    struct mt6575_out *out = (struct mt6575_out *)stream;
    (void)dev;
    if (out->fd >= 0) close(out->fd);
    pthread_mutex_destroy(&out->lock);
    free(out);
}

static int adev_open_input_stream(struct audio_hw_device *dev,
        audio_io_handle_t handle, audio_devices_t devices,
        struct audio_config *config, struct audio_stream_in **stream_in)
{
    (void)dev; (void)handle; (void)devices;
    struct mt6575_in *in;

    in = calloc(1, sizeof(*in));
    if (!in) return -ENOMEM;

    pthread_mutex_init(&in->lock, NULL);

    in->rate     = config ? config->sample_rate : IN_RATE;
    in->channels = config ? config->channel_mask : AUDIO_CHANNEL_IN_MONO;
    if (!in->rate)     in->rate     = IN_RATE;
    if (!in->channels) in->channels = AUDIO_CHANNEL_IN_MONO;

    in->fd = open(EAC_DEV, O_RDONLY);
    if (in->fd < 0) {
        ALOGW("adev_open_input_stream: cannot open %s (%s), returning silence",
              EAC_DEV, strerror(errno));
    }

    in->stream.common.get_sample_rate    = in_get_sample_rate;
    in->stream.common.set_sample_rate    = in_set_sample_rate;
    in->stream.common.get_buffer_size    = in_get_buffer_size;
    in->stream.common.get_channels       = in_get_channels;
    in->stream.common.get_format         = in_get_format;
    in->stream.common.set_format         = in_set_format;
    in->stream.common.standby            = in_standby;
    in->stream.common.dump               = in_dump;
    in->stream.common.set_parameters     = in_set_parameters;
    in->stream.common.get_parameters     = in_get_parameters;
    in->stream.common.add_audio_effect   = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain                  = in_set_gain;
    in->stream.read                      = in_read;
    in->stream.get_input_frames_lost     = in_get_input_frames_lost;

    if (config) {
        config->format       = AUDIO_FORMAT_PCM_16_BIT;
        config->sample_rate  = in->rate;
        config->channel_mask = in->channels;
    }

    *stream_in = &in->stream;
    ALOGI("adev_open_input_stream: rate=%u fd=%d", in->rate, in->fd);
    return 0;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
        struct audio_stream_in *stream)
{
    struct mt6575_in *in = (struct mt6575_in *)stream;
    (void)dev;
    if (in->fd >= 0) close(in->fd);
    pthread_mutex_destroy(&in->lock);
    free(in);
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    (void)dev;
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float vol)
{
    (void)dev; (void)vol;
    return 0;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float vol)
{
    (void)dev; (void)vol;
    return 0;
}

static int adev_get_master_volume(struct audio_hw_device *dev, float *vol)
{
    (void)dev;
    if (vol) *vol = 1.0f;
    return 0;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct mt6575_audio_dev *adev = (struct mt6575_audio_dev *)dev;
    adev->mode = mode;
    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool mute)
{
    struct mt6575_audio_dev *adev = (struct mt6575_audio_dev *)dev;
    adev->mic_mute = mute;
    return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *mute)
{
    const struct mt6575_audio_dev *adev = (const struct mt6575_audio_dev *)dev;
    if (mute) *mute = adev->mic_mute;
    return 0;
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kv)
{
    (void)dev; (void)kv;
    return 0;
}

static char *adev_get_parameters(const struct audio_hw_device *dev, const char *keys)
{
    (void)dev; (void)keys;
    return strdup("");
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
        const struct audio_config *config)
{
    (void)dev;
    uint32_t rate = config ? config->sample_rate : IN_RATE;
    if (!rate) rate = IN_RATE;
    size_t frames = (rate * IN_BUF_MS) / 1000;
    return frames * IN_FRAME_SZ;
}

static int adev_dump(const audio_hw_device_t *dev, int fd)
{
    (void)dev; (void)fd;
    return 0;
}

static int adev_close(hw_device_t *device)
{
    struct mt6575_audio_dev *adev = (struct mt6575_audio_dev *)device;
    pthread_mutex_destroy(&adev->lock);
    free(adev);
    return 0;
}

static int adev_open(const hw_module_t *module, const char *name,
        hw_device_t **device)
{
    struct mt6575_audio_dev *adev;

    /* accept any interface name — CM10 4.1 may pass "audio_hw_if" not "primary" */

    adev = calloc(1, sizeof(*adev));
    if (!adev) return -ENOMEM;

    pthread_mutex_init(&adev->lock, NULL);

    adev->hw.common.tag            = HARDWARE_DEVICE_TAG;
    adev->hw.common.version        = AUDIO_DEVICE_API_VERSION_1_0;
    adev->hw.common.module         = (hw_module_t *)module;
    adev->hw.common.close          = adev_close;

    adev->hw.init_check            = adev_init_check;
    adev->hw.set_voice_volume      = adev_set_voice_volume;
    adev->hw.set_master_volume     = adev_set_master_volume;
    adev->hw.get_master_volume     = adev_get_master_volume;
    adev->hw.set_mode              = adev_set_mode;
    adev->hw.set_mic_mute          = adev_set_mic_mute;
    adev->hw.get_mic_mute          = adev_get_mic_mute;
    adev->hw.set_parameters        = adev_set_parameters;
    adev->hw.get_parameters        = adev_get_parameters;
    adev->hw.get_input_buffer_size = adev_get_input_buffer_size;
    adev->hw.open_output_stream    = adev_open_output_stream;
    adev->hw.close_output_stream   = adev_close_output_stream;
    adev->hw.open_input_stream     = adev_open_input_stream;
    adev->hw.close_input_stream    = adev_close_input_stream;
    adev->hw.dump                  = adev_dump;

    *device = &adev->hw.common;
    ALOGI("adev_open: MT6575 audio HAL ready");
    return 0;
}

static struct hw_module_methods_t hal_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag            = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version    = HARDWARE_HAL_API_VERSION,
        .id             = AUDIO_HARDWARE_MODULE_ID,
        .name           = "MT6575 audio HAL",
        .author         = "vee5ss port",
        .methods        = &hal_methods,
    },
};
