package edu.stanford.helloface;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2;
import org.opencv.android.JavaCameraView;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Mat;

import edu.stanford.helloface.R;
import edu.stanford.helloface.HelloFaceActivity;
import edu.stanford.helloface.FaceTrackingTask.TaskType;
import android.app.ActionBar;
import android.app.Activity;
import android.app.FragmentTransaction;
import android.app.ActionBar.Tab;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
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

    private TaskType mActiveTask;
    
    private List<String> inputFilePaths;
    
    private String[] inputPathsArray;
    
    private long mNativeController = 0;
    
    private boolean mIsReadyForDetection = false;

    private boolean mIsReadyForTracking = false;
    
    private boolean mDeRequiresInit = true;

    private boolean mTrRequiresInit = true;
    
    /*private class UIFaceTrackTask extends FaceTrackingTask
    {
    	UIFaceTrackTask(Context ctx, List<String> imagePaths, TaskType taskType)
        {
            super(ctx, imagePaths, taskType);
        }
    }*/

    
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
        mActiveTask = TaskType.NO_MASK;
        
        // Setup Tabs
        final ActionBar actionBar = getActionBar();
        actionBar.setNavigationMode(ActionBar.NAVIGATION_MODE_TABS);
        ActionBar.TabListener tabListener = new ActionBar.TabListener()
        {
            @Override
            public void onTabSelected(Tab tab, FragmentTransaction ft)
            {
                TaskType newTask = (TaskType) tab.getTag();
                if (newTask != mActiveTask)
                {
                    mActiveTask = (TaskType) tab.getTag();
                }
                handleTaskTrigger();
            }

            @Override
            public void onTabReselected(Tab tab, FragmentTransaction ft)
            {
                handleTaskTrigger();
            }

            @Override
            public void onTabUnselected(Tab tab, FragmentTransaction ft)
            {
            }
        };
        actionBar.addTab(actionBar.newTab().setText("No Mask")
                .setTag(TaskType.NO_MASK).setTabListener(tabListener));
        actionBar.addTab(actionBar.newTab().setText("Cartoon Face")
                .setTag(TaskType.CARTOON_MASK).setTabListener(tabListener));
        actionBar.addTab(actionBar.newTab().setText("Star Face")
                .setTag(TaskType.BLEND_FACES).setTabListener(tabListener));
        
        if (mNativeController == 0) {
            mNativeController = CreateNativeController();
        }
	}
	
	static private File getPictureStorageDir()
	{
	    File picDir = new File(Environment.getExternalStorageDirectory(),
	            "HelloFace-Files");
	    if (!picDir.exists())
	    {
	        try
	        {
	            picDir.mkdir();
	        } catch (Exception e)
	        {
	            Log.e(TAG, "Encountered exception while creating picture dir.");
	            e.printStackTrace();
	        }
	    }
	    return picDir;
	}
	

	private void executeWithTestImages(int[] resourceIds, String prefix)
	{
		File picDir = getPictureStorageDir();
		for (int i = 3; i < resourceIds.length; ++i)
		{
			try
			{
				File imgFile = new File(picDir, prefix + "-" + i + ".jpg");
				InputStream is = getResources().openRawResource(resourceIds[i]);
				OutputStream os = new FileOutputStream(imgFile);
				byte[] data = new byte[is.available()];
				is.read(data);
				os.write(data);
				is.close();
				os.close();
				inputFilePaths.add(imgFile.getPath());
			} catch (IOException e)
			{
				Log.e(TAG, "Failed to write provided mask!");
			}
		}
	}

	private void executeWithXMLs(int[] resourceIds, String prefix)
	{
		File xmlDir = getPictureStorageDir();
		inputFilePaths = new ArrayList<String>();
		for (int i = 0; i < 3; ++i)
		{
			try
			{
				File xmlFile = new File(xmlDir, prefix + "-" + i + ".xml");
				InputStream is = getResources().openRawResource(resourceIds[i]);
				OutputStream os = new FileOutputStream(xmlFile);
				byte[] data = new byte[is.available()];
				is.read(data);
				os.write(data);
				is.close();
				os.close();
				inputFilePaths.add(xmlFile.getPath());
			} catch (IOException e)
			{
				Log.e(TAG, "Failed to write provided xmls!");
			}
		}
	}
	
	private void handleTaskTrigger()
	{
		int[] testIds = {R.raw.haarcascade_frontalface_alt2, R.raw.haarcascade_mcs_eyepair_big, R.raw.haarcascade_mcs_nose, R.raw.mask2, R.raw.tongliya};
		executeWithXMLs(testIds, "TrackProcess");
		executeWithTestImages(testIds, "TrackProcess");
		inputPathsArray = inputFilePaths.toArray(new String[inputFilePaths.size()]);
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
			Toast.makeText(this, "Tap the screen to begin face detection.", Toast.LENGTH_LONG).show();
			mIsReadyForTracking = true;
			mIsReadyForDetection = false;
		} else {
			Toast.makeText(this, "Tap the screen to begin face tracking.", Toast.LENGTH_LONG).show();
			mIsReadyForDetection = true;
			mIsReadyForTracking = false;
			mTrRequiresInit = true;
		}
		return false;
	}

	@Override
	public void onCameraViewStarted(int width, int height) {
		// TODO Auto-generated method stub
		Toast.makeText(this, "Tap the screen to begin face detection.", Toast.LENGTH_LONG).show();
	}

	@Override
	public void onCameraViewStopped() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
		// TODO Auto-generated method stub
		Mat frame = inputFrame.rgba();
		int col = frame.cols();
		int row = frame.rows();
		String col_str = String.valueOf(col);
		String row_str = String.valueOf(row);
		Log.d(TAG, "rows = "+ row_str + ", cols = " + col_str);
		if (mIsReadyForDetection) {
			long startTime = System.currentTimeMillis();
			HandleFrame(mNativeController, frame.getNativeObjAddr(), mDeRequiresInit, true, inputPathsArray, mActiveTask.code);
			long endTime = System.currentTimeMillis();
			long runtime = endTime - startTime;
			String rt_str = String.valueOf(runtime);
			Log.d(TAG, "detection runtime = "+ rt_str);
			if (mDeRequiresInit) {
                mDeRequiresInit = false;
            }
		}
		
        if (mIsReadyForTracking) {
        	long startTime = System.currentTimeMillis();
            HandleFrame(mNativeController, frame.getNativeObjAddr(), mTrRequiresInit, false, inputPathsArray, mActiveTask.code);
            long endTime = System.currentTimeMillis();
			long runtime = endTime - startTime;
			String rt_str = String.valueOf(runtime);
			Log.d(TAG, "tracking runtime = "+ rt_str);
            if (mTrRequiresInit) {
                mTrRequiresInit = false;
            }
        }
        return frame;
	}
	
	
	static
    {
        if (!OpenCVLoader.initDebug())
        {
            Log.e(TAG, "Failed to initialize OpenCV!");
        }
        System.loadLibrary("HelloFace");
    }
	
	public native long CreateNativeController();

    public native void DestroyNativeController(long addr_native_controller);

    public native void HandleFrame(long addr_native_controller, long addr_rgba, boolean is_init, boolean is_detect, String[] file_path_array, int code);
}
