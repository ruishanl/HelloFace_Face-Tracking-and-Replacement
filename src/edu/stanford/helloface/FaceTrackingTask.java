package edu.stanford.helloface;

import java.util.List;

import android.app.ProgressDialog;
import android.content.Context;
import android.os.AsyncTask;

public class FaceTrackingTask
{
	
	private static final int NATIVE_OP_CODE_NOMASK = 0;
	private static final int NATIVE_OP_CODE_CARTOON	= 1;
    private static final int NATIVE_OP_CODE_BLEND = 2;
	
    public enum TaskType
    {
        NO_MASK(NATIVE_OP_CODE_NOMASK, 0),
        CARTOON_MASK(NATIVE_OP_CODE_CARTOON, 3),
        BLEND_FACES(NATIVE_OP_CODE_BLEND, 3);

        public final int code;
        public final int requiredMaskCount;

        TaskType(int code, int choices)
        {
        	this.code = code;
            this.requiredMaskCount = choices;
        }
    }
    
    private List<String> mImagePaths;
    private TaskType mTaskType;
    private Context mContext;
    
    FaceTrackingTask(Context ctx, List<String> imagePaths, TaskType taskType)
    {
        this.mContext = ctx;
        this.mImagePaths = imagePaths;
        this.mTaskType = taskType;
    }
    

}