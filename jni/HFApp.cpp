/*
 * HFApp.cpp
 *
 *  Created on: 2015Äê5ÔÂ26ÈÕ
 *      Author: meikylrs123
 */
#include "HFApp.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/ml/ml.hpp"
#include <vector>
#include <string>
#include "NativeLogging.hpp"

using namespace std;
using namespace cv;

static const char* TAG = "HFApp";

HFApp::HFApp()
{
    has_been_initialized_ = false;
    tracking = -1;
}

bool HFApp::initialize_detector(vector<string> file_path)
{
	if (!cascade.load(file_path[0]))// load classifier for face
	{
		LOG_ERROR(TAG, "ERROR: Could not load classifier cascade for face");
		return false;
	}

	if (!eyesCascade.load(file_path[1])) // load classifier for eyes
	{
		LOG_ERROR(TAG, "ERROR: Could not load classifier cascade for nested objects for eyes");
		return false;
	}

	if (!noseCascade.load(file_path[2])) // load classifier for nose
	{
		LOG_ERROR(TAG, "ERROR: Could not load classifier cascade for nested objects for nose");
		return false;
	}
	faces.clear();
	eyes.clear();
	nose.clear();
    has_been_initialized_ = true;
    return true;
}

void HFApp::initialize_tracker(vector<string> file_path)
{
	cartoon_mask = imread(file_path[3]);
	src_face_img = imread(file_path[4]);

	trackWindow = faces[0];
	histimg = Mat::zeros(200, 320, CV_8UC3);

	kpts_to_track.clear();
	get_kpts_refRect(trackWindow, prev_frame, kpts_to_track, face_ref_rect);
	overall_H = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };

	vector<Rect> src_face;
	vector<vector<Rect> > src_eyes;
	vector<vector<Rect> > src_nose;
	detect_at_frame(src_face_img, src_face, src_eyes, src_nose);
	Rect eye_region, nose_region;
	get_src_eyes_nose(src_face_img, prev_frame, src_face, src_eyes, src_nose, faces, eyes, nose, eye_region, nose_region);
	org_nose_mask = Mat::zeros(prev_frame.rows, prev_frame.cols, CV_32F);
	org_nose_mask(nose_region) = 1;
	org_eye_mask = Mat::zeros(prev_frame.rows, prev_frame.cols, CV_32F);
	org_eye_mask(eye_region) = 1;
	originalBlending(prev_frame, eye_region, nose_region);
}


void HFApp::face_detection(ColorImage& frame)
{
	faces.clear();
	eyes.clear();
	nose.clear();
	detect_at_frame(frame, faces, eyes, nose);
	frame.copyTo(prev_frame);
	draw_face_eyes(frame);
}

void HFApp::detect_at_frame(ColorImage& frame, vector<Rect>& faces, vector<vector<Rect> >& eyes, vector<vector<Rect> >& nose)
{
	int i = 0;
	double t = 0;
	Mat gray;
	cvtColor(frame, gray, CV_BGR2GRAY);
	equalizeHist(gray, gray); // histogram equalization
	// detect faces
	cascade.detectMultiScale(gray, faces, 3.0, 2, 0 | CV_HAAR_SCALE_IMAGE);
	for (vector<Rect>::const_iterator r = faces.begin(); r != faces.end(); r++, i++)
	{
		Mat ImgROI;
		ImgROI = gray(*r);

		// detect eyes
		if (eyesCascade.empty())
			continue;
		vector<Rect> eyeVec;
		eyesCascade.detectMultiScale(ImgROI, eyeVec, 1.1, 2);
		for (int i = 0; i < eyeVec.size(); i++) {
			eyeVec[i].x = cvRound(eyeVec[i].x - eyeVec[i].width * 0.1);
			eyeVec[i].y = cvRound(eyeVec[i].y + eyeVec[i].height * 0.1);
			eyeVec[i].width = cvRound(eyeVec[i].width * 1.2);
			eyeVec[i].height = cvRound(eyeVec[i].height * 0.9);
		}
		eyes.push_back(eyeVec);

		// detect nose
		if (noseCascade.empty())
			continue;
		vector<Rect> noseVec;
		noseCascade.detectMultiScale(ImgROI, noseVec, 3.0, 2);
		for (int i = 0; i < noseVec.size(); i++) {
			noseVec[i].x = cvRound(noseVec[i].x + noseVec[i].width * 0.02);
			noseVec[i].y = cvRound(noseVec[i].y - noseVec[i].height * 0.4);
			//noseVec[i].width = cvRound(noseVec[i].width * 1.2);
			noseVec[i].height = cvRound(noseVec[i].height * 1.1);
		}
		nose.push_back(noseVec);
	}
}

