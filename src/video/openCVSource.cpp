/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

OpenCVFaceSource
Captures frames from either webcam or file stream and extracts a square region containing the face(s).
Returns the LBP, HSV and optical flow histogram of this area.
Author: Florian Gross

*/

#include <video/openCVSource.hpp>
#include <chrono>

#ifdef HAVE_OPENCV

#define MODULE "cOpenCVSource"

using namespace std;
using namespace cv;



SMILECOMPONENT_STATICS(cOpenCVSource)

SMILECOMPONENT_REGCOMP(cOpenCVSource)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_COPENCVSOURCE;
  sdescription = COMPONENT_DESCRIPTION_COPENCVSOURCE;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("display", "Show original and extracted video", 1);
    ct->setField("videoSource", "Choose Video Source: WEBCAM or FILE", "WEBCAM");
    ct->setField("filename", "Filename of video, if VideoSource = FILE", "");
    ct->setField("fps", "Frames per second", 30.0);
    ct->setField("face_cascade_path", "Path to OpenCV face cascade xml file", "/usr/share/opencv/data/lbpcascades/lbpcascade_frontalface.xml");
    ct->setField("eyes_cascade_path", "Path to OpenCV eyes cascade xml file", "/usr/share/opencv/data/haarcascades/haarcascade_eye_tree_eyeglasses.xml");
    ct->setField("extract_face", "Extract face and eyes?", 1);
    ct->setField("extract_hsv_histogram", "Extract HSV histogram?", 1);
    ct->setField("extract_lbp_histogram", "Extract LBP histogram?", 1);
    ct->setField("extract_optical_flow", "Extract optical flow histogram?", 1);
    ct->setField("include_face_features", "Include face features (eye position + distance etc.) in feature vector?", 0);
    ct->setField("ignore_invalid_frames", "Ignore invalid frames (e.g. when no face is detected. Cave: Might lead to synchronisation errors.", 1);
    ct->setField("face_width", "Width/height of extracted face square", 60);
    ct->setField("lbp_uniformpatterns", "Use uniform patterns for LBP histogram", 0);
    ct->setField("hsv_histogram_h_bins", "HSV histogram: Hue bins", 50);
    ct->setField("hsv_histogram_s_bins", "HSV histogram: Saturation bins", 50);
    ct->setField("hsv_histogram_v_bins", "HSV histogram: Value bins", 50);
    ct->setField("of_histogram_bins", "Optical flow histogram: Bins", 10);
    ct->setField("normalize_histograms", "Normalize histograms (L1-norm)", 0);
    ct->setField("of_histogram_max_flow", "Optical flow histogram: Max flow", 0.2);
    ct->setField("of_histogram_downsample", "Optical flow histogram: Downsample", 0.3);
    ct->setField("use_lbpc", "Use LBP-C (circular LBP)", 0);
    ct->setField("lbpc_radius", "LBP-C radius", 1);
    ct->setField("lbpc_points", "LBP-C points (currently only 8 points supported)", 8);
  )

  SMILECOMPONENT_MAKEINFO(cOpenCVSource);
}

SMILECOMPONENT_CREATE(cOpenCVSource)

//-----

cOpenCVSource::cOpenCVSource(const char *_name) :
  cDataSource(_name)
{
	// ...
	mPrevFrame_gray_resized = cv::Mat();
	SMILE_IMSG(1, "cOpenCVSource::cOpenCVSource()");
	SMILE_IMSG(1, "OpenCV Version: %i.%i.%i", CV_MAJOR_VERSION, CV_MINOR_VERSION, CV_SUBMINOR_VERSION);
}

