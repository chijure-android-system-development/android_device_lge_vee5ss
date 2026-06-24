/*
 * LG L5 II (vee5ss) camera HAL wrapper.
 *
 * The stock MTK HAL reports a phantom second/front camera.  This wrapper keeps
 * the fix device-local by delegating to the stock blob and exposing only the
 * single physical rear camera.
 */

#define LOG_TAG "vee5ss-camera"

#include <dlfcn.h>
#include <errno.h>
#include <string.h>

#include <cutils/log.h>
#include <hardware/camera.h>
#include <hardware/hardware.h>

static const char kStockCameraHal[] = "/system/lib/hw/camera.stock.mt6575.so";

static void *g_stock_handle;
static camera_module_t *g_stock_module;

static int load_stock_camera_hal(void)
{
    if (g_stock_module)
        return 0;

    g_stock_handle = dlopen(kStockCameraHal, RTLD_NOW);
    if (!g_stock_handle) {
        ALOGE("failed to load %s: %s", kStockCameraHal, dlerror());
        return -EINVAL;
    }

    g_stock_module = (camera_module_t *)dlsym(g_stock_handle, HAL_MODULE_INFO_SYM_AS_STR);
    if (!g_stock_module) {
        ALOGE("failed to find %s in %s: %s", HAL_MODULE_INFO_SYM_AS_STR,
                kStockCameraHal, dlerror());
        dlclose(g_stock_handle);
        g_stock_handle = NULL;
        return -EINVAL;
    }

    return 0;
}

static int vee5ss_get_number_of_cameras(void)
{
    int count;

    if (load_stock_camera_hal())
        return 0;

    count = g_stock_module->get_number_of_cameras();
    if (count > 1)
        ALOGW("stock HAL reports %d cameras; exposing only rear camera", count);

    return count > 0 ? 1 : 0;
}

static int vee5ss_get_camera_info(int camera_id, struct camera_info *info)
{
    if (load_stock_camera_hal())
        return -EINVAL;

    if (camera_id != 0)
        return -EINVAL;

    return g_stock_module->get_camera_info(0, info);
}

static int vee5ss_camera_open(const hw_module_t *module, const char *id,
        hw_device_t **device)
{
    (void)module;

    if (load_stock_camera_hal())
        return -EINVAL;

    if (strcmp(id, "0"))
        return -EINVAL;

    return g_stock_module->common.methods->open(&g_stock_module->common, id, device);
}

static hw_module_methods_t g_methods = {
    .open = vee5ss_camera_open,
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = CAMERA_HARDWARE_MODULE_ID,
        .name = "vee5ss camera wrapper",
        .author = "lge/mtk",
        .methods = &g_methods,
    },
    .get_number_of_cameras = vee5ss_get_number_of_cameras,
    .get_camera_info = vee5ss_get_camera_info,
};
