package com.arvin.ffmpeg.sample;

import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

import com.arvin.ffmpeg.sample.event.MediaInfoEvent;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;

public class MainActivity extends Activity implements SurfaceHolder.Callback{

	private TextView mInfo, mCurrent, mDuration;
	private ImageView mPlayPause;
	private SeekBar mSeekbar;
	private SurfaceHolder mHolder;
	// 是否正在播放
	private boolean isPlay = false;
	// 是否正在拖动进度条
	private boolean isDragging = false;
	// 视频文件路径
	private static final String PATH = "/sdcard/iphone.mp4";
	private static final int MSG_UPDATE_TIME = 1;
	
	private Handler mHandler = new Handler(){

		@Override
		public void handleMessage(Message msg) {
			switch(msg.what) {
			case MSG_UPDATE_TIME:
				if (!isDragging) {
					int currentTime = FfmpegJni.getCurrentPosition();
					mSeekbar.setProgress(currentTime);
					mCurrent.setText(generateTime(currentTime));
				}
				mHandler.sendEmptyMessageDelayed(MSG_UPDATE_TIME, 500);
				break;
			default:break;
			}
		}
		
	};
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		mInfo = (TextView) findViewById(R.id.main_info);
		mCurrent = (TextView) findViewById(R.id.mediacontroller_time_current);
		mDuration = (TextView) findViewById(R.id.mediacontroller_time_total);
		mPlayPause = (ImageView) findViewById(R.id.mediacontroller_play_pause);
		mSeekbar = (SeekBar) findViewById(R.id.mediacontroller_seekbar);
		
		mCurrent.setText("00:00");
		mDuration.setText("00:00");
		
		SurfaceView surfaceView = (SurfaceView) findViewById(R.id.main_surface);
		mHolder = surfaceView.getHolder();
		mHolder.addCallback(this);
		
		mPlayPause.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				if (isPlay) {
					mHandler.removeMessages(MSG_UPDATE_TIME);
					FfmpegJni.pause();
					mPlayPause.setImageResource(R.drawable.mediacontroller_play);
				} else {
					FfmpegJni.play();
					mPlayPause.setImageResource(R.drawable.mediacontroller_pause);
					mHandler.sendEmptyMessage(MSG_UPDATE_TIME);
				}
				isPlay = !isPlay;
			}
		});
		mSeekbar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			
			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				isDragging = false;
				String time = generateTime(seekBar.getProgress());
				FfmpegJni.seek(seekBar.getProgress());
				mCurrent.setText(time);
			}
			
			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
				isDragging = true;
			}
			
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
				if (!fromUser)
					return;
			}
		});
		EventBus.getDefault().register(this);
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		new Thread(new Runnable() {
			@Override
			public void run() {
            	int result = FfmpegJni.open(mHolder.getSurface(), PATH);
            	if (result == 0)
            		isPlay = true;
            	mHandler.sendEmptyMessage(MSG_UPDATE_TIME);
			}
		}).start();
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		FfmpegJni.close();
	}
	
	@Subscribe(threadMode=ThreadMode.MAIN)
	public void onMediaInfoEvent(MediaInfoEvent event) {
		mDuration.setText(generateTime(event.getDuration()));
		mSeekbar.setMax(event.getDuration());
		mSeekbar.setProgress(0);
	}
	
	/**
	 * 格式化时间(HH:mm:ss)
	 * @param time 时间，单位s
	 * @return
	 */
	private String generateTime(long time) {
		int totalSeconds = (int) time;
		int seconds = totalSeconds % 60;
		int minutes = (totalSeconds / 60) % 60;
		int hours = totalSeconds / 3600;
		
		return hours > 0 ? String.format("%02d:%02d:%02d", hours, minutes, seconds)
				: String.format("%02d:%02d", minutes, seconds);
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		EventBus.getDefault().unregister(this);
	}
	
}
