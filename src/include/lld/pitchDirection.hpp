/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example for dataProcessor descendant

*/


#ifndef __CPITCHDIRECTION_HPP
#define __CPITCHDIRECTION_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>

#define BUILD_COMPONENT_PitchDirection
#define COMPONENT_DESCRIPTION_CPITCHDIRECTION "This component reads pitch data, detects pseudo syllables, and computes pitch direction estimates per syllable. Thereby the classes falling, flat, and rising are distinguished. \n    Required input fields: F0, F0env, and 'loudness' or 'RMSenergy'."
#define COMPONENT_NAME_CPITCHDIRECTION "cPitchDirection"

class cPitchDirection : public cDataProcessor {
  private:
    cVector *myVec;
    long F0field, F0envField, LoudnessField, RMSField;
    double stbs, ltbs;
    long stbsFrames, ltbsFrames;
    FLOAT_DMEM * stbuf, * ltbuf;
    FLOAT_DMEM F0non0, lastF0non0, f0s;
    long stbufPtr, ltbufPtr; /* ring-buffer pointers */
    long bufInit; /* indicates wether buffer has been filled and valid values are to be expected */
    double ltSum, stSum;
    FLOAT_DMEM longF0Avg;
    long nFall,nRise,nFlat;

    int insyl;
    int f0cnt;
    FLOAT_DMEM lastE;
    FLOAT_DMEM startE, maxE, minE, endE;
    long sylen, maxPos, minPos, sylenLast;
    long sylCnt;
    double inpPeriod, timeCnt, lastSyl;

    FLOAT_DMEM startF0, lastF0, maxF0, minF0;
    long maxF0Pos, minF0Pos;

    const char *directionMsgRecp;
    int speakingRateBsize;

    int F0directionOutp, directionScoreOutp;
    int speakingRateOutp;
    int F0avgOutp, F0smoothOutp;

    /* speaking rate variables (buf0 is always the first half of buf1) */
    int nBuf0,nBuf1; /* number of frames collected in bufferA and bufferB */
    int nSyl0,nSyl1; /* number of syllable starts in bufferA and bufferB */
    double curSpkRate;

    int nEnabled;
    
    int isTurn, onlyTurn, invertTurn;
    int resetTurnData;
    const char * turnStartMessage, * turnEndMessage;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual void sendPitchDirectionResult(int result, double _smileTime, const char * _directionMsgRecp);

    virtual int processComponentMessage(cComponentMessage *_msg) override;

    // virtual int dataProcessorCustomFinalise() override;
     virtual int setupNewNames(long nEl) override;
    // virtual int setupNamesForField() override;
    virtual int configureWriter(sDmLevelConfig &c) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cPitchDirection(const char *_name);

    virtual ~cPitchDirection();
};




#endif // __CPITCHDIRECTION_HPP
