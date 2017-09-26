LOCAL_PATH := $(call my-dir)

###########################################
THIRD_LIB_DIR := $(LOCAL_PATH)/../lib
###########################################
include $(CLEAR_VARS)
LOCAL_MODULE := avcodec
LOCAL_SRC_FILES := $(THIRD_LIB_DIR)/libavcodec.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avdevice
LOCAL_SRC_FILES := $(THIRD_LIB_DIR)/libavdevice.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avfilter
LOCAL_SRC_FILES := $(THIRD_LIB_DIR)/libavfilter.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avformat
LOCAL_SRC_FILES := $(THIRD_LIB_DIR)/libavformat.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avutil
LOCAL_SRC_FILES := $(THIRD_LIB_DIR)/libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := postproc
LOCAL_SRC_FILES := $(THIRD_LIB_DIR)/libpostproc.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := swresample
LOCAL_SRC_FILES := $(THIRD_LIB_DIR)/libswresample.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := swscale
LOCAL_SRC_FILES := $(THIRD_LIB_DIR)/libswscale.a
include $(PREBUILT_STATIC_LIBRARY)

###########################################
include $(CLEAR_VARS)

LOCAL_MODULE    := FfmpegSample
LOCAL_SRC_FILES := FfmpegSample.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_STATIC_LIBRARIES := \
		avdevice \
        avformat \
        avfilter \
        avcodec \
        swresample \
        swscale \
        avutil
        
LOCAL_LDLIBS += -ljnigraphics -llog -ldl -lstdc++ -lz \
		-malign-double -malign-loops -landroid -lOpenSLES
LOCAL_SHARED_LIBRARIES += pthread
		
include $(BUILD_SHARED_LIBRARY)
