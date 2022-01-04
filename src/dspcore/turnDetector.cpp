/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

simple silence threshold based turn detector

*/


#include <dspcore/turnDetector.hpp>

#define MODULE "cTurnDetector"

// default values (can be changed via config file...)
#define N_PRE  10
#define N_POST 20

SMILECOMPONENT_STATICS(cTurnDetector)

SMILECOMPONENT_REGCOMP(cTurnDetector)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CTURNDETECTOR;
  sdescription = COMPONENT_DESCRIPTION_CTURNDETECTOR;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("threshold","The silence->speech threshold to use (the default value is for RMS energy, change it to -13.0 for log energy)",0.001);
    ct->setField("threshold2","The speech->silence threshold to use (if this value is not set in the config, the same value as 'threshold' will be used)",0.001);
    
    ct->setField("autoThreshold","1 = automatically adjust threshold for RMS energy (EXPERIMENTAL; works for energy as input)",0);
    ct->setField("minmaxDecay","The decay constant used for min/max values in auto-thresholder (a larger value means a slower recovery from loud sounds)",0.9995);
  //TODO: these should be given in seconds and they should be converted to frames via period or via secToVidx..?
    ct->setField("nPre","number of frames > threshold until a turn start is detected",N_PRE);
    ct->setField("nPost","number of frames < threshold(2) until a turn end is detected",N_POST);
  //---  
    ct->setField("useRMS","1 = the provided energy field in the input is rms energy instead of log energy",1);
    ct->setField("readVad","1 = use the result (bianry 0/1 or probability) from another VAD component instead of reading RMS or LOG energy ('threshold' and 'threshold2' will be set to 0.55 and 0.45 if this option is enabled, unless other values for thresholds are given in the config file)",0);
    ct->setField("idx","The index of the RMS or LOG energy (or vadBin) field to use (-1 to automatically find the field)",-1);

    ct->setField("messageRecp","The (cWinToVecProcessor type) component(s) to send 'frameTime' messages to (use , to separate multiple recepients), leave blank (NULL) to not send any messages. The messages will be sent at the turn end and (optionally) during the turn at fixed intervals configured by the 'msgInterval' parameter (if it is not 0).",(const char *) NULL);
    ct->setField("msgInterval","Interval at which to send 'frameTime' messages during an ongoing turn. Set to 0 to disable sending of intra turn messages.",0.0);
    ct->setField("turnFrameTimePreRollSec", "Time offset which is added to the turnStart for turnFrameTimeMessages. Use this to compensate for VAD lags. Typically one would use negative values here, e.g. -0.1.", 0.0);
    ct->setField("turnFrameTimePostRollSec", "Time offset which is added to the turnEnd for turnFrameTimeMessages. Use this to compensate for VAD lags. CAUTION: If this value is positive, it might prevent the receiving component from working correctly, as it will not have all data (for the full segment) available in the input data memory level when it receives the message.", 0.0);
    ct->setField("msgPeriodicMaxLength", "If periodic message sending is enabled (msgInterval > 0), then this can limit the maximum length of the segments (going backwards from the current position, i.e. a sliding window - as opposed to maxTurnLength, which limits the total turn length from the beginning of the turn). If this is 0, there is no limit (= default), the segments will grow up to maxTurnLength.", 0.0);
    ct->setField("sendTurnFrameTimeMessageAtEnd", "If not 0, indicates that at the end of a turn a turnFrameTime message will be sent. If it is set to 1, a full length (from turn start to turn end) message will be sent. If it is set to 2, and if periodic sending is enabled (msgInterval > 0) and msgPeriodicMaxLength is set (> 0), then only a message of msgPeriodicMaxLength (from turn end backwards) will be sent. Leave this option at the default of 1 if not using periodic message sending (msgInterval > 0).", 1);
    ct->setField("eventRecp","The component(s) to send 'turnStart/turnEnd' messages to (use , to separate multiple recepients), leave blank (NULL) to not send any messages",(const char *) NULL);
    ct->setField("statusRecp","The component(s) to send 'turnSpeakingStatus' messages to (use , to separate multiple recepients), leave blank (NULL) to not send any messages",(const char *) NULL);
    ct->setField("minTurnLengthTurnFrameTimeMessage", "The minimum turn length in seconds (<= 0 : infinite) for turnFrameTime messages. No Message will be sent if the detected turn is shorter than the given value. turnStart and turnEnd messages will still be sent though.", 0);
    ct->setField("minTurnLength", "[NOT YET IMPLEMENTED!] The minimum turn length in seconds (<= 0 : infinite) for turnFrameTime and turnStart messages. No Message will be sent if the detected turn is shorter than the given value. IMPORTANT: This introduces a lag of the given minimum length for turn start messages!", 0);
    ct->setField("maxTurnLength", "The maximum turn length in seconds (<= 0 : infinite). A turn end will be favoured by reducing nPost to 1 after this time",0.0);
    ct->setField("maxTurnLengthGrace", "The grace period to grant, after maxTurnLength is reached (in seconds). After a turn length of maxTurnLength + maxTurnLengthGrace an immediate turn end will be forced.",1.0);
    ct->setField("invert", "Invert the behaviour of turnStart/turnEnd messages. Also send a turnStart message at vIdx = 0, and a turnEnd message at the end (EOI).", 0);
    ct->setField("debug","log level to show some turn detector specific debug messages on",4);
    ct->setField("timeoutSec","turnEnd timeout in seconds (send turnEnd after timeoutSec seconds no input data, <= 0 : infinite)",2.0);
    ct->setField("eoiFramesMissing","set the number of frames that will be subtracted from the last turn end position (the forced turn end that will be sent when an EOI condition (end of input) is encountered). This is necessary, e.g. if you use delta or acceleration coefficients which introduce a lag of a few frames. Increase this value if SMILExtract hangs at the end of input when using a cFunctionals component, etc.",5);
    ct->setField("unblockTimeout","timeout in frames to wait after a turn block condition (started via a semaineCallback message)",60);
    ct->setField("blockStatus","apply event based speech detection block  for speakingStatus messages (i.e.  the sending of these messages is supressed)",0);
    ct->setField("blockAll","apply event based speech detection block for all types, i.e. the voice input is set to 0 by an incoming block message.",1);

    ct->setField("terminateAfterTurns", "Number of turns after which to terminate processing and exit openSMILE. Default 0 is for infinite, i.e. never terminate.", 0);
    ct->setField("terminatePostSil", "Amount of silence after last turn of terminateAfterTurns to wait for before actually exiting. This excludes (i.e. is on top of) postSil which is required to detect the end of the turn.", 0);
    ct->setField("initialBlockTime", "Initial time (in seconds) to block VAD (useful in conjunction with RNN vad, or if high noise occurrs after starting VAD.", 1.0);

    ct->setField("loadSegmentsFromFile", "If set to a filename, load the segment times from this CSV file (; as separator/ header line/ columns 'Start' and 'End' required, others are ignored. The input level data is then ignored, only the frame timestamps are used to sync and send messages based on the file timestamps. Not really suitable for live mode (although it works, but no sense in using pre-defined timestamps...)!", (const char *)NULL);
  )
  SMILECOMPONENT_MAKEINFO(cTurnDetector);
}