void HFApp::draw_face_eyes(ColorImage& frame)
{
	int i = 0;
		const static Scalar colors[] = { CV_RGB(0, 0, 255),
			CV_RGB(0, 128, 255),
			CV_RGB(0, 255, 255),
			CV_RGB(0, 255, 0),
			CV_RGB(255, 128, 0),
			CV_RGB(255, 255, 0),
			CV_RGB(255, 0, 0),
			CV_RGB(255, 0, 255) }; // different colors of faces

		for (vector<Rect>::const_iterator r = faces.begin(); r != faces.end(); r++, i++)
		{
			Point center;
			Scalar color = colors[i % 8];
			center.x = cvRound((r->x + r->width*0.5)); // zoom in to the original size
			center.y = cvRound((r->y + r->height*0.5));
			ellipse(frame, center, Size(r->width*0.4, r->height*0.45), 0, 0, 360, color, 3, 8, 0);

			for (vector<Rect>::const_iterator nr = eyes[i].begin(); nr != eyes[i].end(); nr++)
			{
				color = colors[(i + 1) % 8];
				center.x = cvRound((r->x + nr->x + nr->width*0.5));
				center.y = cvRound((r->y + nr->y + nr->height*0.5));
				ellipse(frame, center, Size(nr->width*0.5, nr->height*0.5), 0, 0, 360, color, 3, 8, 0);
			}

			for (vector<Rect>::const_iterator nr = nose[i].begin(); nr != nose[i].end(); nr++)
			{
				color = colors[(i + 2) % 8];
				center.x = cvRound((r->x + nr->x + nr->width*0.5));
				center.y = cvRound((r->y + nr->y + nr->height*0.5));
				ellipse(frame, center, Size(nr->width*0.5, nr->height*0.5), 0, 0, 360, color, 3, 8, 0);
			}
		}
}

void HFApp::originalBlending(ColorImage& frame, Rect eye_region, Rect nose_region) {
	Mat src_bld_img(frame.rows, frame.cols, frame.type(), Scalar::all(0));
	frame.copyTo(src_bld_img);
	Mat src;
	cvtColor(src_bld_img, src, CV_BGRA2BGR);

	Mat frame_BGR;
	cvtColor(frame, frame_BGR, CV_BGRA2BGR);
	src_nose_resized.copyTo(src(nose_region));
	src_eye_resized.copyTo(src(eye_region));
	src.copyTo(org_src_img);
	Mat nose_blending;
	pyramidBlend(src, frame_BGR, nose_blending, org_nose_mask);
	Mat eye_blending;
	pyramidBlend(src, nose_blending, eye_blending, org_eye_mask);
}

