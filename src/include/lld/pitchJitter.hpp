/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example for dataProcessor descendant

*/


#ifndef __CPITCHJITTER_HPP
#define __CPITCHJITTER_HPP

#include <core/smileCommon.hpp>
#include <smileutil/smileUtil.h>
#include <core/dataProcessor.hpp>

#define BUILD_COMPONENT_PitchJitter
#define COMPONENT_DESCRIPTION_CPITCHJITTER "This component computes Voice Quality parameters Jitter (pitch period deviations) and Shimmer (pitch period amplitude deviations). It requires the raw PCM frames and the corresponding fundamental frequency (F0) as inputs."
#define COMPONENT_NAME_CPITCHJITTER "cPitchJitter"

class cPitchJitter : public cDataProcessor {
  private:
    int onlyVoiced;
    long savedMaxDebugPeriod;
    int jitterLocal, jitterDDP, shimmerLocal, shimmerLocalDB;
    int jitterLocalEnv, jitterDDPEnv, shimmerLocalEnv, shimmerLocalDBEnv;
    int shimmerUseRmsAmplitude;
    int harmonicERMS, noiseERMS, linearHNR, logHNR;
    int sourceQualityRange, sourceQualityMean;
    int useBrokenJitterThresh_;

    int periodLengths, periodStarts;
    long F0fieldIdx;
    long lastIdx, lastMis;
    int refinedF0;
    int usePeakToPeakPeriodLength_;
    FLOAT_DMEM threshCC_;
    int minNumPeriods;

    FILE *filehandle;
    long input_max_delay_;
    double searchRangeRel;
    double minF0;
    const char * F0field;
    cDataReader *F0reader;
    cVector *out;
    long Nout;

    // jitter data memory:
    FLOAT_DMEM lastT0;
    FLOAT_DMEM lastDiff;
    FLOAT_DMEM lastJitterDDP, lastJitterLocal;
    FLOAT_DMEM lastJitterDDP_b, lastJitterLocal_b;

    // shimmer data memory:
    FLOAT_DMEM lastShimmerLocal;
    FLOAT_DMEM lastShimmerLocal_b;

    FLOAT_DMEM lgHNRfloor;

    double crossCorr(FLOAT_DMEM *x, long Nx, FLOAT_DMEM *y, long Ny);
    FLOAT_DMEM amplitudeDiff(FLOAT_DMEM *x, long Nx, FLOAT_DMEM *y, long Ny,
        double *maxI0, double *maxI1, FLOAT_DMEM *_A0, FLOAT_DMEM *_A1);
    FLOAT_DMEM rmsAmplitudeDiff(FLOAT_DMEM *x, long Nx, FLOAT_DMEM *y, long Ny,
        double *maxI0, double *maxI1, FLOAT_DMEM *_A0, FLOAT_DMEM *_A1);
    void saveDebugPeriod(long sample, double sampleInterp);

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    virtual void mySetEnvironment() override;
    virtual int myRegisterInstance(int *runMe=NULL) override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    // virtual int dataProcessorCustomFinalise() override;
    virtual int setupNewNames(long nEl) override;
    // virtual int setupNamesForField() override;
    virtual int configureReader(const sDmLevelConfig &c) override;
    virtual int configureWriter(sDmLevelConfig &c) override;


  public:
    SMILECOMPONENT_STATIC_DECL
    
    cPitchJitter(const char *_name);

    virtual ~cPitchJitter();
};




#endif // __CPITCHJITTER_HPP