void cOpenCVSource::myFetchConfig()
{
	cDataSource::myFetchConfig();
	SMILE_IMSG(1, "cLBPHistSource::myFetchConfig()");

	cfgDisplay = getInt("display");
	SMILE_IDBG(2, "cfgDisplay = %i", cfgDisplay);

	cfgVideoSource = getStr("videoSource");
	SMILE_IDBG(2, "cfgVideoSource = %s", cfgVideoSource.c_str());

	cfgFilename = getStr("filename");
	SMILE_IDBG(2, "cfgFilename = %s", cfgFilename.c_str());

	cfgFps = getDouble("fps");
	if(cfgFps == 0.0)
	{
		cfgFps = 30.0; // Assume 30 fps
	}
	SMILE_IDBG(2, "cfgFps = %s", cfgFps);


	cfgFace_cascade_name = getStr("face_cascade_path");
	SMILE_IDBG(2, "cfgFace_cascade_name = %s", cfgFace_cascade_name.c_str());

	cfgEyes_cascade_name = getStr("eyes_cascade_path");
	SMILE_IDBG(2, "cfgEyes_cascade_name = %s", cfgEyes_cascade_name.c_str());

	cfgExtractFace = getInt("extract_face");
	SMILE_IDBG(2, "cfgExtractFace = %i", cfgExtractFace);

	cfgExtractHSVHist = getInt("extract_hsv_histogram");
	SMILE_IDBG(2, "cfgExtractHSVHist = %i", cfgExtractHSVHist);

	cfgExtractLBPHist = getInt("extract_lbp_histogram");
	SMILE_IDBG(2, "cfgExtractLBPHist = %i", cfgExtractLBPHist);

	cfgExtractOpticalFlow = getInt("extract_optical_flow");
	SMILE_IDBG(2, "cfgExtractOpticalFlow = %i", cfgExtractOpticalFlow);
	
	cfgIncludeFaceFeatures = getInt("include_face_features");
	SMILE_IDBG(2, "cfgIncludeFaceFeatures = %i", cfgIncludeFaceFeatures);
	
	cfgIgnoreInvalid = getInt("ignore_invalid_frames");
	SMILE_IDBG(2, "cfgIgnoreInvalid = %i", cfgIgnoreInvalid);
	
	cfgFaceWidth = getInt("face_width");
	SMILE_IDBG(2, "cfgFaceWidth = %i", cfgFaceWidth);
	
	cfgLBPUniformPatterns = getInt("lbp_uniformpatterns");
	SMILE_IDBG(2, "cfgLBPUniformPatterns = %i", cfgLBPUniformPatterns);

	cfgHueBins = getInt("hsv_histogram_h_bins");
	SMILE_IDBG(2, "cfgHueBins = %i", cfgHueBins);

	cfgSatBins = getInt("hsv_histogram_s_bins");
	SMILE_IDBG(2, "cfgSatBins = %i", cfgSatBins);

	cfgValBins = getInt("hsv_histogram_v_bins");
	SMILE_IDBG(2, "cfgValBins = %i", cfgValBins);

	cfgFlowBins = getInt("of_histogram_bins");
	SMILE_IDBG(2, "cfgFlowBins = %i", cfgFlowBins);
	
	cfgNormalizeHistograms = getInt("normalize_histograms");
	SMILE_IDBG(2, "cfgNormalizeHistograms = %i", cfgNormalizeHistograms);

	cfgMaxFlow = getDouble("of_histogram_max_flow");
	SMILE_IDBG(2, "cfgMaxFlow = %i", cfgMaxFlow);

	cfgFlowDownsample = getDouble("of_histogram_downsample");
	SMILE_IDBG(2, "cfgFlowDownsample = %i", cfgFlowDownsample);
	
	cfgUseLBPC = getDouble("use_lbpc");
	SMILE_IDBG(2, "cfgUseLBPC = %i", cfgUseLBPC);
	
	cfgLBPCRadius = getDouble("lbpc_radius");
	SMILE_IDBG(2, "cfgLBPCRadius = %i", cfgLBPCRadius);
	
	cfgLBPCPoints = getDouble("lbpc_points");
	SMILE_IDBG(2, "cfgLBPCPoints = %i", cfgLBPCPoints);
}


int cOpenCVSource::myConfigureInstance()
{	
  SMILE_IMSG(1, "cOpenCVSource::myConfigureInstance()");
  int ret = cDataSource::myConfigureInstance();

  return ret;
}

int cOpenCVSource::configureWriter(sDmLevelConfig &c)
{
  // configure your writer by setting values in &c
  c.T = 1 / (double) cfgFps;

  return 1;
}

