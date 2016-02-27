/*
 * HFApp.hpp
 *
 *  Created on: 2015Äê5ÔÂ26ÈÕ
 *      Author: meikylrs123
 */

#ifndef HFAPP_HPP_
#define HFAPP_HPP_

#include "Common.hpp"

using namespace cv;
using namespace std;

class HFApp
{
public:

    HFApp();

    virtual ~HFApp() = default;

    bool initialize_detector(vector<string> file_path);

    void initialize_tracker(vector<string> file_path);

    void face_detection(ColorImage& frame);

    void face_tracking(ColorImage& frame, int option);


private:

    bool has_been_initialized_;

    CascadeClassifier cascade;

    CascadeClassifier eyesCascade;

    CascadeClassifier noseCascade;

    Mat prev_frame;

    vector<Rect> faces;

    vector<vector<Rect> > eyes;

    vector<vector<Rect> > nose;

    Mat cartoon_mask;

    Mat src_face_img;

    Rect trackWindow;

    Mat hsv, hue, mask, hist, histimg, backproj;

    int tracking;

    PointArray face_ref_rect;
    PointArray face_cur_rect;
    PointArray kpts_to_track;
    PointArray next_pts;

    Homography overall_H;

    Mat src_eye_resized, src_nose_resized;
    Mat org_src_img, org_nose_mask, org_eye_mask;

    void detect_at_frame(ColorImage& frame, vector<Rect>& faces, vector<vector<Rect> >& eyes, vector<vector<Rect> >& nose);

    void draw_face_eyes(ColorImage& frame);

    void originalBlending(ColorImage& frame, Rect eye_region, Rect nose_region);

    void pyramidBlend(Mat src, Mat tgt, Mat& outputImage, Mat mask);

    void get_kpts_refRect(Rect trackWindow, Mat prev_frame, PointArray &kpts_to_track, PointArray &face_ref_rect);

    void get_src_eyes_nose(Mat src_img, Mat frame, vector<Rect> src_face, vector<vector<Rect> > src_eyes, vector<vector<Rect> > src_nose,
        	vector<Rect> faces, vector<vector<Rect> > eyes, vector<vector<Rect> > nose, Rect &eye_region, Rect &nose_region);

    void changeFaceColor(Mat &src_eyes, Mat &src_nose, Mat tgt_eyes, Mat tgt_nose);

    void KLT_init(GrayscaleImage frame, PointArray& keypoints, int max_points_);

    void RectToPointArray(Rect src, PointArray &dst);

    void KLTtrack(Mat prev_frame, Mat frame, PointArray& prev_pts, PointArray &next_pts);

    bool estimate_homography(PointArray src_points, PointArray dst_points, Homography& H);

    void to_rect(PointArray &src, PointArray &dst, Rect &dst_rect);

    void putMask(Mat mask_RGB, Mat& src, Rect face);

    Rect trackFace(Mat& image, Rect trackWindow, String feature, Mat& hsv, Mat& hue, Mat& mask, Mat& hist, Mat& histimg, Mat& backproj, int option);

};



#endif /* HFAPP_HPP_ */
