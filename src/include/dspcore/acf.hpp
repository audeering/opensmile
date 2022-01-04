/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Autocorrelation Function (ACF)

*/


#ifndef __CACF_HPP
#define __CACF_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>
#include <dspcore/fftXg.h>
#include <math.h>

#define COMPONENT_DESCRIPTION_CACF "This component computes the autocorrelation function (ACF) by squaring a magnitude spectrum and applying an inverse Fast Fourier Transform. This component must read from a level containing *only* FFT magnitudes in a single field. Use the 'cTransformFFT' and 'cFFTmagphase' components to compute the magnitude spectrum from PCM frames. Computation of the Cepstrum is also supported (this applies a log() function to the magnitude spectra)."
#define COMPONENT_NAME_CACF "cAcf"

class cAcf : public cVectorProcessor {
  private:
    int absCepstrum_;
    int oldCompatCepstrum_;
    int acfCepsNormOutput_;
    int symmetricData;
    int expBeforeAbs;
    int cosLifterCepstrum;
    int usePower, cepstrum, inverse;
    FLOAT_TYPE_FFT **data;
    FLOAT_TYPE_FFT **w;
    FLOAT_DMEM **winFunc;
    int **ip;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
//    virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    //virtual eTickResult myTick(long long t) override;

    virtual int dataProcessorCustomFinalise() override;

//    virtual void configureField(int idxi, long __N, long nOut) override;
    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cAcf(const char *_name);

    virtual ~cAcf();
};




#endif // __CACF_HPP