SMILECOMPONENT_CREATE(cTurnDetector)

/*
 * Turndetector advanced:
 * - reads in VAD
 * - reads in Overlap
 * - reads in Gender Male/Female
 *
 * splits at vad start/stop
 * splits at overlap (over a threshold for n frames)
 * splits at gender change
 * small tolerance for the event that multiple split indicators (gender change + pause or overlap)
 *    occurr almost at the same time -> split only once.
 *
 */
//-----

cTurnDetector::cTurnDetector(const char *_name) :
  cDataProcessor(_name),
  turnState(0), actState(0),
  useRMS(1),
  threshold((FLOAT_DMEM)0.0),
  nPost(N_POST),
  nPre(N_PRE),
  cnt1(0), cnt2(0), cnt1s(0), cnt2s(0), debug(4),
  startP(0),
  recFramer(NULL),
  recComp(NULL),
  statusRecp(NULL),
  rmsIdx(-1),
  autoRmsIdx(-1),
  autoThreshold(0),
  minmaxDecay((FLOAT_DMEM)0.9995),
  nmin(0), nmax(0),
  rmin(1.0), rmax(0.0), rmaxlast(0.0),
  tmpmin(0.0), tmpmax(0.0),
  dtMeanO(0.0), dtMeanAll(0.0), dtMeanT(0.0), dtMeanS(0.0),
  nE(0.0),
  nTurn(0), nSil(0),
  alphaM((FLOAT_DMEM)0.9999),
  calCnt(0),
  msgInterval(0.0),
  lastDataTime(0),
  timeout(0),
  timeoutSec(10.0),
  forceEnd(0),
  blockTurn(0), unblockTurnCntdn(0), blockAll(1), blockStatus(0),
  initialBlockTime(0), initialBlockFrames(0),
  terminateAfterTurns(0), terminatePostSil(0), nTurns(0),
  exitFlag(0), nSilForExit(0),
  lastVIdx(0),
  lastVTime(0.0),
  turnTime(0.0)
{
}

void cTurnDetector::myFetchConfig()
{
  cDataProcessor::myFetchConfig();
  
  useRMS = getInt("useRMS");
  readVad = getInt("readVad");
  SMILE_IDBG(2,"readVad = %i",readVad);
  if (readVad) {
    useRMS = 0;
  }

  nPre = getInt("nPre");
  nPost = getInt("nPost");

  threshold = (FLOAT_DMEM)getDouble("threshold");
  if ((useRMS)&&(threshold<(FLOAT_DMEM)0.0)) {
    threshold = (FLOAT_DMEM)0.001;
  }

  if (isSet("threshold2")) {
    threshold2 = (FLOAT_DMEM)getDouble("threshold2");
    if ((useRMS)&&(threshold2<(FLOAT_DMEM)0.0)) {
      threshold2 = threshold;
    }
  } else {
    threshold2 = threshold;
  }
  autoThreshold = getInt("autoThreshold");

  rmsIdx = getInt("idx");
  debug = getInt("debug");
  eoiMis = getInt("eoiFramesMissing");

  if (readVad) {
    autoThreshold = 0;
    useRMS = 0;
    if (isSet("threshold2")) {
      if (threshold2 < 0.2 || threshold2 > 0.8) {
        SMILE_IWRN(1,"readVad=1, and threshold2 set to a value which looks like log- or rms-energy thresholds! You should remove the threshold option, or consider that you are setting the VAD threshold with it!");
      }
    } else {
      threshold2 = (FLOAT_DMEM)0.45;
    }
    if (isSet("threshold")) {
      if (threshold < 0.2 || threshold > 0.8) {
        SMILE_IWRN(1,"readVad=1, and threshold set to a value which looks like log- or rms-energy thresholds! You should remove the threshold option, or consider that you are setting the VAD threshold with it!");
      }
    } else {
      threshold = (FLOAT_DMEM)0.55;
    }
  }
  SMILE_IDBG(2,"nPre = %i",nPre);
  SMILE_IDBG(2,"nPost = %i",nPost);

  SMILE_IDBG(2,"useRMS = %i",useRMS);
  SMILE_IDBG(2,"idx = %i",rmsIdx);
  SMILE_IDBG(2,"autoThreshold = %i",autoThreshold);
  SMILE_IDBG(2,"threshold = %f",threshold);

  recFramer = getStr("messageRecp");
  SMILE_IDBG(2,"messageRecp = '%s'",recFramer);

  msgInterval = getDouble("msgInterval");
  SMILE_IDBG(2,"msgInterval = %f",msgInterval);

  recComp = getStr("eventRecp");
  SMILE_IDBG(2,"eventRecp = '%s'",recComp);

  statusRecp = getStr("statusRecp");
  SMILE_IDBG(2,"statusRecp = '%s'",statusRecp);

  timeoutSec = getDouble("timeoutSec");

  maxTurnLengthS = getDouble("maxTurnLength");
  graceS = getDouble("maxTurnLengthGrace");

  unblockTimeout = getInt("unblockTimeout");
  blockAll = getInt("blockAll");
  blockStatus = getInt("blockStatus");
  initialBlockTime = getDouble("initialBlockTime");

  terminateAfterTurns = getInt("terminateAfterTurns");
  terminatePostSil = getInt("terminatePostSil");
  
  invert_ = getInt("invert");
  minTurnLength_ = getDouble("minTurnLength");
  if (isSet("minTurnLengthTurnFrameTimeMessage")) {
    minTurnLengthTurnFrameTimeMessage_ = getDouble("minTurnLengthTurnFrameTimeMessage");
  } else {
    minTurnLengthTurnFrameTimeMessage_ = minTurnLength_;
  }
  //~ if (msgInterval > 0.0 && minTurnLength_ > 0.0) {
    //~ if (msgInterval < minTurnLength_) {
      //~ SMILE_IWRN(1, "the turnFrameTime message interval (msgInterval) (%f) is smaller than the minimum turn length (%f), this effectively causes to minTurnLength = msgInterval !! Consider fixing your config file.", msgInterval, minTurnLength_);
    //~ }
  //~ }
  
  turnFrameTimePreRollSec_ = getDouble("turnFrameTimePreRollSec");
  turnFrameTimePostRollSec_ = getDouble("turnFrameTimePostRollSec");
  msgPeriodicMaxLengthSec_ = getDouble("msgPeriodicMaxLength");
  sendTurnFrameTimeMessageAtEnd_ = getInt("sendTurnFrameTimeMessageAtEnd");

  segmentFile_ = getStr("loadSegmentsFromFile");
}

