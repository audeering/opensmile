/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

computes mean of full input ?

*/


#ifndef __CFULLTURNMEAN_HPP
#define __CFULLTURNMEAN_HPP

#pragma warning( disable : 4251 )

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

// STL includes for the queue
#include <queue>


#define COMPONENT_DESCRIPTION_CFULLTURNMEAN "This component performs mean normalizing on a data series. A 2-pass analysis of the data is performed, which makes this component unusable for on-line analysis. In the first pass, no output is produced and the mean value (over time) is computed for each input element. In the second pass the mean vector is subtracted from all input frames, and the result is written to the output dataMemory level. Attention: Due to the 2-pass processing the input level must be large enough to hold the whole data sequence."
#define COMPONENT_NAME_CFULLTURNMEAN "cFullturnMean"



// A message received from the turn detector.
struct TurnTimeMsg {
  TurnTimeMsg() : vIdxStart(0), vIdxEnd(0), isForcedTurnEnd(0) {}
  TurnTimeMsg(long _vIdxStart, long _vIdxEnd, int forceend=0) : vIdxStart(_vIdxStart), vIdxEnd(_vIdxEnd), isForcedTurnEnd(forceend) {}
  long vIdxStart, vIdxEnd;        // vector index
  int isForcedTurnEnd;  // forced turn end = at end of input
};

// A queue of turn detector messages.
typedef std::queue<TurnTimeMsg> TurnTimeMsgQueue;

class cFullturnMean : public cDataProcessor {
  private:
    TurnTimeMsgQueue msgQue;

    int dataInQue;
    long curWritePos;
    //TODO: postSil / preSil option for adding extra data at beginning and end of turn

    const char *msgRecp;
    int eNormMode;
    cVector *means;
    long nMeans;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    int checkMessageQueque(long &start, long &end, long &fte);
    virtual int processComponentMessage(cComponentMessage *_msg) override;
    //virtual int configureWriter(sDmLevelConfig &c) override;

    //virtual void configureField(int idxi, long __N, long nOut) override;
    //virtual int setupNamesForField(int i, const char*name, long nEl) override;
    //virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFullturnMean(const char *_name);

    virtual ~cFullturnMean();
};




#endif // __CFULLTURNMEAN_HPP
