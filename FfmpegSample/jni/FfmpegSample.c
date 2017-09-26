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
