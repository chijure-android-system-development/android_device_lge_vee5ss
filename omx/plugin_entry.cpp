/*
 * OMXMaster looks these up with dlsym(). They live apart from
 * omx_wrapper.cpp because HardwareAPI.h declares createOMXPlugin() with
 * C++ linkage.
 */
#include <media/hardware/OMXPluginBase.h>

namespace android {
OMXPluginBase *createVee5ssOMXPlugin();
}

extern "C" android::OMXPluginBase *createOMXPlugin()
{
    return android::createVee5ssOMXPlugin();
}

extern "C" void destroyOMXPlugin(android::OMXPluginBase *plugin)
{
    delete plugin;
}
