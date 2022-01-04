/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:


*/


#ifndef __FFT_MAGPHASE_HPP
#define __FFT_MAGPHASE_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>
//#include "fftXg.h"

#define COMPONENT_DESCRIPTION_CFFTMAGPHASE "This component computes magnitude and phase of each array in the input level (it thereby assumes that the arrays contain complex numbers with real and imaginary parts alternating, as computed by the cTransformFFT component)."
#define COMPONENT_NAME_CFFTMAGPHASE "cFFTmagphase"

class cFFTmagphase : public cVectorProcessor {
  private:
    int inverse;
    int magnitude;
    int phase;
    int joinMagphase;
    int power;
    int normalise, dBpsd;
    FLOAT_DMEM dBpnorm;
    FLOAT_DMEM mindBp;
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;

    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;


  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFFTmagphase(const char *_name);

    virtual ~cFFTmagphase();
};




#endif // __FFT_MAGPHASE_HPP
