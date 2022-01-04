/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functional: Modulation spectra, cepstra, etc.

1. Single modulation frequencies... correlation based? fft on full input based?
2. Overlaping windows -> fft -> average modulation spectrum
3. Method 2 allows for average cepstrum  (Method 1 only for small windows and full input fft -> however scaling problem)
4. Method 2 allows for higher level functionals...


Problems: varying window size... esp. minimum required window size..  zero padding?

*/


#include <functionals/functionalModulation.hpp>

#define MODULE "cFunctionalModulation"

#define N_FUNCTS  1
#define NAMES     "ModulationSpec"

const char *modspecNames[] = {NAMES};

SMILECOMPONENT_STATICS(cFunctionalModulation)

SMILECOMPONENT_REGCOMP(cFunctionalModulation)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFUNCTIONALMODULATION;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALMODULATION;

  /*
     ct->setField("frequencies", "Custom list of modulation frequencies for which to compute the magnitudes. The frequencies will be computed via a zero padded FFT over the full input (modFrqUseFFT>0), or via correlation with sine/cosine functions (slower, but efficient for only very few frequencies).", 0, ARRAY_TYPE);
     ct->setField("modFrqUseFFT", "If > 0, use FFT of full input if computing more than 'modFrqUseFFT' modulation frequencies.", 4);
     ct->setField("modFrqUseACF", "If == 1, use ACF (via FFT) to get better temporal resolution for low modulation frequencies.", 0);
     ct->setField("stftModspec", "1 = Enable average Stft (Short Time Fourier Transform) modulation spectrum (and ignore 'frequencies' array, write full spectrum to output). 2 = Same as 1, however, select frequencies based on 'frequencies' array. 0 = disable.", 0)
     ct->setField("stftCepstrum", "1 = Enable average modulation cepstrum based on Stft.", 0);
     ct->setField("stftAcf", "1 = Enable average ACF based on Stft.", 0);
     ct->setField("nModSpecPeaks", "Number of highest modulation spectrum peaks to output the frequencies for.", 0);
     ct->setField("nCepstralPeaks", "Number N of N highest peaks (in range numCeps..N) in cepstrum to output position for (time in seconds).", 0);
     ct->setField("nAcfPeaks", "Number N of N highest peaks in ACF to output position for (time in seconds). This excludes the peak at time 0.", 0);
     ct->setField("modSpecRange", "Frequency range of Stft modulation spectrum output (leave empty (NULL) to output the full range covered by fftWinSize). Format: start.xx - end.xx (in Hz)", (const char*)NULL);
     ct->setField("acfRange", "ACF range to output. Default (NULL) is output full range", (const char*)NULL);
     ct->setField("acfRange_percentage", "ACF range to output. Default (NULL) is output full range", (const char*)NULL);
     ct->setField("numCeps", "If > 0, output only 1..numCeps cepstral coefficients, otherwise (0) output all cepstral coefficients. Only effective if, stftCepstrum==1, or if using nCepstralPeaks>0.", 0);
     ct->setField("numCeps_percentage", "If > 0.0, output only 1..numCeps*N cepstral coefficients.", 0.0);
     ct->setField("fftWinSize", "Window size for Stft FFT in frames.", 100);
     ct->setField("fftWinSize_sec", "Window size for Stft FFT in seconds.", 1.0);
     ct->setField("fftWinFunc", "Stft FFT window function.", "han");
     ct->setField("shortSegmentMethod", "How to deal with short segments, i.e. segments that are shorter than fftWinSize(_sec). One of: 'zeroPad' zero-pad the input to fftWinSize, 'zeroOutput' output all zero values (no computation), 'noOutput' no output frame, input is discarded.");
     */
  // ct->setField("shortSegmentMethod", "How to deal with short segments, i.e. segments that are shorter than fftWinSize(_sec). One of: 'zeroPad' zero-pad the input to fftWinSize, 'zeroOutput' output all zero values (no computation), 'noOutput' no output frame, input is discarded.");
  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("stftWinSizeSec", "Window size of Short Time Fourier Transformation in seconds. Set to 0 to use (zero-padded to next power of 2) full input segment. If the input is smaller than 'stftWinSizeSec', it will be zero padded to 'stftWinSizeSec'. Segments will further be zero padded to the next higher power of 2 (in frames).", 4.0);
    ct->setField("stftWinStepSec", "Step size of Short Time Fourier Transformation in seconds. Default 0.0 will set the step size to the same as the window size.", 0.0);
    ct->setField("stftWinSizeFrames", "Window size of Short Time Fourier Transformation in input frames. Set to 0 to use (zero-padded to next power of 2) full input segment. If the input is smaller than 'stftWinSizeSec', it will be zero padded to 'stftWinSizeSec'. Segments will further be zero padded to the next higher power of 2. If this option is set, it overrides stftWinSizeSec.", 400);
    ct->setField("stftWinStepFrames", "Window size of Short Time Fourier Transformation in input frames. Default 0 will set the step size to the same as the window size.", 0);
    ct->setField("fftWinFunc", "STFT window function.", "ham");
    ct->setField("modSpecResolution", "Output resolution (in Hz) of modulation spectrum (interpolated from stft). This is preferred over num bins, but if num bins is set, it will override this option.", 0.5);
    ct->setField("modSpecNumBins", "Alternative to specifying the resolution, specifies the number of bins. Overrides 'modSpecResolution', if set.", 50);
    //ct->setField("modSpecRange", "Frequency range of Stft modulation spectrum output (leave empty (NULL) to output the full range covered by fftWinSize). Format: start.xx - end.xx (in Hz)", (const char*)NULL);
    ct->setField("modSpecMinFreq", "Lower bound of modulation spectrum (in Hz).", 0.5);
    ct->setField("modSpecMaxFreq", "Upper bound of modulation spectrum (in Hz).", 20.0);
    ct->setField("showModSpecScale", "(1/0 = yes/no) Print the frequency axis of the modulation spectrum during initialisation.", 0);
    ct->setField("removeNonZeroMean", "(1/0 = yes/no) Remove the mean of all non-zero values (use for F0 modulation spectrum for example).", 0);
    ct->setField("ignoreLastFrameIfTooShort", "(1/0 = yes/no) If stftWinSize is not 0 (i.e. not using full input length), ignore the last window if it is smaller than 2/3 of stftWinSize.", 1);
  )

  SMILECOMPONENT_MAKEINFO_NODMEM(cFunctionalModulation);
}

