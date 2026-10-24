LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE         := libvee5ss-ril
LOCAL_SRC_FILES      := vee5ss_ril.c
LOCAL_CFLAGS         := -DRIL_SHLIB
LOCAL_SHARED_LIBRARIES := liblog libdl

include $(BUILD_SHARED_LIBRARY)
