package edu.stanford.helloface;

import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2;
import org.opencv.android.JavaCameraView;
import org.opencv.core.Mat;

import android.app.Activity;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;

public class HelloFaceActivity extends Activity implements CvCameraViewListener2, OnTouchListener {
	private static final String TAG = "HelloFaceActivity";

    private JavaCameraView mCameraView;

    private long mNativeController = 0;
    
    private boolean mIsReadyForDetection = false;

    private boolean mIsReadyForTracking = false;

    private boolean mRequiresInit = false;
    
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		// Setup camera view
		setContentView(R.layout.activity_camera);
		mCameraView = (JavaCameraView) findViewById(R.id.primary_camera_view);
        mCameraView.setVisibility(SurfaceView.VISIBLE);
        mCameraView.setCvCameraViewListener(this);
        mCameraView.setMaxFrameSize(640, 480);
        
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        if (mNativeController == 0) {
            mNativeController = CreateNativeController();
        }
	}
	
	@Override
    public void onResume()
    {
        super.onResume();
        mNativeController = CreateNativeController();
        mCameraView.enableView();
        mCameraView.setOnTouchListener(HelloFaceActivity.this);
    }

    @Override
    public void onPause()
    {
        super.onPause();
        if (mCameraView != null) {
            mCameraView.disableView();
        }
    }
    
    public void onDestroy()
    {
        super.onDestroy();
        if (mCameraView != null) {
            mCameraView.disableView();
        }
        if (mNativeController != 0) {
            DestroyNativeController(mNativeController);
        }
    }

	@Override
	public boolean onTouch(View arg0, MotionEvent arg1) {
		// TODO Auto-generated method stub
		if (mIsReadyForDetection) {
			//mRequiresInit = true;
			mIsReadyForTracking = true;
			mIsReadyForDetection = false;
		} else {
			mIsReadyForDetection = true;
			mIsReadyForTracking = false;
		}
		return false;
	}

	@Override
	public void onCameraViewStarted(int width, int height) {
		// TODO Auto-generated method stub
		Toast.makeText(this, "Tap the screen to begin tracking.", Toast.LENGTH_LONG).show();
	}

	@Override
	public void onCameraViewStopped() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
		// TODO Auto-generated method stub
		Mat frame = inputFrame.rgba();
		if (mIsReadyForDetection) {
			HandleFrame(mNativeController, frame.getNativeObjAddr(), mRequiresInit, mIsReadyForDetection);
		}
		
        if (mIsReadyForTracking) {
            HandleFrame(mNativeController, frame.getNativeObjAddr(), mRequiresInit, mIsReadyForDetection);
            if (mRequiresInit) {
                mRequiresInit = false;
            }
        }
        return frame;
	}
	
	public native long CreateNativeController();

    public native void DestroyNativeController(long addr_native_controller);

    public native void HandleFrame(long addr_native_controller, long addr_rgba, boolean is_init, boolean is_detect);
}
