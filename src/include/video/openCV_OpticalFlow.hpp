/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

/*
openCV_OpticalFlow
Author: Florian Gross

*/

#ifndef OPENCV_OPTICALFLOW_HPP_
#define OPENCV_OPTICALFLOW_HPP_

#ifdef HAVE_OPENCV

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"


namespace OpticalFlow {

class FrameFtExtrParams{
public:
	//number of histogram bins for optical flow analysis
	int flo_bins;
	FLOAT_DMEM max_flow; //percentual maximum flow for histogram analysis (relative to image width)
	FLOAT_DMEM flow_downsample; //downsampling ratio for flow anaylsis (0.5 is half image size)
};

/**
 * @function computeOpticalFlowHistogram
 * @brief Computes the optical flow histogram.
 */
inline std::vector<FLOAT_DMEM> computeOpticalFlowHistogram(cv::Mat& frame_gray_resized, cv::Mat& prevFrame_gray_resized, FrameFtExtrParams& params, int display, int normalize)
{
	std::vector<FLOAT_DMEM> ft_vec;
	ft_vec.clear();


	// ### OPTICAL FLOW ANALYSIS ##########################################################################

	//calculate optical flow histogram using current frame and previous frame. If no previous frame found, output empty histogram of suitable size.
	//TODO: How to distinguish between pan/zoom and character movement?
	cv::MatND flo_hist;
	cv::Scalar mean_flow(0, 0);
	cv::Scalar stddev_flow(0, 0);

	FLOAT_DMEM px_nrm_fact_x = 1.0f / static_cast<FLOAT_DMEM>(frame_gray_resized.cols);
	FLOAT_DMEM px_nrm_fact_y = 1.0f / static_cast<FLOAT_DMEM>(frame_gray_resized.rows);
	int fhist_size[] = {params.flo_bins};
	FLOAT_DMEM f_ranges[] = {0.0, params.max_flow};
	const FLOAT_DMEM *fhist_ranges[] = {f_ranges};
	int fhist_channels[] = {0};
	if (!prevFrame_gray_resized.empty()){
		//std::cout << "Perform optical flow analysis\n";
		cv::Mat flow_img, flo_abs;
		calcOpticalFlowFarneback(frame_gray_resized, prevFrame_gray_resized, flow_img, 0.5, 5, 10, 10, 5, 1.0, cv::OPTFLOW_FARNEBACK_GAUSSIAN);

		//convert into an image of screen-relative displacements
		for (int cidx = 0; cidx < flow_img.cols; cidx++){
			for (int ridx = 0; ridx < flow_img.rows; ridx++){
				cv::Point2f pixgrad = flow_img.at<cv::Point2f>(ridx, cidx);
				pixgrad.x *= px_nrm_fact_x;
				pixgrad.y *= px_nrm_fact_y;
				flow_img.at<cv::Point2f>(ridx, cidx) = pixgrad;
			}
		}

		cv::meanStdDev(flow_img, mean_flow, stddev_flow);
		//std::cout << mean_flow << ", " << stddev_flow << "\n";
		flo_abs.create(flow_img.rows, flow_img.cols, CV_32FC1);
		FLOAT_DMEM max_x = 0;
		FLOAT_DMEM max_y = 0;
		//get amount of flow for each pixel
		for (int cidx = 0; cidx < flow_img.cols; cidx++){
			for (int ridx = 0; ridx < flow_img.rows; ridx++){
				cv::Point2f pixgrad = flow_img.at<cv::Point2f>(ridx, cidx);
				//pixgrad.x -= mean_flow.at<FLOAT_DMEM>(0);
				//pixgrad.y -= mean_flow.at<FLOAT_DMEM>(1);
				flo_abs.at<FLOAT_DMEM>(ridx, cidx) = sqrt(pixgrad.x*pixgrad.x + pixgrad.y*pixgrad.y);
				max_x = (std::max)(max_x, std::abs(pixgrad.x));
				max_y = (std::max)(max_y, std::abs(pixgrad.y));
			}
		}
		//std::cout << "Maximum gradients " << max_x << ", " << max_y << "\n";
		//TODO: Find absolute normalization scheme
		cv::Mat flo_nrm;
		cv::normalize(flo_abs, flo_nrm, 0, 255, cv::NORM_MINMAX, CV_8UC1);
		cv::Mat tmp_disp_img = flo_nrm.clone();

		if(display)
		{
			cv::imshow("DenseOpticalFlow", tmp_disp_img);
		}

		//get optical flow amount histogram
		cv::calcHist(&flo_abs, 1, fhist_channels, cv::Mat(), flo_hist, 1, fhist_size, fhist_ranges, true);
		
		if(normalize == 1)
		{
			cv::normalize(flo_hist, flo_hist, 1, 0, cv::NORM_L1, -1, cv::Mat()); // L1-norm
		}

		//std::vector<FLOAT_DMEM> flo_zero_hist(par.flo_bins, 0);
		//flo_hist = cv::Mat(flo_zero_hist);
	} else {
		std::cout << "Skip optical flow analysis, no previous frame stored\n";
		std::vector<FLOAT_DMEM> flo_zero_hist(params.flo_bins, 0);
		flo_hist = cv::Mat(flo_zero_hist);
	}
	ft_vec.push_back(mean_flow[0]);
	ft_vec.push_back(mean_flow[1]);
	ft_vec.push_back(stddev_flow[0]);
	ft_vec.push_back(stddev_flow[1]);

	for (int i = 0; i < params.flo_bins; i++){
		ft_vec.push_back(flo_hist.at<FLOAT_DMEM>(i));
	}

	return ft_vec;
}

} // End namespace OpticalFlow

#endif // HAVE_OPENCV

#endif /* OPENCV_OPTICALFLOW_HPP_ */
