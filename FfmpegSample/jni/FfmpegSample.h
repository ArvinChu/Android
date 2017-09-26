/*
 * FfmpegSmaple.h
 *
 *  Created on: 2017年9月25日
 *      Author: R08088
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

JNIEXPORT jstring Java_com_arvin_ffmpeg_sample_FfmpegJni_stringFromJNI( JNIEnv* env, jobject thiz );
JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_open( JNIEnv* env, jobject thiz, jobject surface, jstring filePath );
JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_play( JNIEnv* env, jobject thiz );
JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_pause( JNIEnv* env, jobject thiz );
JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_getCurrentPosition( JNIEnv* env, jobject thiz );
JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_seek( JNIEnv* env, jobject thiz, jint jtime );
JNIEXPORT jint Java_com_arvin_ffmpeg_sample_FfmpegJni_close( JNIEnv* env, jobject thiz );

#ifdef __cplusplus
}
#endif

#endif /* FFMPEGSAMPLE_H_ */