SMILECOMPONENT_CREATE(cFunctionalModulation)

// TODO: set the output field info (spectral freq. axis!) correctly
//       In order to do this we need to add this functionality to the base class...
//       (This is tricky, so maybe move the modulation spectrum to a separate component?)

/////////////////////////////////////////////////////////
cSmileUtilWindowedMagnitudeSpectrum::cSmileUtilWindowedMagnitudeSpectrum(
    long Nin, int winFuncId):
      fftWork_(NULL), winFunc_(NULL), winFuncId_(winFuncId),
      ip_(NULL), w_(NULL), Nin_(Nin), Nfft_(0)
{
  if (Nin > 0) {
    allocateFFTworkspace(Nin);
    allocateWinFunc(Nin);
  }
}

cSmileUtilWindowedMagnitudeSpectrum::~cSmileUtilWindowedMagnitudeSpectrum()
{
  freeWinFunc();
  freeFFTworkspace();
}

void cSmileUtilWindowedMagnitudeSpectrum::freeFFTworkspace()
{
  if (fftWork_ != NULL) {
    free(fftWork_);
    fftWork_ = NULL;
  }
  if (w_ != NULL) {
    free(w_);
    w_ = NULL;
  }
  if (ip_ != NULL) {
    free(ip_);
    ip_ = NULL;
  }
}

void cSmileUtilWindowedMagnitudeSpectrum::freeWinFunc()
{
  if (winFunc_ != NULL) {
    free(winFunc_);
  }
}