void HFApp::pyramidBlend(Mat src, Mat tgt, Mat& outputImage, Mat mask)
{
	vector<vector<Mat> > lapPyramid;
	Mat l_im, r_im;

	int maxPyrIndex = 3;
	src.convertTo(l_im, CV_32FC3, 1.0 / 255.0);
	tgt.convertTo(r_im, CV_32FC3, 1.0 / 255.0);

	vector<Mat> maskGP; // Guassian Pyramid of mask
	buildPyramid(mask, maskGP, maxPyrIndex, BORDER_DEFAULT);

	vector<Mat> l_LP, r_LP; // Laplacian Pyramids of two images
	vector<Mat> l_GP, r_GP; // Gaussian Pyramids of images
	vector<Mat> LP; // Laplacian Pyramid of blending image
	buildPyramid(l_im, l_GP, maxPyrIndex, BORDER_DEFAULT);
	buildPyramid(r_im, r_GP, maxPyrIndex, BORDER_DEFAULT);

	l_LP = l_GP;
	r_LP = r_GP;
	for (unsigned int l = 0; l < maxPyrIndex; l++) {
		Mat l_upim, r_upim;
		pyrUp(l_GP[l + 1], l_upim, Size(l_LP[l].cols, l_LP[l].rows));
		l_LP[l] -= l_upim;
		pyrUp(r_GP[l + 1], r_upim, Size(r_LP[l].cols, r_LP[l].rows));
		r_LP[l] -= r_upim;
	}

	// Compute Laplacian Pyramid for blending image
	for (unsigned int l = 0; l <= maxPyrIndex; l++) {
		vector<Mat> l_ch, r_ch; // l_ch, r_ch are the three channels of l_LP[l] and r_LP[l]
		split(l_LP[l], l_ch);
		split(r_LP[l], r_ch);
		// following youwenti
		Mat LP_levellC0, LP_levellC1, LP_levellC2; // three channels of Laplacian Pyramid of blending image
		LP_levellC0 = maskGP[l].mul(l_ch[0]) + (1 - maskGP[l]).mul(r_ch[0]);
		LP_levellC1 = maskGP[l].mul(l_ch[1]) + (1 - maskGP[l]).mul(r_ch[1]);
		LP_levellC2 = maskGP[l].mul(l_ch[2]) + (1 - maskGP[l]).mul(r_ch[2]);

		vector<Mat> LP_allch;
		LP_allch.push_back(LP_levellC0);
		LP_allch.push_back(LP_levellC1);
		LP_allch.push_back(LP_levellC2);
		Mat LP_levell;
		merge(LP_allch, LP_levell);
		LP.push_back(LP_levell);
	}

	// Construct resulting image
	for (unsigned int l = maxPyrIndex; l > 0; l--) {
		Mat upimg;
		pyrUp(LP[l], upimg, Size(LP[l - 1].cols, l_LP[l - 1].rows));
		LP[l - 1] += upimg;
	}

	LP[0].convertTo(outputImage, tgt.type(), 255);
}

void HFApp::get_kpts_refRect(Rect trackWindow, Mat prev_frame, PointArray &kpts_to_track, PointArray &face_ref_rect)
{
	RectToPointArray(trackWindow, face_ref_rect);
	GrayscaleImage prev_gray;
	cvtColor(prev_frame, prev_gray, CV_BGR2GRAY);
	PointArray keypoints;
	int max_points = 500;
	KLT_init(prev_gray, keypoints, max_points);

	for (int i = 0; i < keypoints.size(); i++) {
		if (keypoints[i].x >= trackWindow.x
			&& keypoints[i].x <= trackWindow.x + trackWindow.width
			&& keypoints[i].y >= trackWindow.y
			&& keypoints[i].y <= trackWindow.y + trackWindow.height) {
			kpts_to_track.push_back(keypoints[i]);
		}
	}
}

void HFApp::get_src_eyes_nose(Mat src_img, Mat frame, vector<Rect> src_face, vector<vector<Rect> > src_eyes, vector<vector<Rect> > src_nose,
	vector<Rect> faces, vector<vector<Rect> > eyes, vector<vector<Rect> > nose, Rect &eye_region, Rect &nose_region) {

	if (eyes[0].empty()) {
		eye_region.x = faces[0].x + 0.15 * faces[0].width;
		eye_region.y = faces[0].y + 0.33 * faces[0].height;
		eye_region.width = 0.7 * faces[0].width;
		eye_region.height = 0.16 * faces[0].height;
	}
	else
	{
		eye_region.x = faces[0].x + eyes[0][0].x;
		eye_region.y = faces[0].y + eyes[0][0].y;
		eye_region.width = eyes[0][0].width;
		eye_region.height = eyes[0][0].height;
	}
	Rect src_eye_roi;
	src_eye_roi.x = src_face[0].x + src_eyes[0][0].x;
	src_eye_roi.y = src_face[0].y + src_eyes[0][0].y;
	src_eye_roi.width = src_eyes[0][0].width;
	src_eye_roi.height = src_eyes[0][0].height;
	Mat eye_srcimg = src_img(src_eye_roi);

	resize(eye_srcimg, src_eye_resized, Size(eye_region.width, eye_region.height));

	if (nose[0].empty()) {
		nose_region.x = faces[0].x + 0.4 * faces[0].width;
		nose_region.y = faces[0].y + 0.5 * faces[0].height;
		nose_region.width = 0.25 * faces[0].width;
		nose_region.height = 0.25 * faces[0].height;
	}
	else
	{
		nose_region.x = faces[0].x + nose[0][0].x;
		nose_region.y = faces[0].y + nose[0][0].y;
		nose_region.width = nose[0][0].width;
		nose_region.height = nose[0][0].height;
	}
	Rect src_nose_roi;
	src_nose_roi.x = src_face[0].x + src_nose[0][0].x;
	src_nose_roi.y = src_face[0].y + src_nose[0][0].y;
	src_nose_roi.width = src_nose[0][0].width;
	src_nose_roi.height = src_nose[0][0].height;
	Mat nose_srcimg = src_img(src_nose_roi);
	resize(nose_srcimg, src_nose_resized, Size(nose_region.width, nose_region.height));
	changeFaceColor(src_eye_resized, src_nose_resized, frame(eye_region), frame(nose_region));
}

