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

#ifndef OPENCV_LBPHISTOGRAM_HPP_
#define OPENCV_LBPHISTOGRAM_HPP_

#ifdef HAVE_OPENCV

#include <core/smileCommon.hpp>
#include <core/dataSource.hpp>

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <stdio.h>

namespace LBPHistogram {

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
 * @function computeLBPImage_
 * @brief Creates an image representation of the LBP value for each pixel. (by binarizing...)
 * @param src Source Image (@link cv::Mat @endlink)
 */
template<class Elem, class Word>
inline cv::Mat computeLBPImage_(cv::Mat& src)
{
	cv::Mat dst = cv::Mat::zeros(src.size(), CV_8UC1);
	for (int i = 1; i < src.rows - 1; ++i)
    {
        for (int j = 1; j < src.cols - 1; ++j)
        {
            std::vector<Elem> neighbors(8);
            neighbors[0] = src.at<Elem>(i,     j + 1);
            neighbors[1] = src.at<Elem>(i + 1, j + 1);
            neighbors[2] = src.at<Elem>(i + 1, j);
            neighbors[3] = src.at<Elem>(i + 1, j - 1);
            neighbors[4] = src.at<Elem>(i,     j - 1);
            neighbors[5] = src.at<Elem>(i - 1, j - 1);
            neighbors[6] = src.at<Elem>(i - 1, j);
            neighbors[7] = src.at<Elem>(i - 1, j + 1);
            Elem c = src.at<Elem>(i, j);
            Word word = 0;
            for (size_t k = 0; k < neighbors.size(); ++k)
            {
                if (k > 0)
                    word = word << 1;
                unsigned char bit = neighbors[k] > c;
                //cout << bit - 0;
                //cout << "val = " << neighbors[k] - 0 << "; thr = " << bit - 0 << endl;
                word |= bit;
                //cout << "word = " << word  - 0 << endl;
            }
            //cout << endl << "(" << i << "," << j << "): " << "word = " << word - 0 << endl;
            /*if (ntrans > 2)
                cout << "non-uniform!" << endl;*/
            // 85 is word representation of 01010101 (non-uniform pattern)
            dst.at<Word>(i, j) = word;
//            dstUnif.at<Word>(i, j) = ntrans > 2 ? 85 : word;
        }
    }
    return dst;
};

/**
 * @function computeLBPImage
 * @brief Wrapper for computeLBPImage_ with standard types <unsigned char, unsigned char>
 */
inline cv::Mat computeLBPImage(cv::Mat& src)
{
	return computeLBPImage_<unsigned char, unsigned char>(src);
};

/**
 * @function lbp_hist
 * @brief Creates an LBP Histogram.
 * @param src LBP image representation (see computeLBPImage)
 * @param dst Destination histogram
 * @param wordMap Mapping to uniform or identity patterns
 */
template<class Word, class Cnt>
inline void lbp_hist_(cv::Mat& src, std::map<Word, Cnt>& dst, const std::map<Word, Word> &wordMap)
{
    dst.clear();
    for (typename std::map<Word, Word>::const_iterator itr = wordMap.begin();
         itr != wordMap.end(); ++itr)
    {
	//printf("ITR->second: %i\n", itr->second);
        dst[itr->second] = 0;
    }
    printf("Fertig!\n");
    for (int i = 0; i < src.rows; ++i)
    {
        for (int j = 0; j < src.cols; ++j)
        {
            Word word = src.at<Word>(i, j);
            typename std::map<Word, Word>::const_iterator itr = wordMap.find(word);
            if (itr != wordMap.end())
            {
                //std::cout << "converting " << itr->first - 0 << " to " << itr->second - 0 << std::endl;
                ++dst[itr->second];
            }
            else
            {
                ++dst[word];
            }
        }
    }
};

/**
 * @function lbp_hist_add
 * @brief Add values of histogram bins of op1 in op2 to op1
 * @param op1 First histogram
 * @param op2 Second histogram
 */
// add values of histogram bins of op1 in op2 to op1
template<class Word, class Cnt>
inline void lbp_hist_add_(std::map<Word, Cnt>& op1, const std::map<Word, Cnt>& op2)
{
    for (typename std::map<Word, Cnt>::const_iterator itr = op2.begin();
         itr != op2.end(); ++itr)
    {
        /*typename map<Word, Cnt>::const_iterator
        itrF = op2.find(itr->first);
        if (itrF != op2.end())
          op1[itr->first] += itrF->second;
        else
        {
          cerr << "WARNING: histogram bin " << itr->first
               << " not found in addition!" << endl;
        }*/
        op1[itr->first] += itr->second;
    }
};

/**
 * @function lbp_hist_normalize
 * @brief Normalizes the LBP Histogram.
 * @param op LBP histogram
 */
template<class Word, class Cnt>
inline void lbp_hist_normalize_(std::map<Word, Cnt>& op)
{
    Cnt total = 0;
    for (typename std::map<Word, Cnt>::const_iterator itr = op.begin();
         itr != op.end(); ++itr)
    {
        total += itr->second;
    }
    for (typename std::map<Word, Cnt>::iterator itr = op.begin();
         itr != op.end(); ++itr)
    {
        itr->second /= total;
    }
};

/**
 * @function computeLBPHistogram_
 * @brief Template function for complete computation of LBP histograms, either with uniform patterns mapping or without
 * @param src LBP image representation (see computeLBPImage)
 * @param wordMap Mapping to uniform or identity patterns
 */
template<class Word, class Cnt>
inline std::map<Word, Cnt> computeLBPHistogram_(cv::Mat& src, std::map<unsigned char, unsigned char>& wordMap, int normalize)
{
	std::map<Word, Cnt> lbpHist;
	
	lbp_hist_<Word, Cnt>(src, lbpHist, wordMap); // Create the histogram
	
	//lbp_hist_add_<Word, Cnt>(lbpHistAll, lbpHist); // Add to overall histogram, perhaps for averaging over time?
	
	if(normalize == 1)
	{
		lbp_hist_normalize_<Word, Cnt>(lbpHist); // Normalize the histogram
	}
	
	return lbpHist;
};

/**
 * @function computeLBPHistogram
 * @brief Wrapper for computeLBPHistogram_ with standard types <unsigned char,float>
 */
inline std::map<unsigned char, FLOAT_DMEM> computeLBPHistogram(cv::Mat& src, std::map<unsigned char, unsigned char>& wordMap, int normalize)
{
	return computeLBPHistogram_<unsigned char, FLOAT_DMEM>(src, wordMap, normalize);
};

/**
 * @function compute_uniform_map_
 * @brief ???
 * @return Map with uniform pattern words
 */
template<class Word>
inline std::map<Word, Word> compute_uniform_map_()
{
	std::map<Word, Word> m;
    m.clear();
    int maxWord = (1 << (sizeof(Word) * 8)) - 1;
    int nextIdx = 0;
    std::vector<Word> nonunif;
    for (int word = 0; word <= maxWord; ++word)
    {
        //std::cout << "word " << word - 0 << std::endl;
        int wordTmp = word;
        int ntrans = 0;
        unsigned char lastBit;
        for (int bit = 0; bit < sizeof(Word) * 8; ++bit)
        {
            // get LSB
            unsigned char bitValue = wordTmp & 1;
            //std::cout << bitValue - 0;
            // shift right
            wordTmp = wordTmp >> 1;
            if (bit > 0 && bitValue != lastBit)
            {
                ++ntrans;
            }
            lastBit = bitValue;
        }
        //std::cout << std::endl;
        if (ntrans > 2)
        {
            nonunif.push_back(word);
            //std::cout << "non-uniform word: " << word << " (ntrans = " << ntrans << ")" << std::endl;
        }
        else if (m.find(word) == m.end())
        {
            m[word] = nextIdx;
            //std::cout << "map word " << word << " to " << nextIdx << std::endl;
            ++nextIdx;
        }
    }
    for (size_t k = 0; k < nonunif.size(); ++k)
    {
        m[nonunif[k]] = nextIdx;
        //std::cout << "map non-uniform word " << nonunif[k] - 0 << " to " << nextIdx << std::endl;
    }
    return m;
};

/**
 * @function compute_identity_map_
 * @brief ???
 * @return Map with identity pattern words
 */
template<class Word>
inline std::map<Word, Word> compute_identity_map_()
{
	std::map<Word, Word> m;
    m.clear();
    int maxWord = (1 << (sizeof(Word) * 8)) - 1;
    for (int word = 0; word <= maxWord; ++word)
    {
        m[word] = word;
    }
    return m;
};

/**
 * @function cropFace
 * @brief Scales and rotates image, so that a image of the upright head of the size dstSize is returned.
 */
inline void cropFace(const cv::Mat& src, cv::Mat& dst, cv::Point& leftEye, cv::Point& rightEye, float offsetXPct, float offsetYPct, cv::Size& dstSize)
{
  float offsetX = std::floor(offsetXPct * dstSize.width);
  float offsetY = std::floor(offsetYPct * dstSize.height);
  //float eyeDirX = rightEye.x - leftEye.x;
  //float eyeDirY = rightEye.y - leftEye.y;
  float rotation = std::atan2(float(rightEye.y - leftEye.y), float(rightEye.x - leftEye.x));
  float rotationDeg = rotation * 180.0 / M_PI;
  float dist = euclideanDistance(leftEye, rightEye);
  float refDist = float(dstSize.width) - 2.0 * offsetX;
  float scale = dist / refDist;
  // cout << "dist = " << dist << endl;
  //float scale = dist / (0.8 * float(dstSize.width));
  /*float cosine = std::cos(rotation);
  float sine = std::sin(rotation);*/
  /*float rotMatData[] = {
    cosine,
    sine,
    leftEye.x - leftEye.x * cosine - leftEye.y * sine,
    -sine,
    cosine,
    leftEye.y + leftEye.x * sine - leftEye.y * cosine
  };
  cv::Mat rotMat(2, 3, CV_32F, rotMatData);*/
  // std::cout << "rotation = " << rotation << "; scale = " << scale << std::endl;
  //std::cout << rotMat << std::endl;
  cv::Mat rotMat = cv::getRotationMatrix2D(leftEye, rotationDeg, 1.0);
  //std::cout << rotMat << std::endl;
  cv::Mat dst2 = cv::Mat::zeros(src.rows, src.cols, src.type());
  cv::warpAffine(src, dst2, rotMat, dst2.size());
  float cropX = leftEye.x - scale * offsetX;
  float cropY = leftEye.y - scale * offsetY;
  float cropSizeWidth = float(dstSize.width) * scale;
  float cropSizeHeight = float(dstSize.height) * scale;
  /*cout << "ROI: (" << cropX << ", " << cropY << ") -> ("
     << (cropX + cropSizeWidth) << ", " << (cropY + cropSizeHeight) << ")" << endl;*/
  cv::Rect roi(cropX, cropY, cropSizeWidth, cropSizeHeight);
//  Rect roi(10, 10, 100, 100);
  if (roi.x + roi.width >= dst2.cols)
    roi.width = dst2.cols - roi.x - 1; // XXX: -1 ???
  if (roi.y + roi.height >= dst2.rows)
    roi.height = dst2.rows - roi.y - 1;
  if (roi.x < 0)
    roi.x = 0;
  if (roi.y < 0)
    roi.y = 0;
  //std::cout << "ROI: (" << roi.x << ", " << roi.y << ", "
  //   << roi.width << ", " << roi.height << ")" << std::endl;
  //std::cout << "Face size: " << dst2.rows << " x " << dst2.cols << std::endl;
  cv::Rect insideROI = roi & cv::Rect(0, 0, dst2.cols, dst2.rows);
  if(insideROI.area() > 0)
  {
  	cv::Mat cropped = dst2(insideROI);
  	cv::resize(cropped, dst, dstSize);
  }
  else
  {
  	cv::resize(dst2, dst, dstSize);
  }
  //imshow( "src", src );
  //imshow( "rot", dst2 );
  //imshow( "cropped", cropped );
  //imshow( "dst", dst );
};

template<class Key, class Value>
inline void print_map(const std::map<Key, Value>& m, std::ostream& os)
{
    os << "[" << std::endl;
    for (typename std::map<Key, Value>::const_iterator itr = m.begin();
         itr != m.end(); ++itr)
    {
        os << itr->first - 0 << " => " << itr->second - 0 << "," << std::endl;
    }
    os << "]" << std::endl;
};


template<class Key, class Value>
inline void print_map_values(const std::map<Key, Value>& m, std::ostream& os, char sep, char eol)
{
    for (typename std::map<Key, Value>::const_iterator itr = m.begin();
         itr != m.end(); )
    {
        os << itr->second - 0;
        if (++itr == m.end())
            break;
        os << sep;
    }
    os << eol;
};

/**
 * @function computeCircularLBPImage_
 * @brief Creates an image representation of the LBP value for each pixel. Instead of the surrounding pixels, bilinearly interpolated points on a circle around the center pixel are used.
 * @param src Source Image (@link cv::Mat @endlink)
 */

template<class Elem, class Word>
inline cv::Mat computeCircularLBPImage_(cv::Mat& src, int radius, int points)
{
	cv::Mat dst = cv::Mat::zeros(src.size(), CV_8UC1);
	unsigned char min, max;
	min = 255;
	max = 0;
	for (int i = radius; i < src.rows - radius; ++i)
    {
        for (int j = radius; j < src.cols - radius; ++j)
        {
            std::vector<Elem> neighbors(8);
            Word word = 0;
            
            for(int p = 0; p < points; ++p)
            {
                // Unit circle, start at the point below the center (x, y+1)
                double x = static_cast<FLOAT_DMEM>(radius * sin(static_cast<FLOAT_DMEM>(2.0 * CV_PI * p / points)));
                double y = static_cast<FLOAT_DMEM>(radius * cos(static_cast<FLOAT_DMEM>(2.0 * CV_PI * p / points)));
                
                // Discrete pixels, previous & next pixels for x and y -> 4 points
                int x1 = static_cast<int>(floor(x));
                int x2 = static_cast<int>(ceil(x));
                int y1 = static_cast<int>(floor(y));
                int y2 = static_cast<int>(ceil(y));
                
                // R1, R2
                double R1 = static_cast<FLOAT_DMEM>((x2 - x) / (x2 - x1))*src.at<Elem>(x1, y1) + static_cast<FLOAT_DMEM>((x - x1) / (x2 - x1))*src.at<Elem>(x2, y1);
                double R2 = static_cast<FLOAT_DMEM>((x2 - x) / (x2 - x1))*src.at<Elem>(x1, y2) + static_cast<FLOAT_DMEM>((x - x1) / (x2 - x1))*src.at<Elem>(x2, y2);
                
                // Interpolate between R1 and R2 -> desired value
                Elem P = static_cast<Elem>(static_cast<FLOAT_DMEM>((y2 - y) / (y2 - y1))*R1 + static_cast<FLOAT_DMEM>((y - y1) / (y2 - y1))*R2);
                
                if (p > 0)
                    word = word << 1;
                    
                unsigned char bit = P > src.at<Elem>(i, j);
                word |= bit;
                
            }
            dst.at<Word>(i, j) = word;
	    if(word < min)
		min = word;
	    if(word > max)
		max = word;

        }
    }
    printf("Min: %i, Max: %i\n", min, max);

    return dst;
};

/**
 * @function computeCircularLBPImage
 * @brief Wrapper for computeCircularLBPImage_ with standard types <unsigned char, unsigned char>
 * @param src Source Image (@link cv::Mat @endlink)
 * @param radius Radius of the circle, on which the points lie
 * @param points Number of neighbouring points (only 8 supported for now)
 */
inline cv::Mat computeCircularLBPImage(cv::Mat& src, int radius, int points)
{
        // At the moment, only 8 points are supported. To support more points, choose the right data type
	return computeCircularLBPImage_<unsigned char, unsigned char>(src, radius, 8);
};

} // End namespace LBPHistogram

#endif // HAVE_OPENCV

#endif /* OPENCV_LBPHISTOGRAM_HPP_ */