void cSmileUtilWindowedMagnitudeSpectrum::allocateWinFunc(long Nin)
{
  freeWinFunc();
  winFunc_ = (FLOAT_DMEM *)malloc(sizeof(FLOAT_DMEM) * Nin);
  double * winFuncD = NULL;
  switch(winFuncId_) {
    case WINF_RECTANGLE: winFuncD = smileDsp_winRec(Nin); break;
    case WINF_HANNING:   winFuncD = smileDsp_winHan(Nin); break;
    case WINF_HAMMING:   winFuncD = smileDsp_winHam(Nin); break;
    case WINF_TRIANGLE:  winFuncD = smileDsp_winTri(Nin); break;
    case WINF_BARTLETT:  winFuncD = smileDsp_winBar(Nin); break;
    case WINF_SINE:      winFuncD = smileDsp_winSin(Nin); break;
    case WINF_LANCZOS:   winFuncD = smileDsp_winLac(Nin); break;
    default:
      SMILE_ERR(1, "unknown window function ID (%i). Fallback to rectangular window!",
          winFuncId_);
  }
  if (winFuncD != NULL) {
    for (int i = 0; i < Nin; i++) {
      winFunc_[i] = (FLOAT_DMEM)winFuncD[i];
    }
    free(winFuncD);
  } else {
    for (int i = 0; i < Nin; i++) {
      winFunc_[i] = 1.0;
    }
  }
}

void cSmileUtilWindowedMagnitudeSpectrum::allocateFFTworkspace(long Nin)
{
  // compute N
  long N = Nin;
  if (!smileMath_isPowerOf2(Nin)) {
    N = smileMath_ceilToNextPowOf2(Nin);
  }
  //fprintf(stderr, "XXX Allocate fft: Nin %ld Npow2 %ld\n", Nin, N);
  if (N < 4) N = 4;
  // free old work areas
  freeFFTworkspace();
  // allocate work areas
  Nfft_ = N;
  Nin_ = Nin;
  ip_ = (int *)calloc(1, sizeof(int) * (N+2));
  w_ = (FLOAT_TYPE_FFT *)calloc(1, sizeof(FLOAT_TYPE_FFT) * ((N*5)/4+2));
  fftWork_ = (FLOAT_TYPE_FFT*)malloc(sizeof(FLOAT_TYPE_FFT) * N);
}

// copies data to work area,
// applies window function (if any)
// and zero-pads
void cSmileUtilWindowedMagnitudeSpectrum::copyInputAndZeropad(
    const FLOAT_DMEM *in, long Nin, bool allowWinSmaller = false)
{
  // some checks
  //fprintf(stderr, "Nfft: %ld, Nin: %ld (Nin_ %ld)\n", Nfft_, Nin, Nin_);
  // This needs to happen before allocateFFTworkspace,
  // because otherwise Nin_ will be overwritten with Nin
  if (Nin != Nin_) {
    allocateWinFunc(Nin);
    Nin_ = Nin;
  }
  if (Nin > Nfft_ || (Nin <= Nfft_ / 2 && allowWinSmaller)) {
    // re-allocate fft buffers ...
    allocateFFTworkspace(Nin);
  }
  // copy to work area
  if (winFunc_ != NULL) {
    for (long i = 0; i < Nin; i++) {
      fftWork_[i] = in[i] * winFunc_[i];
    }
  } else {
    for (long i = 0; i < Nin; i++) {
      fftWork_[i] = in[i];
    }
    //memcpy(fftWork_, in, sizeof(FLOAT_DMEM) * Nin);
  }
  // zero pad
  if (Nfft_ > Nin) {
    bzero(fftWork_ + Nin, sizeof(FLOAT_TYPE_FFT) * (Nfft_ - Nin));
  }
}

void cSmileUtilWindowedMagnitudeSpectrum::doFFT()
{
  rdft((int)Nfft_, 1, fftWork_, ip_, w_);
}

