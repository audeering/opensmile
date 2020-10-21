/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

/*
openCV_LBPHistogram
Various helper functions for computing the LBP representation and histogram of an image.
Author: Florian Gross

*/

#ifndef OPENCV_HSVHISTOGRAM_HPP_
#define OPENCV_HSVHISTOGRAM_HPP_

#ifdef HAVE_OPENCV

#include <core/smileCommon.hpp>
#include <core/dataSource.hpp>

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <stdio.h>

namespace HSVHistogram {

/**
 * @function euclideanDistance
 * @brief Computes the euclidean distance of two points ( \f$ d = \sqrt{(x_2-x_1)^2+(y_2-y_1)^2}\f$ ).
 */
inline float euclideanDistance(cv::Point& p1, cv::Point& p2)
{
  float dx = p1.x - p2.x;
  float dy = p1.y - p2.y;
  return std::sqrt(dx * dx + dy * dy);
};

/**
 * @function computeSingleHistogram
 * @brief Computes a single histogram for a given single channel image (cv::Mat).
 * @param image Matrix representation of image, single channel
 * @return Histogram (cv::Mat)
 */
inline cv::Mat computeSingleHistogram(cv::Mat& input, int& histSize, const float range[], int normalize)
{
	cv::Mat retHist;
	cv::calcHist(&input, 1, 0, cv::Mat(), retHist, 1, &histSize, &range, true, false);

	if(normalize == 1)
	{
		cv::normalize(retHist, retHist, 1, 0, cv::NORM_L1, -1, cv::Mat()); // L1-norm
	}

	return retHist;
}

/**
 * @function computeHSVHistogram
 * @brief Computes the HSV histogram for a given image (cv::Mat).
 * @param image Matrix representation of image, BGR format
 * @return Array of histograms, ([0] => H, [1] => S, [2] => V)
 */
inline std::vector<cv::Mat> computeHSVHistogram(cv::Mat& image, int cfgHueSize, int cfgSatSize, int cfgValSize, int normalize)
{
	cv::Mat hsvImage;
	cv::cvtColor(image, hsvImage, CV_BGR2HSV);

	std::vector<cv::Mat> hsvPlanes;
	cv::split(hsvImage, hsvPlanes);

	const float hranges[] = { 0, 180 };
	const float sranges[] = { 0, 256 };
	const float vranges[] = { 0, 256 };

	cv::Mat h_hist = computeSingleHistogram(hsvPlanes[0], cfgHueSize, hranges, normalize);
	cv::Mat s_hist = computeSingleHistogram(hsvPlanes[1], cfgSatSize, sranges, normalize);
	cv::Mat v_hist = computeSingleHistogram(hsvPlanes[2], cfgValSize, vranges, normalize);

	std::vector<cv::Mat> hists_splitted;
	hists_splitted.push_back(h_hist);
	hists_splitted.push_back(s_hist);
	hists_splitted.push_back(v_hist);

	return hists_splitted;
}


} // End namespace LBPHistogram

#endif // HAVE_OPENCV

#endif /* OPENCV_HSVHISTOGRAM_HPP_ */
