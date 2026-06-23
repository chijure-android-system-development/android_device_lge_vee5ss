LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE        := audio.primary.mt6575
LOCAL_MODULE_PATH   := $(TARGET_OUT)/lib/hw
LOCAL_SRC_FILES     := audio_hw.c
LOCAL_C_INCLUDES    := $(call include-path-for, audio-utils)
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE_TAGS   := optional
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
