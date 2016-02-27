LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Tegra optimized OpenCV.mk
OPENCV_CAMERA_MODULES:=on
OPENCV_INSTALL_MODULES:=on
include $(OPENCV_PATH)/sdk/native/jni/OpenCV-tegra3.mk

# Linker
LOCAL_LDLIBS += -llog

# Our module sources
LOCAL_MODULE    := HelloFace
LOCAL_SRC_FILES := NativeLogging.cpp HelloFace.cpp HFApp.cpp

include $(BUILD_SHARED_LIBRARY)