// NOTE: nEl is always 0 for dataSources....
int cOpenCVSource::setupNewNames(long nEl)
{
	SMILE_IMSG(1, "cOpenCVSource::setupNewNames()");
	mLBPSize = 0;
	
	if(cfgExtractFace && cfgIncludeFaceFeatures)
	{
		writer_->addField("Face_found", 1);
		writer_->addField("Eyes_found", 1);
		writer_->addField("Face_rectangle:_Top_left_X", 1);
		writer_->addField("Face_rectangle:_Top_left_Y", 1);
		writer_->addField("Face_rectangle:_Width", 1);
		writer_->addField("Face_rectangle:_Height", 1);

		writer_->addField("Left_eye:_X", 1);
		writer_->addField("Left_eye:_Y", 1);
		writer_->addField("Right_eye:_X", 1);
		writer_->addField("Right_eye:_Y", 1);
	}
	if(cfgExtractLBPHist)
	{
		if(cfgLBPUniformPatterns)
		{
			writer_->addField("LBP_histogram", 59);
			mLBPSize = 59;
		}
		else
		{
			writer_->addField("LBP_histogram", 256);
			mLBPSize = 256;
		}
	}
	if(cfgExtractHSVHist)
	{
		writer_->addField("HSV_histogram:_Hue", cfgHueBins);
		writer_->addField("HSV_histogram:_Saturation", cfgSatBins);
		writer_->addField("HSV_histogram:_Value", cfgValBins);
	}
	if(cfgExtractOpticalFlow)
	{
		writer_->addField("Optical_flow:_Mean", 2);
		writer_->addField("Optical_flow:_Standard_deviation", 2);
		writer_->addField("Optical_flow_histogram", cfgFlowBins);
	}
	
	
	mVectorSize = cfgExtractFace*cfgIncludeFaceFeatures*10 + cfgExtractHSVHist * (cfgHueBins + cfgSatBins + cfgValBins) + cfgExtractLBPHist * mLBPSize + cfgExtractOpticalFlow * (cfgFlowBins + 4);
	allocVec(mVectorSize);
	return 1;
}

// Used for setting up video capture etc.
int cOpenCVSource::myFinaliseInstance()
{
	int ret = cDataSource::myFinaliseInstance();
	if (ret==0) return 0;
  
	SMILE_IMSG(1, "cOpenCVSource::myFinaliseInstance()");
	if(cfgVideoSource == "WEBCAM")
	{
		mVideoCapture.open(0); // open default camera (e.g. webcam)
	}
	else if(cfgVideoSource == "FILE")
	{
		bool success = mVideoCapture.open(cfgFilename); // open video stream from file

		// Doesn't work for linux, assume 30 fps...
		if(mVideoCapture.get(CV_CAP_PROP_FPS) != 0)
		{
			cfgFps = mVideoCapture.get(CV_CAP_PROP_FPS);
		}
	}
	else
	{
		SMILE_IERR(1, "No video source specified.");
	}

	if(!mVideoCapture.isOpened())
	{
		SMILE_IERR(1, "Could not open video!");
		return 0;
	}
	if(mFace_cascade.empty())
	{
		if( !mFace_cascade.load( cfgFace_cascade_name ) ){ SMILE_IERR(1, "--(!)Error loading face cascade\n"); };
	}
	if(mEyes_cascade.empty())
	{
		if( !mEyes_cascade.load( cfgEyes_cascade_name ) ){ SMILE_IERR(1, "--(!)Error loading eye cascade\n"); };
	}

	mIdentityMap = LBPHistogram::compute_identity_map_<unsigned char>();
	mUniformMap = LBPHistogram::compute_uniform_map_<unsigned char>();

	return ret;
}

