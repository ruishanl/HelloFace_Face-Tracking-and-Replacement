/*
 * HelloFace.cpp
 *
 *  Created on: 2015Äê5ÔÂ26ÈÕ
 *      Author: meikylrs123
 */
#include <jni.h>
#include "HelloFace.hpp"
#include "HFApp.hpp"
#include <string>
#include <vector>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

static string convert_to_string(JNIEnv* env, jstring js)
{
    const char* stringChars = env->GetStringUTFChars(js, 0);
    string s = string(stringChars);
    env->ReleaseStringUTFChars(js, stringChars);
    return s;
}

/*
 * Class:     edu_stanford_helloface_HelloFaceActivity
 * Method:    CreateNativeController
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_edu_stanford_helloface_HelloFaceActivity_CreateNativeController
  (JNIEnv *, jobject)
{
	return (jlong)(new HFApp);
}

/*
 * Class:     edu_stanford_helloface_HelloFaceActivity
 * Method:    DestroyNativeController
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_edu_stanford_helloface_HelloFaceActivity_DestroyNativeController
  (JNIEnv *, jobject, jlong addr_native_controller)
{
	delete (HFApp*)(addr_native_controller);
}
/*
 * Class:     edu_stanford_helloface_HelloFaceActivity
 * Method:    HandleFrame
 * Signature: (JJZZLjava/util/List;)V
 */
JNIEXPORT void JNICALL Java_edu_stanford_helloface_HelloFaceActivity_HandleFrame
  (JNIEnv *env, jobject, jlong addr_native_controller, jlong addr_rgba, jboolean is_init, jboolean is_detect, jobjectArray file_path_array, jint code)
{
	vector<string> file_path;
	int fileCount = env->GetArrayLength(file_path_array);
	for(int i = 0; i < fileCount; ++i)
	{
	   jstring js = (jstring) (env->GetObjectArrayElement(file_path_array, i));
	   file_path.push_back(convert_to_string(env, js));
	}
	HFApp* app = (HFApp*)(addr_native_controller);
	cv::Mat* frame = (cv::Mat*)(addr_rgba);
	int option = code;
	if (is_detect) {
		if (is_init) {
			app->initialize_detector(file_path);
		}
		app->face_detection(*frame);
	} else {
		if (is_init) {
			app->initialize_tracker(file_path);
		}
		app->face_tracking(*frame, option);
	}

}

#ifdef __cplusplus
}
#endif