void cSmileUtilWindowedMagnitudeSpectrum::computeMagnitudes()
{
  fftWork_[0] = fabs(fftWork_[0]);
  FLOAT_DMEM nyq = fabs(fftWork_[1]);
  for (long n = 2; n < Nfft_; n += 2) {
    fftWork_[n/2] = sqrt(fftWork_[n]*fftWork_[n] + fftWork_[n+1]*fftWork_[n+1]);
  }
  fftWork_[Nfft_ / 2] = nyq;
}

const FLOAT_DMEM * cSmileUtilWindowedMagnitudeSpectrum::getMagnitudes(
    const FLOAT_DMEM *in, long Nin, bool allowWinSmaller = false)
{
  copyInputAndZeropad(in, Nin, allowWinSmaller);
  doFFT();
  computeMagnitudes();
// TODO: convert fftWork to right pointer type upon return!!!!
// This is a temp hack to make the compiler happy, but it breaks the compontnt!!
  return (FLOAT_DMEM*)fftWork_;
}

/////////////////////////////////////////////////////////
cSmileUtilMappedMagnitudeSpectrum::cSmileUtilMappedMagnitudeSpectrum(long Nin,
    long Nout, int winFuncId, double minFreq, double maxFreq, double T):
  modSpec_(NULL), Nout_(Nout), minFreq_(minFreq), maxFreq_(maxFreq),
  Nmag_(0), splineWork_(NULL), splineDerivs_(NULL), splineCache(NULL),
  splintCache(NULL), magFreq_(NULL), T_(T)
{
  fft_ = new cSmileUtilWindowedMagnitudeSpectrum(Nin, winFuncId);
  modSpec_ = (FLOAT_DMEM*)calloc(1, sizeof(FLOAT_DMEM) * Nout);
}

cSmileUtilMappedMagnitudeSpectrum::~cSmileUtilMappedMagnitudeSpectrum()
{
  if (modSpec_ != NULL) {
    free(modSpec_);
  }
  if (fft_ != NULL) {
    delete fft_;
  }
  if (splineDerivs_ != NULL) {
    free(splineDerivs_);
  }
  if (splineWork_ != NULL) {
    free(splineWork_);
  }
  if (splineCache != NULL) {
    smileMath_cspline_free(splineCache);
    free(splineCache);
  }
  if (splintCache != NULL) {
    smileMath_csplint_free(splintCache);
    free(splintCache);
  }
  if (magFreq_ != NULL) {
    free(magFreq_);
  }
}