eTickResult cOpenCVSource::myTick(long long t)
{
	auto begin = std::chrono::high_resolution_clock::now();
	//SMILE_IMSG(1,"cOpenCVSource::myTick # %i ...",t);
	//std::vector<FLOAT_DMEM> nextFrame;
	//nextFrame.reserve(mVectorSize);
	cVector nextVector(mVectorSize);
	int vectorPosition = 0;

	mVideoCapture >> mCurrentFrame_bgr; // Get new frame from cap stream
	
	auto grabFrame = std::chrono::high_resolution_clock::now();

	typedef std::chrono::duration<double, std::milli> milliseconds;
	double grabFrame_elapsed = std::chrono::duration_cast<milliseconds>(grabFrame - begin).count();
	double convertColor_elapsed = 0;
	double extrF_elapsed = 0;
	double extrLBP_elapsed = 0;
	double extrHSV_elapsed = 0;
	double extrOF_elapsed = 0;

	//-- 3. Apply the classifier to the frame
	if( !mCurrentFrame_bgr.empty() )
	{
		if (cfgDisplay)
		{
			mCurrentFrameDisplay_bgr = mCurrentFrame_bgr.clone();
		}

		cvtColor( mCurrentFrame_bgr, mCurrentFrame_gray, CV_BGR2GRAY );
		equalizeHist( mCurrentFrame_gray, mCurrentFrame_gray );
		
		auto convertColor = std::chrono::high_resolution_clock::now();
		convertColor_elapsed = std::chrono::duration_cast<milliseconds>(convertColor - grabFrame).count();

		if(cfgExtractFace)
		{
			ExtractedFace extractedFace = this->extractFace(mCurrentFrame_bgr, mCurrentFrame_gray);

			if(extractedFace.faceFound == true)
			{
				mCurrentFrame_bgr = extractedFace.faceColor;
				mCurrentFrame_gray = extractedFace.faceGray;
			}
			
			if(extractedFace.faceFound == false && cfgIgnoreInvalid == 1)
			{
				// Discard frame
				SMILE_IMSG(1, "No face detected, frame discarded.");
				return TICK_SUCCESS;
			}
			
			if(cfgIncludeFaceFeatures)
			{
				nextVector.set(vectorPosition++, static_cast<FLOAT_DMEM>(extractedFace.faceFound));
				nextVector.set(vectorPosition++, static_cast<FLOAT_DMEM>(extractedFace.eyesFound));
				nextVector.set(vectorPosition++, static_cast<FLOAT_DMEM>(extractedFace.faceRect.tl().x)); // top left corner
				nextVector.set(vectorPosition++, static_cast<FLOAT_DMEM>(extractedFace.faceRect.tl().y));
				nextVector.set(vectorPosition++, static_cast<FLOAT_DMEM>(extractedFace.faceRect.width)); // width+height
				nextVector.set(vectorPosition++, static_cast<FLOAT_DMEM>(extractedFace.faceRect.height));

				nextVector.set(vectorPosition++, static_cast<FLOAT_DMEM>(extractedFace.leftEye.x)); // left eye, xy
				nextVector.set(vectorPosition++, static_cast<FLOAT_DMEM>(extractedFace.leftEye.y));
				nextVector.set(vectorPosition++, static_cast<FLOAT_DMEM>(extractedFace.rightEye.x)); // right eye, xy
				nextVector.set(vectorPosition++, static_cast<FLOAT_DMEM>(extractedFace.rightEye.y));
			}

			if (cfgDisplay) {
				imshow( "Resized face", mCurrentFrame_gray );
			}
		}
		
		auto extrF = std::chrono::high_resolution_clock::now();
		extrF_elapsed = std::chrono::duration_cast<milliseconds>(extrF - convertColor).count();

		if(cfgExtractLBPHist)
		{
			Mat lbpFace;
			
			if(cfgUseLBPC == 1)
			{
			        lbpFace = LBPHistogram::computeCircularLBPImage(mCurrentFrame_gray, cfgLBPCRadius, cfgLBPCPoints);
			}
			else
			{
			        lbpFace = LBPHistogram::computeLBPImage(mCurrentFrame_gray);
			}
			
			std::map<unsigned char, FLOAT_DMEM> currentLBPHistogram;
			if(cfgLBPUniformPatterns)
			{
				currentLBPHistogram = LBPHistogram::computeLBPHistogram(lbpFace, mUniformMap, cfgNormalizeHistograms);
			}
			else
			{
				currentLBPHistogram = LBPHistogram::computeLBPHistogram(lbpFace, mIdentityMap, cfgNormalizeHistograms);
			}

			// Create std::vector out ouf map
			std::map<unsigned char, FLOAT_DMEM>::iterator i = currentLBPHistogram.begin();
			for( ; i != currentLBPHistogram.end(); ++i )
			{
				nextVector.set(vectorPosition++, i->second);
			}
			if (cfgDisplay)
			{
				imshow( "LBP face", lbpFace );
			}
		}
		
		auto extrLBP = std::chrono::high_resolution_clock::now();
		extrLBP_elapsed = std::chrono::duration_cast<milliseconds>(extrLBP - extrF).count();

		if(cfgExtractHSVHist)
		{
			std::vector<cv::Mat> hsvHists = HSVHistogram::computeHSVHistogram(mCurrentFrame_bgr, cfgHueBins, cfgSatBins, cfgValBins, cfgNormalizeHistograms);
			
			//SMILE_IMSG(1, "Ges: [0] -> %lf, [12] -> %lf", hsvHists[0].at<FLOAT_DMEM>(0), hsvHists[0].at<FLOAT_DMEM>(12));

			for(int i = 0; i < cfgHueBins; i++)
			{
				nextVector.set(vectorPosition++, hsvHists[0].at<FLOAT_DMEM>(i));
			}
			for(int i = 0; i < cfgSatBins; i++)
			{
				nextVector.set(vectorPosition++, hsvHists[1].at<FLOAT_DMEM>(i));
			}
			for(int i = 0; i < cfgValBins; i++)
			{
				nextVector.set(vectorPosition++, hsvHists[2].at<FLOAT_DMEM>(i));
			}
		}
		

		auto extrHSV = std::chrono::high_resolution_clock::now();
		extrHSV_elapsed = std::chrono::duration_cast<milliseconds>(extrHSV - extrLBP).count();

		if(cfgExtractOpticalFlow)
		{
			OpticalFlow::FrameFtExtrParams featureSettings;
			featureSettings.flo_bins = cfgFlowBins;
			featureSettings.max_flow = cfgMaxFlow;
			featureSettings.flow_downsample = cfgFlowDownsample;

			cv::Mat currentFrame_gray_resized;
			cv::resize(mCurrentFrame_gray, currentFrame_gray_resized, cv::Size(), featureSettings.flow_downsample, featureSettings.flow_downsample, 1);

			// If size changes, face not detected for example
			//printf("Current: %i x %i px, prev: %i x %i px \n", currentFrame_gray_resized.size().width, currentFrame_gray_resized.size().height, mPrevFrame_gray_resized.size().width, mPrevFrame_gray_resized.size().height);
			if(mPrevFrame_gray_resized.size() != currentFrame_gray_resized.size() && mPrevFrame_gray_resized.size().width != 0)
			{
				// Resize image
				//cv::resize(mPrevFrame_gray_resized, mPrevFrame_gray_resized, currentFrame_gray_resized.size());
				// Discard image (optical flow would be erroneous)
				SMILE_IMSG(2, "Size changed, previous frame for optical flow discarded.");
				mPrevFrame_gray_resized = cv::Mat();
			}


			std::vector<FLOAT_DMEM> opticalFlowHist = OpticalFlow::computeOpticalFlowHistogram(currentFrame_gray_resized, mPrevFrame_gray_resized, featureSettings, cfgDisplay, cfgNormalizeHistograms);
			mPrevFrame_gray_resized = currentFrame_gray_resized.clone(); //save frame for next optical flow step
			// ... copy to nextFrame ...
			for(int i = 0; i < opticalFlowHist.size(); i++)
			{
				nextVector.set(vectorPosition++, opticalFlowHist.at(i));
			}
			opticalFlowHist.clear();
		}
		
		
		auto extrOF = std::chrono::high_resolution_clock::now();
		extrOF_elapsed = std::chrono::duration_cast<milliseconds>(extrOF - extrHSV).count();

		if(cfgDisplay)
		{
			imshow( "video", mCurrentFrameDisplay_bgr );
		}
	}
	else
	{
		SMILE_IMSG(1, " --(!) No captured frame -- Stop!");
		return TICK_INACTIVE;
	}
	

	if(cfgDisplay)
	{
		// At least one ms required for event processing (freezes otherwise)
		waitKey(1);
	}

	/*vec_->data = nextFrame.data(); // This part caused an error when program finishes... (double free)
	vec_->tmeta->smileTime = (t / cfgFps); // Set timestamp
	vec_->tmeta->time = (t / cfgFps); // Timestamp
	writer_->setNextFrame(vec_);*/
	
	
	nextVector.tmeta->time = (double) (t / cfgFps); // Set Timestamp
	nextVector.tmeta->period = (double) 1 / cfgFps;
	if (!writer_->setNextFrame(&nextVector))
	  SMILE_IERR(2, "Could not write output to destination level, increase its buffer size.");
	
	bool measureTime = false; // Set to true to measure execution time (+ RTF) of several parts of this code
	if(measureTime)
	{
		auto end = std::chrono::high_resolution_clock::now();
		double total_elapsed = std::chrono::duration_cast<milliseconds>(end - begin).count();
	
		SMILE_IMSG(1, "      Function     |  Elapsed time |  RTF   ");
		SMILE_IMSG(1, "-------------------+-------------+--------");
		SMILE_IMSG(1, " grab frame        |  %3.4f ms  | %.5f   ", grabFrame_elapsed, grabFrame_elapsed/1000*cfgFps);
		SMILE_IMSG(1, "-------------------+-------------+--------");
		SMILE_IMSG(1, " convert to gray   |  %3.4f ms  | %.5f   ", convertColor_elapsed, convertColor_elapsed/1000*cfgFps);
		SMILE_IMSG(1, "-------------------+-------------+--------");
		SMILE_IMSG(1, " extract Face      |  %3.4f ms | %.5f   ", extrF_elapsed, extrF_elapsed/1000*cfgFps);
		SMILE_IMSG(1, "-------------------+-------------+--------");
		SMILE_IMSG(1, " LBP               |  %3.4f ms  | %.5f   ", extrLBP_elapsed, extrLBP_elapsed/1000*cfgFps);
		SMILE_IMSG(1, "-------------------+-------------+--------");
		SMILE_IMSG(1, " HSV               |  %3.4f ms  | %.5f   ", extrHSV_elapsed, extrHSV_elapsed/1000*cfgFps);
		SMILE_IMSG(1, "-------------------+-------------+--------");
		SMILE_IMSG(1, " Optical Flow      |  %3.4f ms  | %.5f   ", extrOF_elapsed, extrOF_elapsed/1000*cfgFps);
		SMILE_IMSG(1, "-------------------+-------------+--------");
		SMILE_IMSG(1, " Total             |  %3.4f ms  | %.5f   ", total_elapsed, total_elapsed/1000*cfgFps);
	}

	return TICK_SUCCESS;
}