int cTurnDetector::setupNewNames(long nEl)
{
  writer_->addField( "isTurn" );
  namesAreSet_ = 1;

  // convert maxTurnLength from seconds to frames...
  double T = (double)(reader_->getLevelT());
  SMILE_IMSG(4, "Reader T: %f", T);
  if (T == 0.0) 
    T = 1.0;
  if (maxTurnLengthS < 0.0) 
    maxTurnLengthS = 0.0;
  maxTurnLength = (long)ceil(maxTurnLengthS / T);
  if (graceS < 0.0) 
    graceS = 0.0;
  grace = (long)ceil(graceS / T);
  if (minTurnLength_ < 0.0) 
    minTurnLength_ = 0.0;
  minTurnLengthFrames_ = (long)ceil(minTurnLength_ / T);
  if (minTurnLengthTurnFrameTimeMessage_ < 0.0) 
    minTurnLengthTurnFrameTimeMessage_ = 0.0;
  minTurnLengthFrameTimeFrames_ = (long)ceil(minTurnLengthTurnFrameTimeMessage_ / T);

  initialBlockFrames = (long)ceil(initialBlockTime / T);

  if (msgPeriodicMaxLengthSec_ < 0.0) 
    msgPeriodicMaxLengthSec_ = 0.0;
  msgPeriodicMaxLengthFrames_ = (long)ceil(msgPeriodicMaxLengthSec_ / T);
  
  if (msgPeriodicMaxLengthSec_ < 0.0) 
    msgPeriodicMaxLengthSec_ = 0.0;
  msgPeriodicMaxLengthFrames_ = (long)ceil(msgPeriodicMaxLengthSec_ / T);
  
  turnFrameTimePreRollFrames_ = (long)ceil(turnFrameTimePreRollSec_ / T);
  turnFrameTimePostRollFrames_ = (long)ceil(turnFrameTimePostRollSec_ / T);
  
  if (turnFrameTimePreRollSec_ > 0.0) {
    SMILE_IWRN(2, "turnFrameTimePreRollSec is a positive value! This means that the turn start will be cut off, as this value is added to the turn start. Did you mean to use a negative value?");
  }
  SMILE_IMSG(3, "postRollFrames: %i", turnFrameTimePostRollFrames_);
  SMILE_IMSG(3, "preRollFrames: %i", turnFrameTimePreRollFrames_);
  if (segmentFile_ != NULL) {
    if (!loadSegmentsFromFile(T))
      return 0;
  }
  return 1;
}

bool cTurnDetector::loadSegmentsFromFile(double T) {
  cSmileUtilCsv csv(segmentFile_);
  if (!csv.openFileAndReadHeader(false, true)) {
    SMILE_IERR(1, "failed to load speech segment file '%s'.", segmentFile_);
    return false;
  }
  std::map<std::string, long> &colmap = csv.getColumnNameMap();
  const FLOAT_DMEM * x = NULL;
  int N = csv.getNcolumns();
  double framesPerSec = (1.0 / T);
  preLoadedSegments_.clear();
  do {
    x = csv.readRow(0.0);
    if (x != NULL) {
      sTurnSegment seg;
      if (colmap.count("Start")) {
        seg.startTime = x[colmap.at("Start")];
        seg.startVidx = (long long)(x[colmap.at("Start")] * framesPerSec);
      }
      if (colmap.count("End")) {
        seg.startTime = x[colmap.at("Start")];
        seg.startVidx = (long long)(x[colmap.at("Start")] * framesPerSec);
      }
      preLoadedSegments_.push_back(seg);
    }
  } while(x != NULL);
  preLoadedSegmentsIndex_ = 0;
  return true;
}

