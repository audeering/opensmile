/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

fast fourier transform using fft4g library
output: complex values of fft or real signal values (for iFFT)

*/


#ifndef __TRANSFORM_FFT_HPP
#define __TRANSFORM_FFT_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>
#include <dspcore/fftXg.h>

#define COMPONENT_DESCRIPTION_CTRANSFORMFFT "This component performs an FFT on a sequence of real values (one frame), the output is the complex domain result of the transform. Use the cFFTmagphase component to compute magnitudes and phases from the complex output."
#define COMPONENT_NAME_CTRANSFORMFFT "cTransformFFT"

class cTransformFFT : public cVectorProcessor {
  private:
    int inverse_;
    int **ip_;
    FLOAT_TYPE_FFT **w_;
    FLOAT_TYPE_FFT **xconv_;
    int newFsSet_;
    double frameSizeSecOut_;
    int zeroPadSymmetric_;

    // generate "frequency axis information", i.e. the frequency in Hz for each spectral bin
    // which is to be saved as meta-data in the dataMemory level field (FrameMetaInfo->FieldMetaInfo->info)
    // &infosize is initialized with the number of fft bins x 2 (= number of input samples)
    //   and should contain the number of complex bins at the end of this function
    void * generateSpectralVectorInfo(long &infosize);

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    virtual int myFinaliseInstance() override;

    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;


  public:
    SMILECOMPONENT_STATIC_DECL
    
    cTransformFFT(const char *_name);

    virtual ~cTransformFFT();
};




#endif // __TRANSFORM_FFT_HPP
