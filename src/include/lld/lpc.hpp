/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

LPC, compute LPC coefficients from wave data (PCM) frames

*/


#ifndef __CLPC_HPP
#define __CLPC_HPP


#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#include <dspcore/fftXg.h>

#define BUILD_COMPONENT_Lpc
#define COMPONENT_DESCRIPTION_CLPC "This component computes linear predictive coding (LPC) coefficients from PCM frames. Burg's algorithm and the standard ACF/Durbin based method are implemented for LPC coefficient computation. The output of LPC filter coefficients, reflection coefficients, residual signal, and LP spectrum is supported."

#define COMPONENT_NAME_CLPC "cLpc"

#define LPC_METHOD_ACF   0
#define LPC_METHOD_BURG   5


class cLpc : public cVectorProcessor {
  private:
    int p;
    int saveLPCoeff, saveRefCoeff;
    int residual;
    int residualGainScale;
    int method;
    int lpGain;
    int forwardRes;
    int lpSpectrum;
    int forwardLPspec;
    double lpSpecDeltaF;
    int lpSpecBins;

    FLOAT_DMEM forwardLPspecFloor;
    FLOAT_DMEM *latB;
    FLOAT_DMEM lastGain;
    FLOAT_TYPE_FFT *lSpec;

    int *_ip;
    FLOAT_TYPE_FFT *_w;

    FLOAT_DMEM *acf;
    FLOAT_DMEM *lpCoeff, *lastLpCoeff, *refCoeff;

    FLOAT_DMEM *gbb, *gb2, *gaa;

    FLOAT_DMEM calcLpc(const FLOAT_DMEM *x, long Nsrc, FLOAT_DMEM * lpc, long nCoeff, FLOAT_DMEM *refl);

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    virtual int setupNamesForField(int i, const char*name, long nEl) override;

    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cLpc(const char *_name);

    virtual ~cLpc();
};


#endif // __CLPC_HPP