void HFApp::changeFaceColor(Mat &src_eyes, Mat &src_nose, Mat tgt_eyes, Mat tgt_nose)
{
	float gamma = 1.5;
	vector<Mat> src_eyes_BGRchs;
	vector<Mat> src_nose_BGRchs;
	src_eyes.convertTo(src_eyes, CV_32FC3, 1.0 / 255.0);
	src_nose.convertTo(src_nose, CV_32FC3, 1.0 / 255.0);
	split(src_eyes, src_eyes_BGRchs);
	split(src_nose, src_nose_BGRchs);

	for (int i = 0; i < 3; i++) {
		pow(src_eyes_BGRchs[i], gamma, src_eyes_BGRchs[i]);
		pow(src_nose_BGRchs[i], gamma, src_nose_BGRchs[i]);
	}
	merge(src_eyes_BGRchs, src_eyes);
	merge(src_nose_BGRchs, src_nose);
	src_eyes.convertTo(src_eyes, CV_8UC3, 255);
	src_nose.convertTo(src_nose, CV_8UC3, 255);

	Scalar src_nose_mean = mean(src_nose);
	Scalar tgt_nose_mean = mean(tgt_nose);
	Scalar colordiff = (tgt_nose_mean - src_nose_mean) * 0.85;
	inRange(src_eyes, src_nose_mean - Scalar(100, 100, 100), src_nose_mean + Scalar(50, 40, 40), org_eye_mask);
	vector<Mat> src_eyes_chs;
	split(src_eyes, src_eyes_chs);
	cout << colordiff[0] << endl;
	addWeighted(org_eye_mask, colordiff[0] / 255, src_eyes_chs[0], 1, 0, src_eyes_chs[0]);
	addWeighted(org_eye_mask, colordiff[1] / 255, src_eyes_chs[1], 1, 0, src_eyes_chs[1]);
	addWeighted(org_eye_mask, colordiff[2] / 255, src_eyes_chs[2], 1, 0, src_eyes_chs[2]);
	merge(src_eyes_chs, src_eyes);
	src_nose += colordiff;
}

void HFApp::KLT_init(GrayscaleImage frame, PointArray& keypoints, int max_points_)
{
	if (keypoints.empty())
	{
		const bool use_my_harris_detector = false;
		double qualityLevel = 0.01;
		double minDistance = 10;
		int blockSize = 3;
		bool useHarrisDetector = false;
		double k = 0.04;
		goodFeaturesToTrack(frame, keypoints, max_points_, qualityLevel, minDistance, Mat(), blockSize,
			useHarrisDetector, k);
		cornerSubPix(frame, keypoints, Size(3, 3), Size(-1, -1),
			TermCriteria(TermCriteria::MAX_ITER + TermCriteria::EPS, 30, 0.1));
	}
}

void HFApp::RectToPointArray(Rect src, PointArray &dst)
{
	Point2f p1, p2, p3, p4;
	p1.x = (float)src.x, p1.y = (float)src.y;
	p2.x = (float)p1.x + src.width, p2.y = (float)p1.y;
	p3.x = (float)p2.x, p3.y = (float)p1.y + src.height;
	p4.x = (float)p1.x, p4.y = (float)p3.y;
	dst.push_back(p1);
	dst.push_back(p2);
	dst.push_back(p3);
	dst.push_back(p4);
}