void cTurnDetector::updateThreshold(FLOAT_DMEM eRmsCurrent)
{
  // compute various statistics on-line:

  // min "percentile" (robust min.)
  // "value below which Nmin values are.."
  if (eRmsCurrent < rmin) {
    nmin++;
    tmpmin += eRmsCurrent;
    if (nmin > 10) {
      rmin = tmpmin / (FLOAT_DMEM)nmin;
      //SMILE_IDBG(2,"adjusted rmin: %f",rmin);
      nmin = 0;
      tmpmin = 0;
    }
  }

  // auto decay..
  rmin *= (FLOAT_DMEM)1.0 + ((FLOAT_DMEM)1.0 - minmaxDecay);

  // max "percentile" (robust max.)
  if (eRmsCurrent > rmax) {
    nmax++;
    tmpmax += eRmsCurrent;
    if (nmax > 10) {
      rmaxlast = rmax;
      rmax = tmpmax / (FLOAT_DMEM)nmax;
      //SMILE_IDBG(2,"adjusted rmax: %f",rmax);
      nmax = 0;
      tmpmax = 0.0;
    }
  }

  // auto decay..
  rmax *= minmaxDecay;

  //}

  // mean overall (exponential decay)
  dtMeanO = minmaxDecay * (dtMeanO - eRmsCurrent) + eRmsCurrent;

  // mean overall, non decaying
  dtMeanAll = (dtMeanAll * nE + eRmsCurrent) / (nE+(FLOAT_DMEM)1.0);
  nE+=1.0;

  // mean of turns

  if (turnState) {
    nTurn++;
    dtMeanT = alphaM * (dtMeanT - eRmsCurrent) + eRmsCurrent;
  } else {
    nSil++;
  // mean of non-turns
    dtMeanS = alphaM * (dtMeanS - eRmsCurrent) + eRmsCurrent;
  }


  // update threshold based on collected statistics:
  FLOAT_DMEM newThresh;
/*  if (dtMeanT == 0.0) {
    //newThresh = ( rmax + (rmax - dtMeanS) ) * 1.1;
  } else {
    //newThresh = 0.5*(dtMeanT + ( rmax + (rmax - dtMeanS) )*1.1) ;
    newThresh = MAX( 0.5*(dtMeanT + rmaxlast ), rmaxlast * 1.41) ;
  }*/

  if (nTurn == 0) { newThresh = dtMeanAll * (FLOAT_DMEM)2.0; }
  else {
    newThresh = (FLOAT_DMEM)0.15 * ( rmax + rmin ) ;
    FLOAT_DMEM w = (FLOAT_DMEM)nTurn / ( (FLOAT_DMEM)nSil + (FLOAT_DMEM)nTurn);
    FLOAT_DMEM w2 = sqrt((FLOAT_DMEM)1.0-w)+(FLOAT_DMEM)1.0;
    if (dtMeanO < newThresh) { newThresh = w*newThresh + ((FLOAT_DMEM)1.0-w)*MAX(dtMeanO*w2, (FLOAT_DMEM)1.2*w2*dtMeanAll) ; }
  }
  

  threshold = (FLOAT_DMEM)0.8*threshold + (FLOAT_DMEM)0.2*newThresh;

#ifdef DEBUG
  tmpCnt++;
  if (tmpCnt>200) {
    SMILE_IDBG(2,"THRESH: %f  rmax %f  rmin %f  dtMeanO %f  dtMeanAll %f",threshold,rmax,rmin,dtMeanO,dtMeanAll);
    //SMILE_IDBG(2,"dtMeanO: %f",dtMeanO);
    //SMILE_IDBG(2,"dtMeanT: %f",dtMeanT);
    //SMILE_IDBG(2,"dtMeanS: %f",dtMeanS);
    tmpCnt = 0;
  }
#endif
}

int cTurnDetector::isVoice(FLOAT_DMEM *src, int state)
{
  /* state 0: silence/noise, state 1: voice */
  if (invert_ == 1) {
    if (state == 0) {
      if (src[rmsIdx] <= threshold) return 1;
    } else {
      if (src[rmsIdx] <= threshold2) return 1;
    }
  } else {
    if (state == 0) {
      if (src[rmsIdx] > threshold) return 1;
    } else {
      if (src[rmsIdx] > threshold2) return 1;
    }
    //if (src[0] > 0.4) return 1;
  }
  return 0;
}

void cTurnDetector::sendMessageTurnStart(const char * recp,
    long long npre, long long vIdx, double smileTime) {
  if (recp != NULL) {
    cComponentMessage cmsg("turnStart");
    cmsg.floatData[0] = (double)npre;    // nPre (idx)
    cmsg.floatData[1] = (double)vIdx;  // start of turn (idx)
    cmsg.floatData[2] = (double)(reader_->getLevelT());  // period of level
    cmsg.userTime1 = smileTime;
    sendComponentMessage(recp, &cmsg);
    SMILE_IMSG(debug, "sending turnStart message to '%s'", recp);
  }
}

void cTurnDetector::sendMessageTurnStart(long long npre,
    long long vIdx, double smileTime) {
  sendMessageTurnStart(recComp, npre, vIdx, smileTime);
}

void cTurnDetector::sendMessageTurnStart() {
  sendMessageTurnStart(recComp, nPre, startP, startSmileTime);
}

void cTurnDetector::sendMessageTurnEnd(const char * recp,
    long long npost, long long vIdx, double smileTime,
    int complete, int turnEndFlag) {
  if (recp != NULL) {
    cComponentMessage cmsg("turnEnd");
    cmsg.floatData[0] = (double)npost;
    cmsg.floatData[1] = (double)(vIdx);
    cmsg.floatData[2] = (double)(reader_->getLevelT());
    cmsg.intData[0] = complete;     // 0 = incomplete turn...
    cmsg.intData[1] = turnEndFlag;  // set forced turn end flag to EOI (2)
    cmsg.userTime1 = smileTime;
    sendComponentMessage(recp, &cmsg);
    SMILE_IMSG(debug, "sending turnEnd message to '%s' (due to TIMEOUT)", recp);
  }
}

void cTurnDetector::sendMessageTurnEnd(
    long long nPost, long long vIdx, double smileTime,
    int complete, int turnEndFlag) {
  sendMessageTurnEnd(recComp, nPost, vIdx, smileTime, complete, turnEndFlag);
}

void cTurnDetector::sendMessageTurnEnd() {

}

void cTurnDetector::sendMessageTurnFrameTime(const char * recp,
    long long startVidx, long long endVidx, int statusFlag,
    int endingForced, double startSmileTime, double endSmileTime) {
  if (recp == NULL)
    return;
  cComponentMessage cmsg("turnFrameTime");
  cmsg.intData[0] = statusFlag; /* 1 = indicates a turn end, 0 intra turn */
  // send start/end in frames of input level
  if (startVidx < 0)
    startVidx = 0;
  if (endVidx < 2)
    endVidx = 2;
  if (endVidx <= startVidx) {
    SMILE_IERR(1, "turn has negative or zero length! Somthing is wrong! (%lld - %lld). Not sending message.",
        startVidx, endVidx);
    return;
  }
  cmsg.floatData[0] = (double)startVidx;
  cmsg.floatData[1] = (double)endVidx;
  double T = (double)(reader_->getLevelT());
  if (T != 0.0) {
    // also send start/end as actual data time in seconds
    cmsg.floatData[2] = ((double)startVidx) * T;
    cmsg.floatData[3] = ((double)endVidx) * T;
    // and send period of input level
    cmsg.floatData[4] = T;
  }
  cmsg.intData[1] = endingForced;
  // intData[1] : 0=normal end / 1=forced turn end (max length)
  cmsg.userTime1 = startSmileTime;
  cmsg.userTime2 = endSmileTime;
  sendComponentMessage(recp, &cmsg);
  const char * status = "turn end";
  if (statusFlag == 0)
    status = "intra turn";
  SMILE_IMSG(debug, "sending turnFrameTime message (%s) to '%s' (%lld - %lld).",
      status, recp, startVidx, endVidx);
}