void cSmileUtilMappedMagnitudeSpectrum::mapMagnitudesToModSpecBins(
    const FLOAT_DMEM *mag, long N)
{
  //fprintf(stderr, "XXX: N %ld, Nmag_ %ld\n", N, Nmag_);
  if (Nmag_ != N) {
    Nmag_ = N;
    // if N has changed, recreate the splines
    if (splineDerivs_ != NULL) {
      free(splineDerivs_);
    }
    splineDerivs_ = (double *)malloc(sizeof(double)*Nmag_);
    if (splineWork_ != NULL) {
      free(splineWork_);
      splineWork_ = NULL;
    }
    if (magFreq_ != NULL) {
      free(magFreq_);
    }
    double deltaFmag_ = fft_->getBinDeltaF(T_);
    magFreq_ = (double *)malloc(sizeof(double)*Nmag_);
    for (long i = 0; i < Nmag_; i++) {
      magFreq_[i] = (double)i * deltaFmag_;
    }
    deltaF_ = (maxFreq_ - minFreq_) / (double)Nout_;

    // pre-compute spline using smileMath_cspline_init
    if (splineCache != NULL) {
      smileMath_cspline_free(splineCache);
      free(splineCache);
    }
    splineCache = (sSmileMath_splineCache*)malloc(sizeof(sSmileMath_splineCache));
    smileMath_cspline_init(magFreq_, Nmag_, splineCache);

    // pre-compute spline using smileMath_csplint_init
    double *x = (double*)malloc(sizeof(double)*Nout_);  
    for (long i = 0; i < Nout_; i++) {
      x[i] = minFreq_ + (double)i * deltaF_;
    }    
    if (splintCache != NULL) {
      smileMath_csplint_free(splintCache);
      free(splintCache);
    }
    splintCache = (sSmileMath_splintCache*)malloc(sizeof(sSmileMath_splintCache));
    if (!smileMath_csplint_init(magFreq_, Nmag_, x, Nout_, splintCache)) {
      SMILE_ERR(1,"smileMath_csplint_init failed. Output of this component will be invalid!");
    }
    free(x);
  }

  // apply the spline interpolation to get the mapped spectrum
#if FLOAT_DMEM_NUM == FLOAT_DMEM_DOUBLE
  if (smileMath_cspline(magFreq_, mag, Nmag_, 1e30, 1e30, splineDerivs_, &splineWork_, splineCache)) {
    smileMath_csplint(mag, splineDerivs_, splintCache, modSpec_);
  }
#else
  // we need to convert between FLOAT_DMEM and double
  double *tmpMag = (double*)malloc(sizeof(double) * Nmag_);
  for (int i = 0; i < Nmag_; i++) {
    tmpMag[i] = (double)mag[i];
  }
  if (smileMath_cspline(magFreq_, tmpMag, Nmag_, 1e30, 1e30, splineDerivs_, &splineWork_, splineCache)) {
    double *tmpModSpec = (double*)malloc(sizeof(double) * Nout_);
    smileMath_csplint(tmpMag, splineDerivs_, splintCache, tmpModSpec);
    for (int i = 0; i < Nout_; i++) {
      modSpec_[i] = (FLOAT_DMEM)tmpModSpec[i];
    }
    free(tmpMag);
    free(tmpModSpec);
  }
#endif
}

// allowWinSmaller = false : If window gets smaller than N/2, everything will be re-allocated and an FFT of N/2 (or smaller) will be performed
// .. = true: The smaller input window will only be zero-padded and the full fft is performed
void cSmileUtilMappedMagnitudeSpectrum::compute(
    const FLOAT_DMEM *in, long Nin, bool allowWinSmaller = false)
{
  const FLOAT_DMEM *mags = fft_->getMagnitudes(in, Nin, allowWinSmaller);
  mapMagnitudesToModSpecBins(mags, fft_->getNmagnitudes());
}

const FLOAT_DMEM * cSmileUtilMappedMagnitudeSpectrum::getModSpec()
{
  return modSpec_;
}

///////////////////////////////////////////////
//-----
cFunctionalModulation::cFunctionalModulation(const char *name) :
  cFunctionalComponent(name, N_FUNCTS, modspecNames),
  tmpstr_(NULL), mappedSpec_(NULL), avgModSpec_(NULL),
  stftWinSizeFrames_(0), stftWinStepFrames_(0),
  inNorm_(NULL), inNormN_(0)
{
}

