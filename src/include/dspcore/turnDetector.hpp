/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

simple silence threshold based turn detector

*/


#ifndef __CTURNDETECTOR_HPP
#define __CTURNDETECTOR_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>
#include <smileutil/smileUtilCsv.hpp>
#include <vector>

#define COMPONENT_DESCRIPTION_CTURNDETECTOR "Speaker turn detector using data from cVadV1 component or cSemaineSpeakerID1 (adaptive VAD) to determine speaker turns and identify continuous segments of voice activity."
#define COMPONENT_NAME_CTURNDETECTOR "cTurnDetector"

class sTurnSegment {
public:
  double startTime;
  double endTime;
  long long startVidx;
  long long endVidx;
  FLOAT_DMEM voiceScore;
  sTurnSegment() : startTime(0.0), endTime(0.0),
      startVidx(0), endVidx(0), voiceScore(0.0) {}
};

class cTurnDetector : public cDataProcessor {
  private:
    int lastVIdx;
    double lastVTime;
    FLOAT_DMEM threshold, threshold2;
    int nPost, nPre;
    int useRMS;
    int turnState, actState;
    long startP, startP0, endP0;
    double endSmileTime, startSmileTime, startSmileTimeS;
    double turnTime, turnStep, msgInterval;
    double maxTurnLengthS, graceS;
    long maxTurnLength, grace;
    long minTurnLengthFrames_, minTurnLengthFrameTimeFrames_;
    double minTurnLength_;
    double minTurnLengthTurnFrameTimeMessage_;

    double turnFrameTimePreRollSec_;
    long turnFrameTimePreRollFrames_;
    double turnFrameTimePostRollSec_;
    long turnFrameTimePostRollFrames_;
    double msgPeriodicMaxLengthSec_;
    long msgPeriodicMaxLengthFrames_;
    int sendTurnFrameTimeMessageAtEnd_;
  
    int blockAll, blockStatus;
    int blockTurn, unblockTurnCntdn, unblockTimeout;
    double initialBlockTime;
    long initialBlockFrames;

    int terminateAfterTurns, terminatePostSil, nTurns;
    int exitFlag;
    long nSilForExit;

    long eoiMis;
    int forceEnd;
    int timeout; double lastDataTime;
    double timeoutSec;

    int debug;
    int cnt1, cnt2, cnt1s, cnt2s;

    long rmsIdx, autoRmsIdx, readVad;
    int autoThreshold;
    int invert_;

    // variables for auto threshold statistics:
    int nmin, nmax;
    FLOAT_DMEM minmaxDecay;
    FLOAT_DMEM rmin, rmax, rmaxlast;
    FLOAT_DMEM tmpmin, tmpmax;
    FLOAT_DMEM dtMeanO, dtMeanT, dtMeanS;
    FLOAT_DMEM alphaM;
    FLOAT_DMEM nE, dtMeanAll;
    long nTurn, nSil;
    int tmpCnt;
    int calCnt;

    const char *segmentFile_;  // load segments from this file
    std::vector<sTurnSegment> preLoadedSegments_;
    int preLoadedSegmentsIndex_;

    const char *recFramer;
    const char *recComp;
    const char *statusRecp;
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    void sendMessageTurnStart(const char * recp,
        long long npre, long long vIdx, double smileTime);
    void sendMessageTurnStart(
        long long npre, long long vIdx, double smileTime);
    void sendMessageTurnStart();
    void sendMessageTurnEnd(const char * recp,
        long long npost, long long vIdx, double smileTime,
        int complete, int turnEndFlag);
    void sendMessageTurnEnd(
        long long nPost, long long vIdx, double smileTime,
        int complete, int turnEndFlag);
    void sendMessageTurnEnd();
    void sendMessageTurnFrameTime(const char * recp,
        long long startVidx, long long endVidx, int statusFlag,
        int endingForced, double startSmileTime, double endSmileTime);
    void sendMessageTurnFrameTime(
        long long startVidx, long long endVidx, int statusFlag,
        int endingForced, double startSmileTime, double endSmileTime);
    void sendMessageTurnFrameTime();

    virtual int isVoice(FLOAT_DMEM *src, int state=0);  /* state 0: silence/noise, state 1: voice */
    virtual void updateThreshold(FLOAT_DMEM eRmsCurrent);

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;
    virtual int processComponentMessage(cComponentMessage *_msg) override;

    //virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNewNames(long nEl) override;
    virtual void resumeEvent() override {
      lastDataTime = getSmileTime();
    }
    //virtual void configureField(int idxi, long __N, long nOut) override;
    //virtual int setupNamesForField(int i, const char*name, long nEl) override;
//    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

    bool loadSegmentsFromFile(double T);

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cTurnDetector(const char *_name);

    virtual ~cTurnDetector();
};




#endif // __CTURNDETECTOR_HPP