void cTurnDetector::sendMessageTurnFrameTime(
    long long startVidx, long long endVidx, int statusFlag,
    int endingForced, double startSmileTime, double endSmileTime) {
  sendMessageTurnFrameTime(recFramer,
      startVidx, endVidx, statusFlag, endingForced, startSmileTime,
      endSmileTime);
}

eTickResult cTurnDetector::myTick(long long t) {
  // Send a end-of-turn message at the end of input!
  if ((isEOI() || timeout || isPaused()) && turnState) {
    SMILE_IMSG(debug, "turn end at EOI (%i|%i|%i|%i) at vIdx %i (tick nr %llu)!",
        isEOI(),timeout,isPaused(),turnState,lastVIdx - eoiMis,t);
    if (recComp!=NULL) {
      SMILE_IDBG(3, "forced turnEnd message!");
      /*cComponentMessage cmsg("turnEnd");
      cmsg.floatData[0] = (double)nPost;
      cmsg.floatData[1] = (double)(lastVIdx - eoiMis);
      cmsg.floatData[2] = (double)(reader_->getLevelT());
      cmsg.intData[0] = 0; // 0 = incomplete turn...
      cmsg.intData[1] = 2;  // set forced turn end flag to EOI (2)
      cmsg.userTime1 = lastVTime;
      sendComponentMessage( recComp, &cmsg );
      SMILE_IMSG(debug, "sending turnEnd message to '%s' (due to TIMEOUT)",recComp);*/
      sendMessageTurnEnd(nPost, lastVIdx - eoiMis, lastVTime, 0, 2);
    }
    if (recFramer!=NULL) {
      if (sendTurnFrameTimeMessageAtEnd_
          && (minTurnLengthFrameTimeFrames_ == 0 || lastVIdx - eoiMis - startP >= minTurnLengthFrameTimeFrames_)) {
        long long myStartIdx = startP;
        long long myEndIdx = lastVIdx - eoiMis;
        if (sendTurnFrameTimeMessageAtEnd_ == 2 && msgPeriodicMaxLengthFrames_ > 0 &&
            (myEndIdx - msgPeriodicMaxLengthFrames_ > startP)) {
          myStartIdx = myEndIdx - msgPeriodicMaxLengthFrames_;
        } else {
          myStartIdx += turnFrameTimePreRollFrames_;
        }
        sendMessageTurnFrameTime(myStartIdx, myEndIdx, 0, 2,
            startSmileTime, endSmileTime);
        #if 0
        cComponentMessage cmsg("turnFrameTime");
        cmsg.intData[0] = 0; /* 0=incomplete turn */
        // send start/end in frames of input level
        cmsg.floatData[0] = (double)startP;
        cmsg.floatData[1] = (double)(lastVIdx - eoiMis);
        if (sendTurnFrameTimeMessageAtEnd_ == 2 && msgPeriodicMaxLengthFrames_ > 0) {
          if (cmsg.floatData[1] - msgPeriodicMaxLengthFrames_ > startP)
            cmsg.floatData[0] = cmsg.floatData[1] - msgPeriodicMaxLengthFrames_;
          else
            cmsg.floatData[0] += turnFrameTimePreRollFrames_;
        } else {
          cmsg.floatData[0] += turnFrameTimePreRollFrames_;
        }
        if (cmsg.floatData[1] < 2)
          cmsg.floatData[1] = 2;
        if (cmsg.floatData[0] < 0)
          cmsg.floatData[0] = 0;
        if (cmsg.floatData[1] <= cmsg.floatData[0])
          SMILE_IERR(1, "turn has negative or zero length! Somthing is wrong! (%ld - %ld) (turn end due to timeout)",
              (long)cmsg.floatData[0], (long)cmsg.floatData[1]);
        double T = (double)(reader_->getLevelT());
        if (T != 0.0) {
          // also send start/end as actual data time in seconds
          cmsg.floatData[2] = ((double)startP) * T;
          cmsg.floatData[3] = ((double)(lastVIdx - eoiMis)) * T;
          // and send period of input level
          cmsg.floatData[4] = T;
        }
        cmsg.intData[1] = 2; // intData[1] : 0=normal end / 1=forced turn end (max length) / 2=forced turn End (EOI)
        cmsg.userTime1 = startSmileTime;
        cmsg.userTime2 = endSmileTime;
        sendComponentMessage(recFramer, &cmsg);
        SMILE_IMSG(debug, "sending turnFrameTime message (turn end) to '%s' (due to TIMEOUT) (%ld - %ld)",
            recFramer, (long)cmsg.floatData[0], (long)cmsg.floatData[1]);
#endif
      } else {
        if (!sendTurnFrameTimeMessageAtEnd_) {
          SMILE_IMSG(debug, "NOT sending turnFrameTime message to '%s' (due to TIMEOUT) (%ld - %ld): sending at end disabled",
              recFramer, startP, lastVIdx - eoiMis);
        } else {
          SMILE_IMSG(debug, "NOT sending turnFrameTime message to '%s' (due to TIMEOUT) (%ld - %ld): turn too short",
              recFramer, startP, lastVIdx - eoiMis);
        }
      }
    }

    turnState = 0;
    cnt1 = 0; cnt2 = 0; turnTime=0.0;
  }

  // get next frame from dataMemory
  cVector *vec = NULL;
  int canWrite = writer_->checkWrite(1);
  if (canWrite) {
    vec = reader_->getNextFrame();
  }
  if (vec == NULL) {
    if ((timeoutSec>0)&&(lastDataTime>0)&&(getSmileTime()-lastDataTime > timeoutSec)) {
      timeout = 1;
    } else {
      timeout = 0;
    } // timeout flag needs to be reset when there is new data
    return canWrite ? TICK_SOURCE_NOT_AVAIL : TICK_DEST_NO_SPACE;
  }
  // Note: when paused by the component manager, we need to reset the lastDataTime to current time in the resume method
  lastDataTime = getSmileTime();

  if (vec->tmeta) {
    lastVIdx = vec->tmeta->vIdx;
    lastVTime = vec->tmeta->smileTime;
  }
  
  cVector *vec0 = new cVector(1);  // TODO: move vec0 to class...
  FLOAT_DMEM *src = vec->data;
  FLOAT_DMEM *dst = vec0->data;

  if (rmsIdx < 0) {
    if (autoRmsIdx < 0) { // if autoRmsIdx has not been set yet
      if (readVad) {
        autoRmsIdx = vec->fmeta->fieldToElementIdx( vec->fmeta->findFieldByPartialName( "vadBin" ) );
        SMILE_IDBG(3,"automatically found vadBin field at index %i in input vector",autoRmsIdx);
      } else {
        if (useRMS) {
          autoRmsIdx = vec->fmeta->fieldToElementIdx( vec->fmeta->findFieldByPartialName( "RMS" ) );
          SMILE_IDBG(3,"automatically found RMSenergy field at index %i in input vector",autoRmsIdx);
        } else {
          autoRmsIdx = vec->fmeta->fieldToElementIdx( vec->fmeta->findFieldByPartialName( "LOG" ) );
          SMILE_IDBG(3,"automatically found LOGenergy field at index %i in input vector",autoRmsIdx);
        }
      }
    }
    rmsIdx = autoRmsIdx;
  }

  /*
   * get a second field (energy)
   * do an automatic energy threshold updating  OR fixed threhold
   * get the minimum from histogram
   * set vad to 0 if below that energy threshold
   */

  // just to be sure we don't exceed arrray bounds...
  if (rmsIdx >= vec->N)
    rmsIdx = vec->N-1;
  
  //printf("s : %f\n",src[rmsIdx]);
  if (autoThreshold) updateThreshold(src[rmsIdx]);

  //if (src[rmsIdx] > threshold) {

  if (segmentFile_ && preLoadedSegments_.size() > 0) {
    // pre loaded segments mode:
    // to init:
    nPre = 0;
    nPost = 0;

    if (!turnState) {
      // find start if index is behind
      while (preLoadedSegmentsIndex_ < preLoadedSegments_.size()
          && lastVIdx > preLoadedSegments_.at(preLoadedSegmentsIndex_).endVidx) {
        preLoadedSegmentsIndex_++;
      }
      if (lastVIdx >= preLoadedSegments_.at(preLoadedSegmentsIndex_).startVidx) {
        turnState = 1;
        startP = preLoadedSegments_.at(preLoadedSegmentsIndex_).startVidx;
        turnTime = 0.0;
        startSmileTime = preLoadedSegments_.at(preLoadedSegmentsIndex_).startTime;
        sendMessageTurnStart();
      }
    } else if (turnState) {
      if (lastVIdx >= preLoadedSegments_.at(preLoadedSegmentsIndex_).endVidx) {
        turnState = 0;
        turnTime = 0.0;
        sendMessageTurnEnd(0, preLoadedSegments_.at(preLoadedSegmentsIndex_).endVidx,
            preLoadedSegments_.at(preLoadedSegmentsIndex_).endTime, 1, 0);
      }
    }

    // update turnState:
    turnState = 1;

  } else if (!autoThreshold || calCnt > 200) { /* TODO: make autoThresholdCalibrationTime configurable!*/
    // live detection mode
    int vo = isVoice(src, turnState);
    // check for maxTurnLength...
    if (turnState && (maxTurnLength > 0) && !forceEnd) {
      if ((grace > 0)&&(vec->tmeta->vIdx - startP  > maxTurnLength)) {
        // during grace period, end turn even if only 1 frame is non-voiced (no nPost smoothing)
        if (vo==0) { 
          //cnt2 = nPost-eoiMis; // <<--offline!
          cnt2 = nPost+1; // <--online 
          forceEnd = 1; 
        }
      }
      if ( (vec->tmeta->vIdx - startP)  > (maxTurnLength+grace) ) {
        // grace period expired, hard turn end forced right here...
        //cnt2 = nPost-eoiMis; // <<--offline!
        cnt2 = nPost+1; // <--online 
        forceEnd = 1;
      }
    }

    // check for smile message based speech detection block ...
    int statusBlocked=0;
    if (blockAll) {
      lockMessageMemory();
      if ((blockTurn) || (unblockTurnCntdn > 0)) {
        /*
        if (turnState && vo) {
        cnt2 = nPost+1; /// <<-online only
        }
        */
        vo=0;
        if (unblockTurnCntdn > 0) {
          unblockTurnCntdn--;
          if ((unblockTurnCntdn == 0)&&(vo)) { cnt1 = nPre+20; }
          // careful: this (the +20) might break things if there are sounds when the character finished speaking...
        }
      }
      unlockMessageMemory();
    } else {
      if (blockStatus) {
        lockMessageMemory();
        if ((blockTurn) || (unblockTurnCntdn > 0)) {
          statusBlocked = 1;
          if (unblockTurnCntdn > 0) {
            unblockTurnCntdn--;
          }
        }
        unlockMessageMemory();
      }
    }

    // apply the force turn end flag ..
    if (forceEnd) {
      vo = 0; 
    }

    if (vec->tmeta->vIdx < initialBlockFrames) {
      vo = 0;
    }
    if (vo) {  /* if voice activity */
      cnt2 = 0;
      cnt2s=0;
      if (!actState) {
        if (cnt1s<=0) { 
          if (vec->tmeta != NULL) startSmileTimeS = vec->tmeta->smileTime; 
        }
        cnt1s++;
        if (cnt1s > 1) {
          actState = 1;
          cnt1s=0;
          cnt2s=0;
          if (statusRecp != NULL) {
            SMILE_IMSG(debug + 1, "detected voice activity start at vIdx %i!",
                vec->tmeta->vIdx - 1);
            cComponentMessage cmsg("turnSpeakingStatus");
            cmsg.intData[0] = 1;
            cmsg.floatData[1] = (double)(vec->tmeta->vIdx-1);
            cmsg.floatData[2] = (double)(reader_->getLevelT());
            cmsg.userTime1 = startSmileTimeS;
            sendComponentMessage( statusRecp, &cmsg );
            SMILE_IDBG(4,"sending turnSpeakingStatus (1) message to '%s'",recComp);
          }
        }
      }

      if (!turnState) {
        if (cnt1<=0) { 
          if (vec->tmeta != NULL) startSmileTime = vec->tmeta->smileTime;/*offline: time !  FIX THIS PERMANENTLY...*/
        }
        cnt1++;
        if (cnt1 > nPre) {
          //printf("XXX vo=%i turnState=%i actState=%i cnt2=%i\n",vo,turnState,actState,cnt2);
          startP = vec->tmeta->vIdx - cnt1 + 1; // cnt1 was nPre ! // added the +1 to fix the vIdx=-1 issue...
          turnState = 1;
          cnt1 = 0;
          cnt2 = 0;
          turnTime=0.0;
          SMILE_IMSG(debug,"detected turn start at vIdx %i!",startP);
          sendMessageTurnStart();
          /*if (recComp!=NULL) {
            cComponentMessage cmsg("turnStart");
            cmsg.floatData[0] = (double)nPre;
            cmsg.floatData[1] = (double)startP;
            cmsg.floatData[2] = (double)(reader_->getLevelT());
            cmsg.userTime1 = startSmileTime;
            sendComponentMessage( recComp, &cmsg );
            SMILE_IMSG(debug,"sending turnStart message to '%s'",recComp);
          }*/
        }
      }
    } else {
      cnt1 = 0;
      cnt1s=0;
      if (cnt2 <= 0) {
        if (vec->tmeta != NULL)
          endSmileTime = vec->tmeta->smileTime;
          /*offline: time !  FIX THIS PERMANENTLY...*/
      }
      if (actState) {
        cnt2s++;
        if (cnt2s > 5) {
          actState = 0;
          cnt2s=0;
          cnt1s=0;
          if (statusRecp != NULL) {
            SMILE_IMSG(debug + 1, "detected voice activity end at vIdx %i!",
                vec->tmeta->vIdx - 2);
            cComponentMessage cmsg("turnSpeakingStatus");
            cmsg.intData[0] = 0;
            cmsg.floatData[1] = (double)(vec->tmeta->vIdx - 2);
            cmsg.floatData[2] = (double)(reader_->getLevelT());
            cmsg.userTime1 = endSmileTime;
            sendComponentMessage( statusRecp, &cmsg );
            SMILE_IDBG(debug+1,"sending turnSpeakingStatus (0) message to '%s'",recComp);
          }
        }
      }
      cnt2++;
      if (exitFlag == 1) {
        nSilForExit++;
        if (nSilForExit >= terminatePostSil) {
          exitFlag = 2;
        }
      }
      if (turnState) {
        if (cnt2 > nPost) {
          turnState = 0;
          cnt1 = 0;
          cnt2 = 0;
          SMILE_IMSG(debug,"detected turn end at vIdx %i !",(vec->tmeta->vIdx)-nPost);
          // WARNING: if we set cnt2 to nPost manually we run the risk of having negative turn lengths
          //    when we subtract nPost to get to the turn end:
          //  solution:  introduce another variable that is either nPost, or set to 0 if we set cnt2 during forcing a turn end

          long blag;
          if (forceEnd)
            blag = eoiMis;
          else blag = nPost;

          if (recFramer!=NULL) {
            if ((sendTurnFrameTimeMessageAtEnd_) &&
                (minTurnLengthFrameTimeFrames_ == 0 ||
                 vec->tmeta->vIdx - blag - startP >= minTurnLengthFrameTimeFrames_)) {
              long long myStartIdx = startP;
              long long myEndIdx = vec->tmeta->vIdx;
              if (sendTurnFrameTimeMessageAtEnd_ == 2 && msgPeriodicMaxLengthFrames_ > 0 &&
                  (myEndIdx - msgPeriodicMaxLengthFrames_ > startP)) {
                myStartIdx = myEndIdx - msgPeriodicMaxLengthFrames_;
              } else {
                myStartIdx += turnFrameTimePreRollFrames_;
              }
              myEndIdx += turnFrameTimePostRollFrames_;
              sendMessageTurnFrameTime(myStartIdx, myEndIdx - blag, 1, forceEnd,
                  startSmileTime, endSmileTime);
#if 0
              cComponentMessage cmsg("turnFrameTime");
              cmsg.intData[0] = 1; /* indicates a turn end */
              // send start/end in frames of input level
              long thisStartP = startP;
              long thisEnd = vec->tmeta->vIdx;
              if (sendTurnFrameTimeMessageAtEnd_ == 2 && msgPeriodicMaxLengthFrames_ > 0) {
                if (vec->tmeta->vIdx - msgPeriodicMaxLengthFrames_ > startP)
                  thisStartP = vec->tmeta->vIdx - msgPeriodicMaxLengthFrames_;
                else
                  thisStartP += turnFrameTimePreRollFrames_;
              } else {
                thisStartP += turnFrameTimePreRollFrames_;
              }
              thisEnd += turnFrameTimePostRollFrames_;
              if (thisStartP < 0)
                thisStartP = 0;
              if (thisEnd < 2)
                thisEnd = 2;
              cmsg.floatData[0] = (double)thisStartP;
              cmsg.floatData[1] = (double)(thisEnd - blag);
              double _T = (double)(reader_->getLevelT());
              if (_T!=0.0) {
                // also send start/end as actual data time in seconds
                cmsg.floatData[2] = ((double)thisStartP) * _T;
                cmsg.floatData[3] = ((double)(thisEnd - blag)) * _T;
                // and send period of input level
                cmsg.floatData[4] = _T;
              }
              cmsg.intData[1] = forceEnd; // intData[1] : 0=normal end / 1=forced turn end (max length)
              cmsg.userTime1 = startSmileTime;
              cmsg.userTime2 = endSmileTime;
              sendComponentMessage(recFramer, &cmsg);
              SMILE_IMSG(debug, "sending turnFrameTime message (turn end) to '%s' (%ld - %ld, blag: %ld).",
                  recFramer, thisStartP, thisEnd - blag, blag);
#endif
            } else {
              if (sendTurnFrameTimeMessageAtEnd_) {
                SMILE_IMSG(debug, "NOT sending turnFrameTime message (turn end) to '%s': turn too short (%ld -> %ld). Min length frames = %ld",
                    recFramer, startP, vec->tmeta->vIdx - blag, minTurnLengthFrameTimeFrames_);
              } else {
                SMILE_IMSG(debug, "NOT sending turnFrameTime message (turn end) to '%s': sending at end disabled. (%ld -> %ld)",
                    recFramer, startP, vec->tmeta->vIdx - blag);
              }
            }
          }
          /*
           * TODO: turn end is never sent, always too short! check;: vec->tmeta->vIdx - blag - startP
           * FIXME: if nPost is large, then it might happen that an intermediate message is sent
           *     and the turn end then retrospective is detected before that point!!
           *     Solution? We can only send intermediates up to vIdx - nPost
           *       Then at the turn end, we have to catch up all intermediate between end and end-nPost!
           *     We should also fix the send at end option in this context!
           */
          sendMessageTurnEnd(nPost, vec->tmeta->vIdx - blag,
              endSmileTime, 1 /* turn end */, forceEnd);
          /*
          if (recComp!=NULL) {
            cComponentMessage cmsg("turnEnd");
            cmsg.floatData[0] = (double)nPost;
            cmsg.floatData[1] = (double)(vec->tmeta->vIdx - blag);
            cmsg.floatData[2] = (double)(reader_->getLevelT());
            cmsg.intData[0] = 1;  // turn end
            cmsg.intData[1] = forceEnd; // intData[1] : 0=normal end / 1=forced turn end (max length)
            cmsg.userTime1 = endSmileTime;
            sendComponentMessage( recComp, &cmsg );
            SMILE_IDBG(debug, "sending turnEnd message to '%s'",recComp);
          }
          */
          forceEnd = 0;
          // increase number of turns counter
          nTurns++;
          //printf("nTurns = %i\n", nTurns);
          if (terminateAfterTurns > 0 && nTurns >= terminateAfterTurns) {
            exitFlag = 1;
            nSilForExit = 0;
          }
        }
      }
    }
  } else {
    calCnt++;
  }

  /* if turnState and interval timeout, then send message */
  if ((turnState) && (msgInterval > 0.0)) {
    if (vec->tmeta->vIdx - nPre - startP > minTurnLengthFrameTimeFrames_) {
      if (turnTime > msgInterval) {
        turnTime = 0.0;
        long long myStartIdx = startP;
        long long myEndIdx = vec->tmeta->vIdx - nPre;
        if (msgPeriodicMaxLengthFrames_ > 0 &&
            (myEndIdx - msgPeriodicMaxLengthFrames_ > startP)) {
          myStartIdx = myEndIdx - msgPeriodicMaxLengthFrames_;
        } else {
          myStartIdx += turnFrameTimePreRollFrames_;
        }
        double endTime = startSmileTime;
        if (vec->tmeta != NULL)
          endTime = vec->tmeta->smileTime;
        sendMessageTurnFrameTime(myStartIdx, myEndIdx, 0, 0,
            startSmileTime, endTime);
#if 0
        if (recFramer!=NULL) {
          cComponentMessage cmsg("turnFrameTime");
          cmsg.intData[0] = 0; /* indicates an incomplete turn */
          // send start/end in frames of input level
          long thisStartP = startP;
          long thisEnd = vec->tmeta->vIdx - nPre;
          if (msgPeriodicMaxLengthFrames_ > 0) {
            if (vec->tmeta->vIdx - nPre - msgPeriodicMaxLengthFrames_ > startP)
              thisStartP = vec->tmeta->vIdx - msgPeriodicMaxLengthFrames_;
            else // pre roll only applied if at real turn start
              thisStartP += turnFrameTimePreRollFrames_;
          } else {
            // pre roll only applied if at real turn start
            thisStartP += turnFrameTimePreRollFrames_;
          }
          // post roll is not applied, as this is intra turn!
          if (thisStartP < 0)
            thisStartP = 0;
          if (thisEnd < 2)
            thisEnd = 2;
          cmsg.floatData[0] = (double)thisStartP;
          cmsg.floatData[1] = (double)thisEnd;
          double _T = (double)(reader_->getLevelT());
          if (_T!=0.0) {
            // also send start/end as actual data time in seconds
            cmsg.floatData[2] = ((double)thisStartP) * _T;
            cmsg.floatData[3] = ((double)thisEnd) * _T;
            // and send period of input level
            cmsg.floatData[4] = _T;
          }
          cmsg.userTime1 = startSmileTime;
          if (vec->tmeta != NULL)
            cmsg.userTime2 = vec->tmeta->smileTime;
          else
            cmsg.userTime2 = startSmileTime;
          sendComponentMessage(recFramer, &cmsg);
          SMILE_IMSG(debug, "sending turnFrameTime message (intra turn) to '%s' (%ld -> %ld)",
              recFramer, thisStartP, thisEnd);
        }
#endif
      } 
    }
    turnTime += (double)(reader_->getLevelT());
  }

  dst[0] = (FLOAT_DMEM)turnState;
  // save to dataMemory
  writer_->setNextFrame(vec0);
  delete vec0;

  if (terminateAfterTurns > 0 && exitFlag == 2) {
    // stop the component manager!
    abortProcessing();
    exitFlag = 3;
    return TICK_INACTIVE;
  }
  return TICK_SUCCESS;
}

int cTurnDetector::processComponentMessage( cComponentMessage *_msg )
{
  if (isMessageType(_msg,"semaineCallback")) {
    // determine origin by message's user-defined name, which can be set in the config file
    SMILE_IMSG(3,"received 'semaineCallback' message '%s'",_msg->msgname);
    if (!strncmp(_msg->msgname,"start",5)) { blockTurn = 1; unblockTurnCntdn = 0; }
    else if (!strncmp(_msg->msgname,"end",3)) {
      blockTurn = 0; unblockTurnCntdn = unblockTimeout;
    }
  /*  else if (!strncmp(_msg->msgname,"present",7)) { if (userPresence != 1) { userPresence = 1; doReset=1; } }
    else if (!strncmp(_msg->msgname,"absent",6)) { if (userPresence != 0) { userPresence = 0; doReset=1; } }*/
    return 1;  // message was processed
  }
  return 0; // if message was not processed
}


cTurnDetector::~cTurnDetector()
{
}