void cFunctionalModulation::myFetchConfig()
{
  ignoreLastFrameIfTooShort_ = getInt("ignoreLastFrameIfTooShort");
  removeNonZeroMean_ = getInt("removeNonZeroMean");

  stftWinSizeSec_ = getDouble("stftWinSizeSec");
  stftWinStepSec_ = getDouble("stftWinStepSec");
  if (stftWinStepSec_ == 0.0) {
    stftWinStepSec_ = stftWinSizeSec_;
  }
  if (isSet("stftWinSizeFrames")) {
    stftWinSizeFrames_ = getInt("stftWinSizeFrames");
    stftWinSizeSec_ = 0.0;
  }
  if (isSet("stftWinStepFrames")) {
    stftWinStepFrames_ = getInt("stftWinStepFrames");
    stftWinStepSec_ = 0.0;
  }
  if (stftWinStepFrames_ == 0) {
    stftWinStepFrames_ = stftWinSizeFrames_;
  }

  modSpecMinFreq_ = getDouble("modSpecMinFreq");
  modSpecMaxFreq_ = getDouble("modSpecMaxFreq");
  if (isSet("modSpecNumBins")) {
    modSpecNumBins_ = getInt("modSpecNumBins");
    modSpecResolution_ = (modSpecMaxFreq_ - modSpecMinFreq_) / (double)(modSpecNumBins_ - 1);
  } else {
    modSpecResolution_ = getDouble("modSpecResolution");
    modSpecNumBins_ = (int)round((modSpecMaxFreq_ - modSpecMinFreq_) / modSpecResolution_) + 1;
  }
  SMILE_IMSG(2, "modSpecNumBins_ = %i", modSpecNumBins_);
  if (getInt("showModSpecScale")) {
    for (int i = 0; i < modSpecNumBins_; i++) {
      SMILE_IMSG(1, "modulation spectrum bin %i : %f Hz", i, modSpecMinFreq_ + (double)i * modSpecResolution_);
    }
  }
  const char * wf = getStr("fftWinFunc");
  if (wf != NULL) {
    winFuncId_ = winFuncToInt(wf);
  }

  enab[0] = 1;
  cFunctionalComponent::myFetchConfig();
  nEnab += modSpecNumBins_ - 1;   // -1 because enab[0] = 1 will increase nEnab by 1
  //nEnab = 0;
}

long cFunctionalModulation::getNumberOfElements(long j)
{
  if (j == 0)
    return modSpecNumBins_;
  else
    return 0;
}

// TODO: option for output of modspec frequencies!
const char* cFunctionalModulation::getValueName(long i)
{
  const char *n = cFunctionalComponent::getValueName(0);
  // append coefficient number
  if (tmpstr_ != NULL) free(tmpstr_);
  tmpstr_ = myvprint("%s%i", n, i);
  return tmpstr_;
}

// is called by cFunctionals::setupNamesForElement after each new field has been added
void cFunctionalModulation::setFieldMetaData(cDataWriter *writer,
    const FrameMetaInfo *fmeta, int idxi, long nEl) {
  long infosize = nEl * sizeof(double);
  if (fmeta->field[idxi].infoSize > 0 && fmeta->field[idxi].infoSize != infosize) {
    SMILE_IWRN(3, "nEl (* sizeof(double)) in call to setFieldMetaData != infoSize from previous level. Check your setup and the code! (infoSize = %i, nEl * sizeof(double) = %i). Using nEl here...",
        fmeta->field[idxi].infoSize, infosize);
  }

  double * buf = (double *)malloc(infosize);
  int i;
  for (i = 0; i < nEl; i++) {
    buf[i] = modSpecResolution_ * (double)i + modSpecMinFreq_;
  }
  writer->setFieldInfo(-1, // refers to last field added
      DATATYPE_SPECTRUM_BINS_MAG, buf,
      infosize);
}

int cFunctionalModulation::computeModSpecSTFTavg(const FLOAT_DMEM *in, long Nin, FLOAT_DMEM *ms)
{
  int nSpec = 0;
  bzero(ms, sizeof(FLOAT_DMEM) * mappedSpec_->getModSpecN());
  for (long n = 0; n < Nin; n += stftWinStepFrames_) {
    long N = MIN(stftWinSizeFrames_, Nin - n - 1);
    //fprintf(stderr, "XXX candidate n %ld, Nin %ld, winSize %ld, chosen N %ld\n", n, Nin, stftWinSizeFrames_, N);
    if (N > 2 * stftWinSizeFrames_ / 3 || nSpec == 0) {
      //fprintf(stderr, "XXX do compte\n");
      // always pad first frame, don't pad/process the following frames if shorter than 2/3 winSize
      mappedSpec_->compute(in + n, N, true);
      N = mappedSpec_->getModSpecN();
      const FLOAT_DMEM * spec = mappedSpec_->getModSpec();
      // do the averaging...
      for (int i = 0; i < N; i++) {
        ms[i] += spec[i];
      }
      nSpec++;
    }
  }
  long N = mappedSpec_->getModSpecN();
  if (nSpec > 0) {
    for (int i = 0; i < N; i++) {
      ms[i] /= (FLOAT_DMEM)nSpec;
    }
  }
  return N;
}

