package com.arvin.ffmpeg.sample.event;

public class MediaInfoEvent {
	private int mDuration = 0;

	public MediaInfoEvent() {
		super();
	}

	public int getDuration() {
		return mDuration;
	}

	public void setDuration(int mDuration) {
		this.mDuration = mDuration;
	}
	
}
