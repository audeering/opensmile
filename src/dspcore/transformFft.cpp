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


#include <dspcore/transformFft.hpp>

#define MODULE "cTransformFFT"

SMILECOMPONENT_STATICS(cTransformFFT)

SMILECOMPONENT_REGCOMP(cTransformFFT)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CTRANSFORMFFT;
  sdescription = COMPONENT_DESCRIPTION_CTRANSFORMFFT;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("inverse", "1 = perform inverse real FFT", 0);
    ct->setField("zeroPadSymmetric", "1 = zero pad symmetric (when zero padding to next power of 2), i.e. center frame and pad left and right with zeros. New since version 2.3: this is the default, but should not affect FFT magnitudes at all, only phase.", 1);
  )
  SMILECOMPONENT_MAKEINFO(cTransformFFT);
}

SMILECOMPONENT_CREATE(cTransformFFT)

//-----

cTransformFFT::cTransformFFT(const char *_name) :
  cVectorProcessor(_name),
  ip_(NULL),
  w_(NULL),
  xconv_(NULL),
  newFsSet_(0),
  frameSizeSecOut_(0.0)
{ }

void cTransformFFT::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();
  inverse_ = getInt("inverse");
  if (inverse_) {
    SMILE_IDBG(2, "transformFFT set for inverse FFT.");
    inverse_ = -1;  // sign of exponent
  } else {
    inverse_ = 1; // sign of exponent
  }
  zeroPadSymmetric_ = getInt("zeroPadSymmetric");
}

int cTransformFFT::configureWriter(sDmLevelConfig &c)
{
  // determine new frameSizeSec resulting from rounding up to closest power of 2
  for (int i = 0; i < c.Nf; i++) {
    long nEl = c.fmeta->field[i].N;
    /* for each field we must ensure the power of 2 constraint and adjust the frameSize if necessary*/
    if (!smileMath_isPowerOf2(nEl)) {
      if (inverse_==-1) {
        SMILE_IERR(1,"cannot perform zero-padding for inverse real FFT (this would mean zero padding frequencies in the complex domain...)! A framesize which is power of 2 is required here! (current framesize = %i)",nEl);
        COMP_ERR("aborting");
        
      } else {
        long nElOld = nEl;
        nEl = smileMath_ceilToNextPowOf2(nEl);  // TODO:: change frameSizeSec in write Level!
        if (!newFsSet_) {
          // compute new frame size in seconds:
          c.lastFrameSizeSec = c.frameSizeSec; // save last frame size
          c.frameSizeSec *= (double)nEl / (double)nElOld;
          newFsSet_=1;
        }
      }
    }
    if (inverse_==-1) {
       //TODO: detect frames which were originally zero-padded, and output truncated frames

    }
    if (newFsSet_) break;
  }
  frameSizeSecOut_ = c.frameSizeSec;
  return 1;
}

// generate "frequency axis information", i.e. the frequency in Hz for each spectral bin
// which is to be saved as meta-data in the dataMemory level field (FrameMetaInfo->FieldMetaInfo->info)
// &infosize is initialized with the number of fft bins x 2 (= number of input samples)
//   and should contain the number of complex bins at the end of this function
void * cTransformFFT::generateSpectralVectorInfo(long &infosize)
{
  int i;
  infosize /= 2;
  infosize++; // nyquist and DC...
  double *inf = (double*)calloc(1,sizeof(double)*infosize);
  
  double F0;
  if (frameSizeSecOut_ > 0.0) {
    F0 = (double)(1.0) / (double)frameSizeSecOut_;
    for (i=0; i<infosize; i++) {
      inf[i] = F0*(double)i;
    }
  }
  return (void *)inf;
}

