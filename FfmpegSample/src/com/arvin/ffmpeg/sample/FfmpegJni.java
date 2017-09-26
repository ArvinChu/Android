package com.arvin.ffmpeg.sample;

import org.greenrobot.eventbus.EventBus;

import com.arvin.ffmpeg.sample.event.MediaInfoEvent;

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
		MediaInfoEvent event = new MediaInfoEvent();
		event.setDuration(time);
		EventBus.getDefault().post(event);
	}
	
	
	static {
		System.loadLibrary("FfmpegSample");
	}
}
