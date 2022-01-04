/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

BLSTM RNN processor

*/


#ifndef __CRNNVAD2_HPP
#define __CRNNVAD2_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>
#include <rnn/rnn.hpp>

#define COMPONENT_DESCRIPTION_CRNNVAD2 "BLSTM RNN processor."
#define COMPONENT_NAME_CRNNVAD2 "cRnnVad2"



/* ring buffer average computation over holdTime + decayTime segments */
/* hold of max over holdTime, decay over decayTime */
class cEavgHold 
{
private:
  /* moving average */
  FLOAT_DMEM *Ebuf;
  long EbufSize, nAvg;
  long EbufPtr, EbufPtrOld;
  double EavgSumCur;
  FLOAT_DMEM EmaxRiseAlpha; /* alpha for exponential rise smoothing of max energy: 1= rise immediately, 0=do not follow input */

  /* envelope */
  FLOAT_DMEM Emax;
  long EmaxAge;
  FLOAT_DMEM EmaxDecayStep;

  int holdTime;
  int decayTime;

public:
  /* holdTime and decayTime in frames/samples */
  cEavgHold(int _holdTime, int _decayTime, int _decayFunct=0, FLOAT_DMEM _EmaxRiseAlpha=0.2) :
      holdTime(_holdTime),
      decayTime(_decayTime),
      EmaxRiseAlpha(_EmaxRiseAlpha)
  {
    if (decayTime <= 0) decayTime = 1;
    if (holdTime <= 0) holdTime = 1;

    EbufSize = holdTime + decayTime;
    EbufPtr = 0;
    EbufPtrOld = 0;
    Ebuf = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*EbufSize);
    EavgSumCur = 0.0; nAvg = 0;
    Emax = 0.0;
    EmaxAge = 0;
    EmaxDecayStep = 0.0;
  }

  /* add next input to buffer */
  void nextE(FLOAT_DMEM E)
  {
    /* average: */
    EavgSumCur -= Ebuf[EbufPtrOld];
    Ebuf[EbufPtr] = E;
    EbufPtrOld = EbufPtr++;
    EavgSumCur += E;
    if (EbufPtr >= EbufSize) EbufPtr %= EbufSize;
    if (nAvg < EbufSize) nAvg++;
    /* envelope: */
    if (E > Emax) {
      Emax = Emax*((FLOAT_DMEM)1.0-EmaxRiseAlpha) + E*EmaxRiseAlpha;
      EmaxDecayStep = Emax/(FLOAT_DMEM)decayTime;
      EmaxAge = 0;
    } else {
      EmaxAge++;
      if (EmaxAge > holdTime && EmaxAge < holdTime+decayTime && Emax > EmaxDecayStep) {
        Emax -= EmaxDecayStep;
      }
    }
  }

  /* get the current short-term average */
  FLOAT_DMEM getAvg() 
  {
    if (nAvg > 100 || nAvg >= EbufSize) {
      return (FLOAT_DMEM) ( EavgSumCur / (double)nAvg ) ;
    } else { return 0.0; }
  }

  /* get the current envelope (max. hold) */
  FLOAT_DMEM getEnv() 
  {
    if (nAvg > 100  || nAvg >= EbufSize) return Emax;
    else return 0.0;
  }

  ~cEavgHold()
  {
    if (Ebuf != NULL) free(Ebuf);
  }
};


class cRnnVad2 : public cDataProcessor {
  private:
    cEavgHold * eUser;
    cEavgHold * eCurrent;
    cEavgHold * eAgent;
    cEavgHold * eBg;
    
    long voiceIdx, agentIdx, energyIdx, f0Idx;

    long cnt; 
    int isV;
    int vadDebug, allowEoverride;
    cVector *frameO;
    
    FLOAT_DMEM voiceThresh, agentThresh;
    
    //long agentBlockTime;
    long agentTurnCntdn;
    long agentTurnPastBlock;
    int alwaysRejectAgent, smartRejectAgent;
    int doReset, agentTurn, userPresence;
    long userCnt, agentCntdn;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;
    virtual int processComponentMessage(cComponentMessage *_msg) override;

    virtual int setupNewNames(long nEl) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cRnnVad2(const char *_name);

    virtual ~cRnnVad2();
};




#endif // __CRNNVAD2_HPP