int cTransformFFT::setupNamesForField(int i, const char*name, long nEl)
{
  // round of nEl to closest power of 2, setup output fields (and thus, Ndst passed to processVectorXXX, etc.)

  /* for each field we must ensure the power of 2 constraint and adjust the frameSize if necessary*/
  if (!smileMath_isPowerOf2(nEl)) {
    if (inverse_==-1) { COMP_ERR("error with input framesize, not a power of 2!"); }
    long nElOld = nEl;
    nEl = smileMath_ceilToNextPowOf2(nEl);  
  }
  if (nEl < 4) nEl = 4;

  //TODO? : append name "fftc" -> set the nameAppend field : ct->setField("nameAppend",(const char*) NULL,"fftc");
  int ret = cVectorProcessor::setupNamesForField(i,name,nEl);
  long tmp = nEl;
  void *fmap = generateSpectralVectorInfo(tmp);
  writer_->setFieldInfo(-1,DATATYPE_SPECTRUM_BINS_COMPLEX,fmap,tmp*sizeof(double));
  return ret;
}

int cTransformFFT::myFinaliseInstance()
{
  int ret = cVectorProcessor::myFinaliseInstance();
  
  if (ret) {
    //?? to support re-configure once it is implemented in component manager ??
    if (ip_ != NULL) {
      multiConfFree(ip_);
      ip_=NULL;
    }
    if (w_ != NULL) {
      multiConfFree(w_);
      w_ = NULL;
    }
    if (xconv_ != NULL) {
      multiConfFree(xconv_);
      xconv_ = NULL;
    }
    ip_ = (int**)multiConfAlloc(); 
    w_ = (FLOAT_TYPE_FFT**)multiConfAlloc();
    xconv_ = (FLOAT_TYPE_FFT**)multiConfAlloc();
  }
  return ret;
}

// a derived class should override this method, in order to implement the actual processing
int cTransformFFT::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  idxi = getFconf(idxi);
  FLOAT_TYPE_FFT *x = xconv_[idxi];
  FLOAT_TYPE_FFT *w_l = w_[idxi];
  int *ip_l = ip_[idxi];
  if (x == NULL) {
    x = (FLOAT_TYPE_FFT *)malloc(sizeof(FLOAT_TYPE_FFT) * Ndst);
    xconv_[idxi] = x;
  }
  if (inverse_ == 1) {
    // this is the forward transform (inverse is the exponent factor..)
    if (zeroPadSymmetric_) {
      int padlen2 = (Ndst - Nsrc) / 2;
      for (int i = 0; i < padlen2; i++) {  // zeropadding first half
        x[i] = 0;
      }
      for (int i = 0; i < Nsrc; i++) {
        x[i + padlen2] = (FLOAT_TYPE_FFT)src[i];
      }
      for (int i = Nsrc + padlen2; i < Ndst; i++) {  // zeropadding second half
        x[i] = 0;
      }
    } else {
      for (int i = 0; i < Nsrc; i++) {
        x[i] = (FLOAT_TYPE_FFT)src[i];
      }
      for (int i = Nsrc; i < Ndst; i++) {  // zeropadding second half
        x[i] = 0;
      }
    }
  } else {
    for (int i = 0; i < Nsrc; i++) {
      x[i] = (FLOAT_TYPE_FFT)src[i];
    }
  }
  if (ip_l == NULL) {
    ip_l = (int *)calloc(1, sizeof(int) * (3 + (size_t)ceil(sqrt((float)Ndst))));
    ip_[idxi] = ip_l;
  }
  if (w_l == NULL) {
    w_l = (FLOAT_TYPE_FFT *)calloc(1, sizeof(FLOAT_TYPE_FFT) * (Ndst / 2 + 1));
    w_[idxi] = w_l;
  }
    //w_l = (FLOAT_TYPE_FFT *)calloc(1,sizeof(FLOAT_TYPE_FFT)*((Ndst*5)/4+2));
  // perform real FFT
  rdft((int)Ndst, inverse_, x, ip_l, w_l);
  if (inverse_==-1) {
    FLOAT_DMEM norm = (FLOAT_DMEM)2.0 / (FLOAT_DMEM)Ndst;
    for (int i = 0; i < Ndst; i++) {
      dst[i] = ((FLOAT_DMEM)x[i])*norm;
    }
  } else {
    for (int i = 0; i < Ndst; i++) {
      dst[i] = (FLOAT_DMEM)x[i];
    }
  }
  return 1;
}

cTransformFFT::~cTransformFFT()
{
  if (ip_ != NULL)
    multiConfFree(ip_);
  if (w_ != NULL)
    multiConfFree(w_);
  if (xconv_ != NULL)
    multiConfFree(xconv_);
}