void HFApp::face_tracking(ColorImage& frame, int option)
{
	Mat image;
	cvtColor(frame, image, CV_BGRA2BGR);
	trackWindow = trackFace(image, trackWindow, "face", hsv, hue, mask, hist, histimg, backproj, option);

	KLTtrack(prev_frame, frame, kpts_to_track, next_pts);
	frame.copyTo(prev_frame);

	if (kpts_to_track.size() >= 8) {
		//estimatehomography
		Homography H;
		bool estimate_success = estimate_homography(kpts_to_track, next_pts, H);
		PointArray face_dst_rect_vtx;
		Rect face_dst_rect;
		if (estimate_success) {
			overall_H = H * overall_H;
			perspectiveTransform(face_ref_rect, face_cur_rect, H);
			to_rect(face_cur_rect, face_dst_rect_vtx, face_dst_rect);
			if (option == 2) {
				Mat org_src_img_warpped, org_nose_mask_warpped, org_eye_mask_warpped;
				warpPerspective(org_src_img, org_src_img_warpped, overall_H, Size(org_src_img.cols, org_src_img.rows));
				warpPerspective(org_nose_mask, org_nose_mask_warpped, overall_H, Size(org_nose_mask.cols, org_nose_mask.rows));
				warpPerspective(org_eye_mask, org_eye_mask_warpped, overall_H, Size(org_eye_mask.cols, org_eye_mask.rows));
				Mat nose_blending;
				pyramidBlend(org_src_img_warpped, image, nose_blending, org_nose_mask_warpped);
				pyramidBlend(org_src_img_warpped, nose_blending, image, org_eye_mask_warpped);
			}

			face_ref_rect = face_dst_rect_vtx;
			kpts_to_track = next_pts;
			}
			cvtColor(image, frame, CV_BGR2BGRA);
	}
}

void HFApp::KLTtrack(Mat prev_frame, Mat frame, PointArray& prev_pts, PointArray &next_pts)
{
	// Step 1 : Compute optical flow.
	Mat prev_gray, frame_gray;
	cvtColor(prev_frame, prev_gray, CV_BGR2GRAY);
	cvtColor(frame, frame_gray, CV_BGR2GRAY);
	vector<uchar> status;
	vector<float> err;
	int maxLevel = 3;
	Size winsize = Size(9, 9); // win_size_ = Size(9, 9)
	calcOpticalFlowPyrLK(prev_frame, frame, prev_pts, next_pts, status, err, winsize, maxLevel);

	// Step 2 : Prune points based on the status returned by calcOpticalFlowPyrLK.
	PointArray prev_pts1;
	PointArray cur_pts;
	for (int i = 0; i < status.size(); i++) {
		if ((int)status[i] != 0) {
			cur_pts.push_back(next_pts[i]);
			prev_pts1.push_back(prev_pts[i]);
		}
	}
	prev_pts = prev_pts1;
	next_pts = cur_pts;
}

bool HFApp::estimate_homography(PointArray src_points, PointArray dst_points, Homography& H)
{
	float thresh = 0.4;
	std::vector<uchar> mask;
	H = findHomography(src_points, dst_points, CV_RANSAC, 3, mask);
	int numInliers = 0;
	// Store inliners in vector keypoints
	for (int i = 0; i < src_points.size(); i++) {
		if ((int)mask[i] == 1) {
			numInliers++;
		}
	}
	float ratio = (float)numInliers / src_points.size();
	if (ratio >= thresh) {
		return true;
	}
	return false;
}

void HFApp::to_rect(PointArray &src, PointArray &dst, Rect &dst_rect) {
	Point2f c1, c2, c3, c4;
	c1.x = (src[0].x + src[1].x) / 2;
	c1.y = (src[0].y + src[1].y) / 2;
	c2.x = (src[1].x + src[2].x) / 2;
	c2.y = (src[1].y + src[2].y) / 2;
	c3.x = (src[2].x + src[3].x) / 2;
	c3.y = (src[2].y + src[3].y) / 2;
	c4.x = (src[3].x + src[0].x) / 2;
	c4.y = (src[3].y + src[0].y) / 2;
	float minx = min(min(min(c1.x, c2.x), c3.x), c4.x);
	float miny = min(min(min(c1.y, c2.y), c3.y), c4.y);
	float maxx = max(max(max(c1.x, c2.x), c3.x), c4.x);
	float maxy = max(max(max(c1.y, c2.y), c3.y), c4.y);
	Point2f r1, r2, r3, r4;
	r1.x = minx, r1.y = miny;
	r2.x = maxx, r2.y = miny;
	r3.x = maxx, r3.y = maxy;
	r4.x = minx, r4.y = maxy;
	dst.push_back(r1);
	dst.push_back(r2);
	dst.push_back(r3);
	dst.push_back(r4);
	dst_rect.x = minx;
	dst_rect.y = miny;
	dst_rect.width = maxx - minx;
	dst_rect.height = maxy - miny;
}

