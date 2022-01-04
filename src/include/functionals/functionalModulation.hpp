/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

number of segments based on delta thresholding

*/


#ifndef __CFUNCTIONALMODULATION_HPP
#define __CFUNCTIONALMODULATION_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>
#include <smileutil/smileUtil.h>
#include <smileutil/smileUtilSpline.h>

#define COMPONENT_DESCRIPTION_CFUNCTIONALMODULATION "  Modulation Spectrum"
#define COMPONENT_NAME_CFUNCTIONALMODULATION "cFunctionalModulation"

class cSmileUtilWindowedMagnitudeSpectrum {
private:
  FLOAT_TYPE_FFT * fftWork_;  // fft work area (size Nfft)
  int * ip_;
  FLOAT_TYPE_FFT * w_;
  FLOAT_DMEM * winFunc_;
  int winFuncId_;
  long Nfft_;  // number of samples to perform FFT on (after zero-padding)
  long Nin_;  // number of inputs (for window function)

protected:
  void allocateWinFunc(long Nin);
  void allocateFFTworkspace(long Nin);
  void freeFFTworkspace();
  void freeWinFunc();
  void copyInputAndZeropad(const FLOAT_DMEM *in, long Nin, bool allowWinSmaller);
  void doFFT();
  void computeMagnitudes();

public:
  cSmileUtilWindowedMagnitudeSpectrum():
    fftWork_(NULL),
    ip_(NULL), w_(NULL),
    winFunc_(NULL), winFuncId_(0),
    Nin_(0), Nfft_(0) {}

  long getNin() {
    return Nin_;
  }
  cSmileUtilWindowedMagnitudeSpectrum(long Nin, int winFuncId);
  ~cSmileUtilWindowedMagnitudeSpectrum();

  // T is the period of one input sample
  // Returns the frequency width of each FFT magnitude bin (deltaF)
  double getBinDeltaF(double T) {
    if (T == 0.0) {
      return 0.0;
    } else {
      return 1.0 / (T * (double)Nfft_);
    }
  }
  const FLOAT_DMEM * getMagnitudes(const FLOAT_DMEM *in,
      long Nin, bool allowWinSmaller);
  long getNmagnitudes() {
    return Nfft_ / 2 + 1;
  }
};

class cSmileUtilMappedMagnitudeSpectrum {
private:
  long Nmag_;
  long Nout_;  // number of output bins
  cSmileUtilWindowedMagnitudeSpectrum *fft_;
  FLOAT_DMEM * modSpec_;  // the actual modulation spectrum
  double minFreq_;
  double maxFreq_;
  double * splineWork_;
  double * splineDerivs_;
  sSmileMath_splineCache * splineCache;
  sSmileMath_splintCache * splintCache;
  double * magFreq_;
  double deltaF_;
  double T_;

protected:
  void mapMagnitudesToModSpecBins(const FLOAT_DMEM *mag, long N);

public:
  cSmileUtilMappedMagnitudeSpectrum():
    modSpec_(NULL), fft_(NULL), Nmag_(0),
    splineWork_(NULL), splineDerivs_(NULL), splineCache(NULL),
    splintCache(NULL), magFreq_(NULL) {}
  cSmileUtilMappedMagnitudeSpectrum(
      long Nin,       // number of input samples (before zero padding)
      long Nout,      // number of modulation spectrum bins
      int winFuncId,  // window function numeric ID (see smileutil)
      double minFreq, double maxFreq, // min/max frequency of modulation spectrum
      double T        // T is the sample/frame period  (to be able to map FFT bins to frequencies)
      );
  ~cSmileUtilMappedMagnitudeSpectrum();

  void compute(const FLOAT_DMEM *in, long Nin, bool allowWinSmaller);
  long getNin() {
    if (fft_ != NULL)
      return fft_->getNin();
    return 0;
  }
  const FLOAT_DMEM * getModSpec();
  long getModSpecN() {
    return Nout_;
  }  
};


////////////////////////

class cFunctionalModulation : public cFunctionalComponent {
private:
  double stftWinSizeSec_;
  double stftWinStepSec_;
  long stftWinSizeFrames_;
  long stftWinStepFrames_;
  double modSpecMinFreq_;
  double modSpecMaxFreq_;
  int modSpecNumBins_;
  double modSpecResolution_;
  int winFuncId_;
  int removeNonZeroMean_;
  int ignoreLastFrameIfTooShort_;

  FLOAT_DMEM *inNorm_;
  long inNormN_;
  FLOAT_DMEM *avgModSpec_;
  cSmileUtilMappedMagnitudeSpectrum *mappedSpec_;
  char * tmpstr_;

protected:
  SMILECOMPONENT_STATIC_DECL_PR
  virtual void myFetchConfig() override;
  void computeModSpecSTFT(const FLOAT_DMEM *in, long Nin);
  int computeModSpecSTFTavg(const FLOAT_DMEM *in, long Nin, FLOAT_DMEM *ms);

public:
  SMILECOMPONENT_STATIC_DECL

  cFunctionalModulation(const char *_name);
  // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
  virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout) override;
  virtual const char * getValueName(long i) override;
  virtual void setFieldMetaData(cDataWriter *writer, const FrameMetaInfo *fmeta, int idxi, long nEl) override;
  virtual long getNumberOfElements(long j) override;
  virtual long getNoutputValues() override { return nEnab; }
  virtual int getRequireSorted() override { return 0; }

  virtual ~cFunctionalModulation();
};




#endif // __CFUNCTIONALMODULATION_HPP
