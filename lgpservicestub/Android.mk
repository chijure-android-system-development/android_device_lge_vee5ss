LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := lgpservicestub
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := lgpservicestub.cpp
LOCAL_SHARED_LIBRARIES := libbinder libutils libcutils liblog

include $(BUILD_EXECUTABLE)
