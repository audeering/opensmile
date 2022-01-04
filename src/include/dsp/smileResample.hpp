/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

simple preemphasis : x(t) = x(t) - k*x(t-1)

*/


#ifndef __CSMILERESAMPLE_HPP
#define __CSMILERESAMPLE_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>

#define BUILD_COMPONENT_SmileResample
#define COMPONENT_DESCRIPTION_CSMILERESAMPLE "This component implements a spectral domain resampling component. Input frames are transferred to the spectral domain, then the spectra are shifted, and a modified DFT is performed to synthesize samples at the new rate."
#define COMPONENT_NAME_CSMILERESAMPLE "cSmileResample"


class cSmileResample : public cDataProcessor {
  private:
    cMatrix *matnew; cMatrix *rowout;
    cMatrix *row;
    int flushed;
    int useQuickAlgo;

    double ND;
    double resampleRatio, targetFs, pitchRatio;
    double winSize, winSizeTarget;
    long winSizeFramesTarget, winSizeFrames;

    FLOAT_DMEM *outputBuf, *lastOutputBuf;
    FLOAT_TYPE_FFT *inputBuf;
    sResampleWork *resampleWork;
    long Ni;

    int getOutput(FLOAT_DMEM *cur, FLOAT_DMEM *last, long N, FLOAT_DMEM *out, long Nout);

  protected:
    SMILECOMPONENT_STATIC_DECL_PR


    virtual void myFetchConfig() override;
/*
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    
*/

    virtual eTickResult myTick(long long t) override;
    virtual int configureWriter(sDmLevelConfig &c) override;

   // buffer must include all (# order) past samples
    //virtual int processBuffer(cMatrix *_in, cMatrix *_out, int _pre, int _post) override;
    virtual int dataProcessorCustomFinalise() override;
/*
    virtual int setupNamesForField(int i, const char*name, long nEl) override;
*/
    //virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

    
  public:
    SMILECOMPONENT_STATIC_DECL
    
    cSmileResample(const char *_name);

    virtual ~cSmileResample();
};




#endif // __CSMILERESAMPLE_HPP
