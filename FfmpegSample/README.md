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

############# 编译Sample ##################
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
### Java层接口定义
```java
public class FfmpegJni {

	private static final String TAG = "FFMPEGJNI";

	/**
	 * 打开视频文件并播放
	 * @param surface Surface对象
	 * @param filePath 文件路径
	 * @return {@code 0} Open success, {@code -1} Open failed.
	 */
	public static native int open(Object surface, String filePath);
	/**
	 * 播放
	 * @return
	 */
	public static native int play();
	/**
	 * 暂停
	 * @return
	 */
	public static native int pause();
	/**
	 * 跳转到指定进度播放
	 * @param time
	 * @return
	 */
	public static native int seek(int time);
	/**
	 * 停止播放并释放资源
	 * @return
	 */
	public static native int close();
	/**
	 * 获取当前播放进度
	 * @return
	 */
	public static native int getCurrentPosition();
	/**
	 * 设置视频时长
	 * @param time
	 */
	public static void setDuration(int time) {

	}


	static {
		System.loadLibrary("FfmpegSample");
	}
}
```
### Native层接口实现
```c
#include "FfmpegSample.h"

#define ANDROID

#ifdef ANDROID
#define LOG_TAG_ERROR "FFMPEG (>_<)"
#define LOG_TAG_INFO "FFMPEG (^_^)"
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_ERROR, format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  LOG_TAG_INFO, format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("(>_<) " format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("(^_^) " format "\n", ##__VA_ARGS__)
#endif

#define BOOL int
#define TRUE 1
#define FALSE 0

// define variables
static AVFormatContext * pFormatCtx = NULL;
static ANativeWindow* nativeWindow = NULL;
static int videoStream = -1;

static JavaVM *g_jvm = NULL;
static pthread_t playThread;
extern void *play();
static pthread_mutex_t mutex;
static pthread_cond_t cond;
static BOOL isPlay = FALSE;
static BOOL isSeek = FALSE;
static BOOL isFinish = FALSE;
static int current_time = 0;
static int64_t seek_time = 0;
// end

JNIEXPORT jstring Java_com_arvin_ffmpeg_sample_FfmpegJni_stringFromJNI( JNIEnv* env, jobject thiz )
{
    char info[10000] = { 0 };
    av_register_all();
    sprintf(info, "%s\n", avcodec_configuration());
    return (*env)->NewStringUTF(env, info);
}

JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_open( JNIEnv* env, jobject thiz,
		jobject jsurface, jstring jfilePath ) {
	(*env)->GetJavaVM(env, &g_jvm);
	const char* filePath = NULL;
	filePath = (*env)->GetStringUTFChars(env, jfilePath, NULL);
	if(filePath == NULL) {
		return -1; /* OutOfMemoryError already thrown */
	}
	LOGI("play %s", filePath);

	int result = -1;
	//######################################################
	av_register_all();
	pFormatCtx = avformat_alloc_context();

	// Open video file
	if((result = avformat_open_input(&pFormatCtx, filePath, NULL, NULL)) != 0 ) {
		LOGE("Couldn't open file:%s. error_code:%d", filePath, result);
		(*env)->ReleaseStringUTFChars(env, jfilePath, filePath);
		return -1; // Couldn't open file
	}

	// 获取时长
	int totalSeconds = (int)pFormatCtx->duration / 1000000;
	LOGI("duration: %d", totalSeconds);
	jmethodID methodSetDuration = NULL;
	methodSetDuration = (*env)->GetStaticMethodID(env, thiz, "setDuration","(I)V");
	if (methodSetDuration == NULL) {
		LOGE("Couldn't find method: setDuration");
		(*env)->ReleaseStringUTFChars(env, jfilePath, filePath);
		return -1;
	}
	// 更新界面视频时长信息
	(*env)->CallStaticVoidMethod(env, thiz, methodSetDuration, totalSeconds);

	// 获取native window
	nativeWindow = ANativeWindow_fromSurface(env, jsurface);

	// 创建子线程播放视频
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	result = pthread_create(&playThread, NULL, play, NULL);
	if (result != 0) {
	    LOGE("create play thread failed!");
	    (*env)->ReleaseStringUTFChars(env, jfilePath, filePath);
	    return -1;
	}

	(*env)->ReleaseStringUTFChars(env, jfilePath, filePath);
	return 0;
}

void *play() {
	JNIEnv *env;
	if((*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL) != JNI_OK) {
		LOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
		return NULL;
	}

	int result = -1;
	// Retrieve stream information
	if((result = avformat_find_stream_info(pFormatCtx, NULL)) < 0 ) {
		LOGE("Couldn't find stream information. error_code:%d", result);
		return -1;
	}

	// Find the first video stream
	int i;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
				&& videoStream < 0) {
			videoStream = i;
		}
	}
	if(videoStream == -1) {
		LOGE("Didn't find a video stream.");
		return -1; // Didn't find a video stream
	}

	// Get a pointer to the CODEC context for the video stream
	AVCodecContext  * pCodecCtx = pFormatCtx->streams[videoStream]->codec;
	// Find the decoder for the video stream
	AVCodec * pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec == NULL) {
		LOGE("Codec not found.");
		return -1; // Codec not found
	}

	if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		LOGE("Could not open codec.");
		return -1; // Could not open codec
	}



	// 获取视频宽高
	int videoWidth = pCodecCtx->width;
	int videoHeight = pCodecCtx->height;

	// 设置native window的buffer大小,可自动拉伸
	ANativeWindow_setBuffersGeometry(nativeWindow,  videoWidth, videoHeight, WINDOW_FORMAT_RGBA_8888);
	ANativeWindow_Buffer windowBuffer;

	if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		LOGE("Could not open codec.");
		return -1; // Could not open codec
	}

	// Allocate video frame
	AVFrame * pFrame = av_frame_alloc();

	// 用于渲染
	AVFrame * pFrameRGBA = av_frame_alloc();
	if(pFrameRGBA == NULL || pFrame == NULL) {
		LOGE("Could not allocate video frame.");
		return -1;
	}

	// Determine required buffer size and allocate buffer
	int numBytes=av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height, 1);
	uint8_t * buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, buffer, AV_PIX_FMT_RGBA,
			pCodecCtx->width, pCodecCtx->height, 1);

	// 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
	struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,
			pCodecCtx->height,
			pCodecCtx->pix_fmt,
			pCodecCtx->width,
			pCodecCtx->height,
			AV_PIX_FMT_RGBA,
			SWS_BILINEAR,
			NULL,
			NULL,
			NULL);

	int frameFinished;
	AVPacket packet;
	isPlay = TRUE;
	isSeek = FALSE;
	isFinish = FALSE;
	seek_time = 0;
	while(av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if(packet.stream_index == videoStream) {
			if (!isPlay) {
				// wait
				result = pthread_cond_wait(&cond, &mutex);
				if(result != 0) {
					LOGE("pthread_cond_wait...error");
				}
				isPlay = TRUE;
			}
			if (isSeek) {
				int64_t frameNum = av_rescale(seek_time * 1000,
						pFormatCtx->streams[videoStream]->time_base.den,
						pFormatCtx->streams[videoStream]->time_base.num);
				frameNum /= 1000;
				if(avformat_seek_file(pFormatCtx, videoStream, 0, frameNum, frameNum, AVSEEK_FLAG_FRAME) >= 0) {
					avcodec_flush_buffers(pCodecCtx);
				}
				isSeek = FALSE;
			}
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// 并不是decode一次就可解码出一帧
			if (frameFinished) {
				// lock native window buffer
				ANativeWindow_lock(nativeWindow, &windowBuffer, 0);

				// 格式转换
				sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
						pFrame->linesize, 0, pCodecCtx->height,
						pFrameRGBA->data, pFrameRGBA->linesize);

				// 获取stride
				uint8_t * dst = windowBuffer.bits;
				int dstStride = windowBuffer.stride * 4;
				uint8_t * src = (uint8_t*) (pFrameRGBA->data[0]);
				int srcStride = pFrameRGBA->linesize[0];
				// 计算当前播放时间
				current_time = packet.pts * av_q2d(pFormatCtx->streams[packet.stream_index]->time_base);
				// 由于window的stride和帧的stride不同,因此需要逐行复制
				int h;
				for (h = 0; h < videoHeight; h++) {
					memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
				}

				ANativeWindow_unlockAndPost(nativeWindow);
			}

		}
		av_packet_unref(&packet);
		if (isFinish)
			break;
	}

	av_free(buffer);
	av_free(pFrameRGBA);

	// Free the YUV frame
	av_free(pFrame);

	// Close the codecs
	avcodec_close(pCodecCtx);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	if((*g_jvm)->DetachCurrentThread(g_jvm) != JNI_OK)
	{
		LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
	}
	pthread_exit(0);
}

JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_play( JNIEnv* env, jobject thiz ) {
	pthread_cond_signal(&cond);
}

JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_pause( JNIEnv* env, jobject thiz ) {
	isPlay = FALSE;
}

JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_getCurrentPosition( JNIEnv* env, jobject thiz ) {
	return current_time;
}

JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_seek( JNIEnv* env, jobject thiz, jint jtime ) {
	LOGI("Java_com_arvin_ffmpeg_sample_FfmpegJni_seek: %d", jtime);
	if (pFormatCtx != NULL) {
		seek_time = jtime;
		isSeek = TRUE;
	}
}

JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_close( JNIEnv* env, jobject thiz ) {
	isFinish = TRUE;
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
	pthread_join(playThread, NULL);

	LOGI("Java_com_arvin_ffmpeg_sample_FfmpegJni_close end!");
	return 0;
}
```
## 待处理问题
 1. 视频播放速度未控制，导致视频播放的速度有问题。
首先我们分析一下 ffplay 控制视频正常播放的原理，先看一下总体流程图：
![ffplay总体流程图][1]
再看看Video PacketQueue走向
![ffplay Video PacketQueue][2]

 - 音频原始数据本身就是采样数据，有固定时钟周期，所以选择视频跟音频进行同步。

> 视频想跟音频进行同步的话，可能会出现跳帧的情况，每个视频帧播放时间差，都会起伏不定，不是恒定周期

 - DTS（Decode Time Stamp）和PTS（Presentation Time Stamp）都是时间戳，前者是解码时间，后者是显示时间，都是为视频帧、音频帧打上的时间标签，以更有效地支持上层应用的同步机制
最终视频的显示是根据比对音频/外部时钟做延时或提前显示（立即显示）

2. 音频播放未开发。
3. 音/视频同步未开发。
<i class="icon-hand-right icon-2x"></i>尽情期待


  [1]: https://raw.githubusercontent.com/ArvinChu/Android/master/FfmpegSample/doc/ffplay_flowchart.png
  [2]: https://raw.githubusercontent.com/ArvinChu/Android/master/FfmpegSample/doc/ffplay%20video%20PacketQueue.png
