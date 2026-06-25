#include <binder/Binder.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <utils/String16.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

namespace android {

static const String16 kDescriptor("android.apps.ILGPService");
static const char kBtAddrNvramPath[] = "/data/nvram/APCFG/APRDEB/BT_Addr";
static const char kBtAddrKernelPath[] = "/data/BT_Addr";

enum {
    LGP_INIT = 1,
    LGP_BT_WRITE = 108,
    LGP_BT_READ = 109,
    LGP_FAC_INITIALIZE = 152,
};

class LGPServiceStub : public BBinder {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
            uint32_t flags = 0) {
        (void)flags;

        if (code == INTERFACE_TRANSACTION) {
            reply->writeString16(kDescriptor);
            return NO_ERROR;
        }

        switch (code) {
        case LGP_INIT:
        case LGP_FAC_INITIALIZE:
            data.enforceInterface(kDescriptor);
            ALOGI("ILGPService transaction %u -> ok", code);
            reply->writeInt32(0);
            return NO_ERROR;

        case LGP_BT_WRITE:
            data.enforceInterface(kDescriptor);
            ALOGI("ILGPService lgp_bt_write");
            reply->writeInt32(0);
            return NO_ERROR;

        case LGP_BT_READ: {
            data.enforceInterface(kDescriptor);
            unsigned char btaddr[6] = {0};
            readBtAddr(kBtAddrNvramPath, btaddr);
            if (isEmptyBtAddr(btaddr)) {
                readBtAddr(kBtAddrKernelPath, btaddr);
            }
            ALOGI("ILGPService lgp_bt_read -> %02x:%02x:%02x:%02x:%02x:%02x",
                    btaddr[0], btaddr[1], btaddr[2], btaddr[3], btaddr[4], btaddr[5]);
            unsigned int first = 0;
            unsigned int second = 0;
            memcpy(&first, btaddr, sizeof(first));
            memcpy(&second, btaddr + sizeof(first), 2);
            reply->writeInt32(first);
            reply->writeInt32(second);
            reply->writeInt32(0);
            return NO_ERROR;
        }

        default:
            data.enforceInterface(kDescriptor);
            ALOGW("Unhandled ILGPService transaction %u", code);
            reply->writeInt32(0);
            return NO_ERROR;
        }
    }

private:
    static bool isEmptyBtAddr(const unsigned char* btaddr) {
        for (size_t i = 0; i < 6; ++i) {
            if (btaddr[i] != 0) {
                return false;
            }
        }
        return true;
    }

    static bool readBtAddr(const char* path, unsigned char* btaddr) {
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            return false;
        }
        ssize_t n = read(fd, btaddr, 6);
        close(fd);
        return n == 6;
    }
};

}  // namespace android

int main(int, char**) {
    android::sp<android::ProcessState> proc(android::ProcessState::self());
    android::sp<android::IServiceManager> sm(android::defaultServiceManager());
    android::status_t ret = sm->addService(android::String16("android.apps.ILGPService"),
            new android::LGPServiceStub());
    if (ret != android::NO_ERROR) {
        ALOGE("Failed to register android.apps.ILGPService: %d", ret);
        return 1;
    }
    property_set("lgpservice.ready", "1");
    android::ProcessState::self()->startThreadPool();
    android::IPCThreadState::self()->joinThreadPool();
    return 0;
}