ExtractedFace cOpenCVSource::extractFace(cv::Mat& frame, cv::Mat& frame_gray)
{
	ExtractedFace extractedFace;
	extractedFace.faceFound = false;
	extractedFace.eyesFound = false;
	extractedFace.faceRect = cv::Rect();
	extractedFace.leftEye = cv::Point();
	extractedFace.rightEye = cv::Point();
	extractedFace.faceColor = cv::Mat();
	extractedFace.faceGray = cv::Mat();
	
	cv::Rect matBoundingRect = cv::Rect(0, 0, frame_gray.cols, frame_gray.rows);

	std::vector<cv::Rect> faces;

	mFace_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0, Size(80, 80) );

	   if (faces.size() >= 1)
	      {
	      	 cv::Rect insideFrame = faces[0] & matBoundingRect;
	      	 // (0 <= roi.x && 0 <= roi.width && roi.x + roi.width <= m.cols && 0 <= roi.y && 0 <= roi.height && roi.y + roi.height <= m.rows
	      	 //std::cout << "roi.x = " << insideFrame.x << ", roi.y = " << insideFrame.y << ", roi.width = " << insideFrame.width
	      	 //			<< ", roi.height = " << insideFrame.height << ", m.cols = " << frame_gray.cols
  	 		 //			<< ", m.rows = " << frame_gray.rows << std::endl;
	      	 
		  	 if(insideFrame.area() > 0)
		  	 {
		  	 	 extractedFace.faceFound = true;
				 extractedFace.faceRect = faces[0];
			     Mat faceROI = frame_gray(insideFrame).clone();
			     Mat faceResized_gray = Mat::zeros(faceROI.rows, faceROI.cols, faceROI.type());
			     std::vector<Rect> eyes;
			     mEyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |CV_HAAR_SCALE_IMAGE, Size(30, 30) );
			     if( eyes.size() == 2)
			     {
			    	extractedFace.eyesFound = true;
			        std::vector<Point> eyeCenters(eyes.size());
			        Point leftEye, rightEye;
			        for( size_t j = 0; j < eyes.size(); j++ )
			        {
			           //eyeCenters[j] = Point(eyes[j].x + eyes[j].width / 2, eyes[j].y + eyes[j].height / 2);
			           eyeCenters[j] = Point(faces[0].x + eyes[j].x + eyes[j].width / 2,
			               faces[0].y + eyes[j].y + eyes[j].height / 2);
			        }
			        if (eyeCenters[0].x > eyeCenters[1].x)
			        {
			           rightEye = eyeCenters[0];
			           leftEye = eyeCenters[1];
			        }
			        else
			        {
			           rightEye = eyeCenters[1];
			           leftEye = eyeCenters[0];
			        }

			        extractedFace.leftEye = leftEye;
			        extractedFace.rightEye = rightEye;

			        Size faceSize = cv::Size(cfgFaceWidth, cfgFaceWidth);
			        LBPHistogram::cropFace(frame_gray, faceResized_gray, leftEye, rightEye, 0.3, 0.3, faceSize);
			        extractedFace.faceGray = faceResized_gray;

			        LBPHistogram::cropFace(frame, extractedFace.faceColor, leftEye, rightEye, 0.3, 0.3, faceSize);

			        if (cfgDisplay)
			        {
			            // imshow( "lbp_uni", lbpFaceUni );


			            for( size_t j = 0; j < eyes.size(); j++ )
			             { //-- Draw the eyes
			               Point eye_center( faces[0].x + eyes[j].x + eyes[j].width/2, faces[0].y + eyes[j].y + eyes[0].height/2 );
			               int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );
			               circle( mCurrentFrameDisplay_bgr, eye_center, radius, Scalar( 255, 0, 255 ), 3, 8, 0 );
			             }
			        }
			      }
			      else
			      {
			        SMILE_IMSG(1, "No eyes detected!");

			        resize(faceROI, faceResized_gray, Size(cfgFaceWidth, cfgFaceWidth));
			        resize(frame(insideFrame), extractedFace.faceColor, Size(cfgFaceWidth, cfgFaceWidth));
			        extractedFace.eyesFound = false;
			        extractedFace.faceGray = faceResized_gray;

			      }

			     if (cfgDisplay)
			     {
			        //-- Draw the face
			        Point center( faces[0].x + faces[0].width/2, faces[0].y + faces[0].height/2 );
			        ellipse( mCurrentFrameDisplay_bgr, center, Size( faces[0].width/2, faces[0].height/2), 0, 0, 360, Scalar( 255, 0, 0 ), 2, 8, 0 );
			     }
			 } // if(insideFrame.area() > 0)
			 else
			 {
			 	SMILE_IMSG(1, "Face not in image region -> discarded.");
			 	extractedFace.faceFound = false;
			 }
	      } // if (faces.size() >= 1)
	      else
	      {
	        //SMILE_IMSG(1, "No face detected!");
	        extractedFace.faceFound = false;
	      }

	   return extractedFace;
}


cOpenCVSource::~cOpenCVSource()
{

}

#endif // HAVE_OPENCV
