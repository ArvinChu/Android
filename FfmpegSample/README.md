[TOC]
# FFmpegSameple介绍

----------

## 编译FFmpeg
### 选择参考项目
参考[FFmpeg CompilationGuide](http://trac.ffmpeg.org/wiki/CompilationGuide)，上面有FFmpeg在各平台的编译指导。这里我们需要在Android上进行移植，选择[Android](http://trac.ffmpeg.org/wiki/CompilationGuide/Android)。可以看到有以下5个参考项目：

- https://github.com/hiteshsondhi88/ffmpeg-android
- [android-ffmpeg-with-rtmp](https://github.com/cine-io/android-ffmpeg-with-rtmp)
- [Bambuser](http://bambuser.com/opensource)
- [ffmpeg4android](http://sourceforge.net/projects/ffmpeg4android/)
- [How to Build FFmpeg for Android](http://www.roman10.net/how-to-build-ffmpeg-for-android/)
> <i class="icon-hand-right icon-2x"></i>注：以上项目并非FFmpeg官方制定，而是使用人数较多的第三方项目，所以移植的过程中需耐心排除问题。

**这里我们选择第一个项目，因为这个项目算是比较新的项目**
### 下载编译 ffmpeg-android
打开[ffmpeg-android](https://github.com/WritingMinds/ffmpeg-android)，查看项目简介，包含以下特性：

- FFmpeg for Android compiled with x264, libass, fontconfig, freetype and fribidi
- Supports Android L
- FFmpeg Android Library

并且支持以下CPU架构：armv7、armv7-neon、x86。<br>
由于需要编译FFmpeg，所以选在Ubuntu16.04虚拟机中进行编译移植。步骤如下：

- 虚拟机中安装 NDK 开发工具，并配置环境变量<br>
`export ANDROID_NDK={Android NDK Base Path}`
- 执行以下命令安装编译工具<br>
`sudo apt-get --quiet --yes install build-essential git autoconf libtool pkg-config gperf gettext yasm python-lxml`
- 执行以下脚本更新源码<br>
`./init_update_libs.sh`
- 执行以下脚本进行编译<br>
`./android_build.sh`

下面介绍更新源码及编译过程中遇到的问题。<br>
首先，看看init_update_libs.sh文件：<br>
```
#!/bin/bash

echo "============================================"
echo "Updating submodules"
# 定义在.gitmodules文件中的子模块，下载的是各个库的最新代码
git submodule update --init
echo "============================================"
echo "Updating libpng, expat and fribidi"
rm -rf libpng-*
rm -rf expat-*
rm -rf fribidi-*
rm -rf lame-*
# 以下子模块不能使用最新的代码，所以直接给出相关下载地址
wget -O- ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.21.tar.xz | tar xJ
wget -O- http://downloads.sourceforge.net/project/expat/expat/2.1.0/expat-2.1.0.tar.gz | tar xz
wget -O- http://fribidi.org/download/fribidi-0.19.7.tar.bz2 | tar xj
wget -O- http://sourceforge.net/projects/lame/files/lame/3.99/lame-3.99.5.tar.gz | tar xz
echo "============================================"

```
><i class="icon-hand-right icon-2x"></i>注：libpng-1.6.21.tar.xz上述给出的地址已经废弃了，无法下载。可以参考Issues中的[#45](https://github.com/WritingMinds/ffmpeg-android/issues/45)给出的[下载地址](http://downloads.sourceforge.net/libpng/libpng-1.6.21.tar.xz)。

所有源码下载完成后，进行编译，看看android_build.sh文件：
```
#!/bin/bash

. settings.sh

BASEDIR=$(pwd)
# FFmpeg依赖的库文件临时存放路径
TOOLCHAIN_PREFIX=${BASEDIR}/toolchain-android
# Applying required patches
patch  -p0 -N --dry-run --silent -f fontconfig/src/fcxml.c < android_donot_use_lconv.patch 1>/dev/null
if [ $? -eq 0 ]; then
  patch -p0 -f fontconfig/src/fcxml.c < android_donot_use_lconv.patch
fi
# 遍历支持的CPU架构，定义在settings.sh文件中
for i in "${SUPPORTED_ARCHITECTURES[@]}"
do
  rm -rf ${TOOLCHAIN_PREFIX}
  # $1 = architecture
  # $2 = base directory
  # $3 = pass 1 if you want to export default compiler environment variables
  ./x264_build.sh $i $BASEDIR 0 || exit 1
  ./libpng_build.sh $i $BASEDIR 1 || exit 1
  ./freetype_build.sh $i $BASEDIR 1 || exit 1
  ./expat_build.sh $i $BASEDIR 1 || exit 1
  ./fribidi_build.sh $i $BASEDIR 1 || exit 1
  ./fontconfig_build.sh $i $BASEDIR 1 || exit 1
  ./libass_build.sh $i $BASEDIR 1 || exit 1
  ./lame_build.sh $i $BASEDIR 1 || exit 1
  # 以上步骤编译FFmpeg依赖库，下面开始编译FFmpeg
  ./ffmpeg_build.sh $i $BASEDIR 0 || exit 1
done

rm -rf ${TOOLCHAIN_PREFIX}
```
>注：编译过程中可能会遇到fp = urllib2.urlopen('http://unicode.org/cldr/utility/list-unicodeset.jsp?a=[%3AGC%3DZs%3A][%3ADI%3A]&abb=on&ucd=on&esc=on&g')报错，那是因为被墙或网络有问题。可以换个网络环境或翻墙后再试试。

编译成功后，会在`{ffmpeg-android path}/build/`目录下生成FFmpeg静态库文件： libavcodec.a libavfilter.a libavutil.a libswresample.a libavdevice.a libavformat.a libpostproc.a libswscale.a

----------

## 移植 ffmpeg-android
### 定制编译 ffmpeg-android
编写jni的时候，遇到libmp3lame及x264引用问题（MP3及Video编码使用），考虑到目前不需要编码方面的功能，所以修改`ffmpeg_build.sh`文件：
```
#!/bin/bash

. abi_settings.sh $1 $2 $3

pushd ffmpeg

case $1 in
  armeabi-v7a | armeabi-v7a-neon)
    CPU='cortex-a8'
  ;;
  x86)
    CPU='i686'
  ;;
esac

make clean

./configure \
--target-os="$TARGET_OS" \
--cross-prefix="$CROSS_PREFIX" \
--arch="$NDK_ABI" \
--cpu="$CPU" \
--enable-runtime-cpudetect \
--sysroot="$NDK_SYSROOT" \
--enable-pic \
# 修改enable-libx264为disable-libx264
--disable-libx264 \
--enable-libass \
--enable-libfreetype \
--enable-libfribidi \
# 修改enable-libmp3lame为disable-libmp3lame
--disable-libmp3lame \
--enable-fontconfig \
--enable-pthreads \
--disable-debug \
--disable-ffserver \
--enable-version3 \
--enable-hardcoded-tables \
--disable-ffplay \
--disable-ffprobe \
--enable-gpl \
--enable-yasm \
--disable-doc \
--disable-shared \
--enable-static \
--pkg-config="${2}/ffmpeg-pkg-config" \
--prefix="${2}/build/${1}" \
--extra-cflags="-I${TOOLCHAIN_PREFIX}/include $CFLAGS" \
--extra-ldflags="-L${TOOLCHAIN_PREFIX}/lib $LDFLAGS" \
--extra-libs="-lpng -lexpat -lm" \
--extra-cxxflags="$CXX_FLAGS" || exit 1

make -j${NUMBER_OF_CORES} && make install || exit 1

popd
```
修改完以上两处后，重新编译。
### 编写 NDK demo
手上有台x86的PAD，所以这里选择移植到x86上。
1. Eclipse新建Android项目，然后新建jni目录，将`{ffmpeg-android path}/build/x86`下的`lib、include`文件夹拷贝到jni目录下
2. jni目录下新建`Application.mk`文件，内容如下：
```
# 表示编译目标为x86平台
APP_ABI := x86
```
3. jni目录下新建`Android.mk`文件，内容如下：
```
LOCAL_PATH := $(call my-dir)

######## 库文件路径 ######################
THIRD_LIB_DIR := $(LOCAL_PATH)/../lib
######## 集成FFmpeg相关静态库文件 #########
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

############# 编译Smaple ##################
include $(CLEAR_VARS)
# 库文件名称
LOCAL_MODULE    := FfmpegSample
# 使用的源文件
LOCAL_SRC_FILES := FfmpegSample.c
# 引用的头文件路径
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
# 引用的静态库
LOCAL_STATIC_LIBRARIES := \
		avdevice \
        avformat \
        avfilter \
        avcodec \
        swresample \
        swscale \
        avutil
# 使用的 Android 系统包含的动态库        
LOCAL_LDLIBS += -ljnigraphics -llog -ldl -lstdc++ -lz \
		-malign-double -malign-loops -landroid -lOpenSLES
LOCAL_SHARED_LIBRARIES += pthread
# 表示编译成动态库		
include $(BUILD_SHARED_LIBRARY)
```
4. 编写demo代码
FfmpegSample.h
```c
/*
 * FfmpegSmaple.h
 *
 *  Created on: 2017年9月25日
 *      Author: ArvinChu
 */

#ifndef FFMPEGSAMPLE_H_
#define FFMPEGSAMPLE_H_

#include <string.h>
#include <jni.h>
#include <pthread.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * 测试函数
 */
JNIEXPORT jstring Java_com_arvin_ffmpeg_sample_FfmpegJni_stringFromJNI( JNIEnv* env, jobject thiz );

#ifdef __cplusplus
}
#endif

#endif /* FFMPEGSAMPLE_H_ */
```

FfmpegSample.c
```c
#include "FfmpegSample.h"

#define ANDROID

#ifdef ANDROID
// 定义Logcat的TAG标签
#define LOG_TAG_ERROR "FFMPEG (>_<)"
#define LOG_TAG_INFO "FFMPEG (^_^)"
// 输出error信息
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_ERROR, format, ##__VA_ARGS__)
// 输出info信息
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  LOG_TAG_INFO, format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("(>_<) " format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("(^_^) " format "\n", ##__VA_ARGS__)
#endif

JNIEXPORT jstring Java_com_arvin_ffmpeg_sample_FfmpegJni_stringFromJNI( JNIEnv* env, jobject thiz )
{
    char info[10000] = { 0 };
    // 注册ffmpeg相关函数
    av_register_all();
    // 获取ffmpeg编解码器相关信息
    sprintf(info, "%s\n", avcodec_configuration());
    LOGI("avcodec config:%s", info);
    return (*env)->NewStringUTF(env, info);
}

```
编译运行Demo，可以在logcat中看到`avcodec config:...`打印信息，表示集成FFmpeg OK.

----------

## 封装接口

<i class="icon-hand-right icon-2x"></i>尽情期待
