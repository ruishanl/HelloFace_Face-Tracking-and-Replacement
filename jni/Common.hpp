/*
 * Common.hpp
 *
 *  Created on: 2015Äê5ÔÂ26ÈÕ
 *      Author: meikylrs123
 */

#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <opencv2/opencv.hpp>
#include <cassert>
#include <functional>
#include <memory>



typedef cv::Matx33f Homography;

typedef cv::Mat ColorImage;

typedef cv::Mat GrayscaleImage;

typedef std::vector<cv::Point2f> PointArray;

typedef std::function<void(const PointArray&, const PointArray&)> MatchHandler;



#endif /* COMMON_HPP_ */