void HFApp::putMask(Mat mask_RGB, Mat& src, Rect face)
{
	Mat mask;
	cvtColor(mask_RGB, mask, CV_RGB2BGR);
	Mat mask0;
	resize(mask, mask0, Size(face.width, face.height));

	Rect face_mask(0, 0, face.width, face.height);
	if (face.x < 0) {
		face.width += face.x;
		face_mask.x = 0 - face.x;
		face.x = 0;
	}
	if (face.y < 0) {
		face.height += face.y;
		face_mask.y = 0 - face.y;
		face.y = 0;
	}
	if (face.x >= src.cols - face.width) {
		face.width = src.cols - face.x;
	}
	if (face.y >= src.rows - face.height) {
		face.height = src.rows - face.y;
	}
	face_mask.width = face.width;
	face_mask.height = face.height;

	Mat mask1;
	mask0(face_mask).copyTo(mask1);

	Mat src1(mask1.rows, mask1.cols, CV_8UC3);

	// ROI selection
	src(face).copyTo(src1);


	// to make the white region transparent
	Mat mask2, m, m1;
	cvtColor(mask1, mask2, CV_BGR2GRAY);
	threshold(mask2, mask2, 245, 255, CV_THRESH_BINARY_INV);
	vector<Mat> maskChannels(3), result_mask(3);
	split(mask1, maskChannels);
	bitwise_and(maskChannels[0], mask2, result_mask[0]);
	bitwise_and(maskChannels[1], mask2, result_mask[1]);
	bitwise_and(maskChannels[2], mask2, result_mask[2]);
	merge(result_mask, m);      //       imshow("m",m);

	mask2 = 255 - mask2;
	vector<Mat> srcChannels(3);
	split(src1, srcChannels);
	bitwise_and(srcChannels[0], mask2, result_mask[0]);
	bitwise_and(srcChannels[1], mask2, result_mask[1]);
	bitwise_and(srcChannels[2], mask2, result_mask[2]);
	merge(result_mask, m1);        //    imshow("m1",m1);

	addWeighted(m, 1, m1, 1, 0, m1);    //    imshow("m2",m1);
	m1.copyTo(src(face));
}

Rect HFApp::trackFace(Mat& image, Rect trackWindow, String feature, Mat& hsv, Mat& hue, Mat& mask, Mat& hist, Mat& histimg, Mat& backproj, int option)
{
	float hranges[] = {0,180};
    const float* phranges = hranges;
	int hsize = 16;
	int vmin = 10, vmax = 256, smin = 30;

	cvtColor(image, hsv, COLOR_BGR2HSV);
	inRange(hsv, Scalar(0, smin, MIN(vmin,vmax)), Scalar(180, 256, MAX(vmin, vmax)), mask);

	int ch[] = {0, 0};
	hue.create(hsv.size(), hsv.depth());
	mixChannels(&hsv, 1, &hue, 1, ch, 1);

	if (tracking < 0) {
		Mat roi(hue, trackWindow), maskroi(mask, trackWindow);
		calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
		normalize(hist, hist, 0, 255, NORM_MINMAX);
		tracking = 1;
		histimg = Scalar::all(0);
        int binW = histimg.cols / hsize;
        Mat buf(1, hsize, CV_8UC3);
        for( int i = 0; i < hsize; i++ )
		buf.at<Vec3b>(i) = Vec3b(saturate_cast<uchar>(i*180./hsize), 255, 255);
        cvtColor(buf, buf, COLOR_HSV2BGR);
	}

	calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
	backproj &= mask;
	RotatedRect trackBox = CamShift(backproj, trackWindow, TermCriteria(TermCriteria::EPS | TermCriteria::COUNT, 10, 1));

	trackWindow = trackBox.boundingRect();

	float coefh = 0.9, coefw = 0.8, coefy = 0.5, coefx = 0.5;
	if (feature == "eyes") {
		coefh = 0.5;
		coefw = 0.85;
		coefy = 0.4;
		coefx = 0.35;
	}
	if (option == 1) {
		putMask(cartoon_mask, image, trackWindow);
	}
	trackWindow.height = trackBox.size.width * coefh;
	trackWindow.y = trackBox.center.y - trackWindow.height * coefy;
	trackWindow.width = trackBox.size.height * coefw;
	trackWindow.x = trackBox.center.x - trackWindow.width * coefx;

	if (option == 0) {
		rectangle(image, trackWindow, Scalar(255,0,0));
	}
	return trackWindow;
}