long cFunctionalModulation::process(FLOAT_DMEM *in,
    FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout)
{
  if (mappedSpec_ == NULL ||
      (stftWinSizeFrames_ == 0 && mappedSpec_->getNin() != Nin)) {
    // we need to init
    FLOAT_DMEM T = (FLOAT_DMEM)getInputPeriod();
    if (T == 0.0) {
      SMILE_IERR(1, "Cannot compute modulation spectrum when input period is unknown (asynchronous input level!). T = 0.0");
      return 0;
    }
    if (stftWinSizeFrames_ == 0 && T > 0) {
      stftWinSizeFrames_ = stftWinSizeSec_ / T;
      stftWinStepFrames_ = stftWinStepSec_ / T;
    }
    long N = Nin;
    if (stftWinSizeFrames_ > 0) {
      N = stftWinSizeFrames_;
      //fprintf(stderr, "XXX stftWinSize: %ld\n", stftWinSizeFrames_);
    }
    //fprintf(stderr, "XXX allocate with N = %ld\n", N);
    mappedSpec_ = new cSmileUtilMappedMagnitudeSpectrum(N, modSpecNumBins_,
        winFuncId_, modSpecMinFreq_, modSpecMaxFreq_, T);
  }
  if (avgModSpec_ == NULL) {
    avgModSpec_ = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM) * modSpecNumBins_);
  }
  if (removeNonZeroMean_) {
    if (inNormN_ != Nin) {
      if (inNorm_ != NULL) {
        free(inNorm_);
      }
      inNorm_ = NULL;
    }
    if (inNorm_ == NULL) {
      inNormN_ = Nin;
      inNorm_ = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM) * Nin);
    }
    FLOAT_DMEM mean = 0.0;
    long nMean = 0;
    for (long i = 0; i < Nin; i++) {
      if (in[i] != 0.0) {
        mean += in[i];
        nMean++;
      }
    }
    if (nMean > 0) {
      mean /= (FLOAT_DMEM)nMean;
    }
    for (long i = 0; i < Nin; i++) {
      if (in[i] != 0.0) {
        inNorm_[i] = in[i] - mean;
      } else {
        inNorm_[i] = 0.0;
      }
    }
    in = inNorm_;
  }
  if (stftWinSizeFrames_ == 0) {
    mappedSpec_->compute(in, Nin, false);
    const FLOAT_DMEM * ms = mappedSpec_->getModSpec();
    memcpy(avgModSpec_, ms, sizeof(FLOAT_DMEM) * modSpecNumBins_);
  } else {
    computeModSpecSTFTavg(in, Nin, avgModSpec_);
  }
  // do some transformations, like power, log

  // here would be also the place to apply ACF/CEPSTRUM/DCT etc.

  // copy the resulting modspec to the output vector
  //printf("XXX out[1] = %f\n", avgModSpec_[1]);
  for (int i = 0; i < modSpecNumBins_; i++) {
    out[i] = avgModSpec_[i];
  }
  return Nout;
}

// FURTHER TODO: functionalsVecToVec..

cFunctionalModulation::~cFunctionalModulation()
{
  if (tmpstr_ != NULL) {
    free(tmpstr_);
  }
  if (mappedSpec_ != NULL) {
    delete mappedSpec_;
  }
  if (avgModSpec_ != NULL) {
    free(avgModSpec_);
  }
  if (inNorm_ != NULL) {
    free(inNorm_);
  }
}


// TODO: modspec phases might be of interest... or rather the relative phase deviations between mod freqs.

// TODO: 2D modspec component (not functional.. MVR like)

/*
 This will take a log spectrogram, downsampled to 50 freq. bins?
 Time resolution downsampled to 20fps
  Then 2D spec/cep ?  (looks more at the LF components)

 Or fine grained struct of non-downsampled as extra features? (HF components)
*/

