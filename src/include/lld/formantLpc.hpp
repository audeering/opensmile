/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

LPC, compute LPC coefficients from wave data (PCM) frames

*/


#ifndef __CFORMANTLPC_HPP
#define __CFORMANTLPC_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define BUILD_COMPONENT_FormantLpc
#define COMPONENT_DESCRIPTION_CFORMANTLPC "This component computes formant frequencies and bandwidths by solving for the roots of the LPC polynomial. The formant trajectories can and should be smoothed by the cFormantSmoother component."
#define COMPONENT_NAME_CFORMANTLPC "cFormantLpc"

class cFormantLpc : public cVectorProcessor {
  private:
    int nFormants;
    int saveFormants;
    int saveIntensity;
    int saveBandwidths;
    int saveNumberOfValidFormants;
    int useLpSpec;
    int nSmooth;
    int medianFilter, octaveCorrection;

    int nLpc;
    long lpcCoeffIdx;
    long lpcGainIdx;
    long lpSpecIdx, lpSpecN;

    double minF, maxF;
    
    double T;
    double *lpc, *roots;
    double *formant, *bandwidth;

    FLOAT_DMEM calcFormantLpc(const FLOAT_DMEM *x, long Nsrc, FLOAT_DMEM * lpc, long nCoeff, FLOAT_DMEM *refl);
  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    virtual int setupNewNames(long nEl) override;
    virtual void findInputFields();

    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFormantLpc(const char *_name);

    virtual ~cFormantLpc();
};

#endif // __CFORMANTLPC_HPP
