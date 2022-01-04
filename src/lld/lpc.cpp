/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

LPC, compute LPC coefficients from wave data (PCM) frames 

*/


#include <lld/lpc.hpp>
#include <math.h>

#define MODULE "cLpc"

SMILECOMPONENT_STATICS(cLpc)

SMILECOMPONENT_REGCOMP(cLpc)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CLPC;
  sdescription = COMPONENT_DESCRIPTION_CLPC;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("method","This option sets the lpc method to use. Choose between: 'acf' acf (autocorrelation) method with Levinson-Durbin algorithm , 'burg' Burg method (N. Anderson (1978)) ","acf");  
    ct->setField("p","Predictor order (= number of lpc coefficients)",8);
    ct->setField("saveLPCoeff","1 = save LP coefficients to output",1);
    ct->setField("lpGain","1 = save lpc gain (error) in output vector",0);
    ct->setField("saveRefCoeff","1 = save reflection coefficients to output",0);
    ct->setField("residual","1 = compute lpc residual signal and store in output frame",0);
    ct->setField("residualGainScale","1 = scale lpc residual signal by lpc gain (divides each frame by gain)",0);
    ct->setField("forwardFilter","1 = apply forward instead of inverse filter when computing residual",0);
    ct->setField("lpSpectrum","1 = compute lp spectrum using 'lpSpecDeltaF' as frequency resolution or 'lpSpecBins' bins",0);
    ct->setField("forwardLPspec","1 = compute forward filter transfer function as LP spectrum, instead of old default behaviour of computing spectrum of inverse filter.", 1);
    ct->setField("forwardLPspecFloor","Floor value to add to all spectral bins of inverse LP spectrum when inverting the spectrum (1/x) to avoid division by 0. Default: 10^-13.", 0.0000000000001);
    ct->setField("lpSpecDeltaF","frequency resolution of lp spectrum (only applicable if 'lpSpectrum=1')",10.0);
    ct->setField("lpSpecBins","number of bins to compute lp spectrum for (overrides lpSpecDeltaF) (only applicable if 'lpSpectrum=1')",100,0,0);
  )

  SMILECOMPONENT_MAKEINFO(cLpc);
}

SMILECOMPONENT_CREATE(cLpc)

//-----

cLpc::cLpc(const char *_name) :
  cVectorProcessor(_name),
  p(0),
  saveRefCoeff(0), acf(NULL),
  lpCoeff(NULL), lastLpCoeff(NULL), refCoeff(NULL),
  gbb(NULL), gb2(NULL), gaa(NULL),
  lSpec(NULL), _ip(NULL), _w(NULL), latB(NULL)
{

}

void cLpc::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();

  const char *met = getStr("method");
  method = 0;
  if (met != NULL) {
    if (!strncasecmp(met,"acf",3)) {
      method = LPC_METHOD_ACF;
    } else if (!strncasecmp(met,"burg",4)) {
      method = LPC_METHOD_BURG; 
    }
  }

  p=getInt("p");
  if (p<1) p=1;
  SMILE_IDBG(2,"predictor order p = %i",p); 

  saveLPCoeff=getInt("saveLPCoeff");
  SMILE_IDBG(2,"saveLPCoeff = %i",saveLPCoeff); 

  lpGain=getInt("lpGain");
  SMILE_IDBG(2,"lpGain = %i",lpGain); 

  saveRefCoeff=getInt("saveRefCoeff");
  SMILE_IDBG(2,"saveRefCoeff = %i",saveRefCoeff); 

  residual=getInt("residual");
  residualGainScale=getInt("residualGainScale");
  SMILE_IDBG(2,"residual = %i",residual); 

  forwardRes=getInt("forwardFilter");
  SMILE_IDBG(2,"forwardRes = %i",forwardRes); 

  lpSpectrum = getInt("lpSpectrum");
  SMILE_IDBG(2,"lpSpectrum = %i",lpSpectrum); 

  forwardLPspec = getInt("forwardLPspec");
  forwardLPspecFloor = (FLOAT_DMEM)getDouble("forwardLPspecFloor");

  lpSpecDeltaF = getDouble("lpSpecDeltaF");
  SMILE_IDBG(2,"lpSpecDeltaF = %f",lpSpecDeltaF); 
  lpSpecBins = getInt("lpSpecBins");
  SMILE_IDBG(2,"lpSpecBins = %i",lpSpecBins); 

  latB = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*p);
  lpCoeff = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*(p+1));
  lastLpCoeff = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*(p));
  refCoeff = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*p);
  lastGain = 0.0;
}

// setup size of output vector (= predictor order p)
int cLpc::setupNamesForField(int i, const char*name, long nEl)
{
  int n=0;

  nInput_ = nEl;
  inputStart_ = 0;

  if (saveLPCoeff) {
    writer_->addField( "lpcCoeff", p ); n += p;
  }

  if (saveRefCoeff) {
    writer_->addField( "reflectionCoeff", p ); n += p;
  }

  if (lpGain) {
    writer_->addField( "lpGain", 1 ); n += 1;
  }

  if (lpSpectrum) {
    writer_->addField( "lpSpectrum", lpSpecBins ); n += lpSpecBins;
    //TODO: set the spectral info struct of the lp spec and debug output the bin F0
  }

  if (residual) {
    writer_->addField( "lpcResidual", nEl ); n += nEl;
  }

  return n;

  // TODO: add reflection coeffs..
 // return cDataProcessor::setupNamesForField(i,name,p);
}



// return value: gain
FLOAT_DMEM cLpc::calcLpc(const FLOAT_DMEM *x, long Nsrc, FLOAT_DMEM * lpc, long nCoeff, FLOAT_DMEM *refl)
{
  FLOAT_DMEM gain = 0.0;
  if (method == LPC_METHOD_ACF) {
    if (acf == NULL) acf = (FLOAT_DMEM *)malloc(sizeof(FLOAT_DMEM)*(nCoeff+1));
    smileDsp_autoCorr(x, Nsrc, acf, nCoeff+1);
    smileDsp_calcLpcAcf(acf, lpc, nCoeff, &gain, refl);
  } 
  else if (method == LPC_METHOD_BURG) {
    smileDsp_calcLpcBurg(x, Nsrc, lpc, nCoeff, &gain, &gbb, &gb2, &gaa);
    if (refl != NULL) SMILE_IWRN(1,"computation of reflection coefficients with Burg's LPC method is not yet implemented!");
  }
  return gain;
}

int cLpc::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  long myN = Ndst;
  if (residual) myN = Ndst - Nsrc;

  long expectedN = 0;
  if (saveRefCoeff) expectedN += p;
  if (saveLPCoeff) expectedN += p;
  if (lpGain) expectedN += 1;
  if (lpSpectrum) expectedN += lpSpecBins;
  if (myN != expectedN) {
    SMILE_IWRN(1,"Ndst(-Nsrc) (=%i) <> expected value (%i) ! something is wrong.. the program might crash!",myN,expectedN);
  }

  if (p<0) {
    SMILE_IWRN(1,"p<0! something is wrong...");
    p=0;
  }

  long i;
  FLOAT_DMEM *dst0 = dst;
  FLOAT_DMEM gain = 0.0;

  if (saveRefCoeff) {
    //calcLpc(const FLOAT_DMEM *x, long Nsrc, FLOAT_DMEM * lpc, long nCoeff, FLOAT_DMEM *refl) 
    gain = calcLpc(src, Nsrc, lpCoeff, p, refCoeff);
    if (saveLPCoeff) {
      for (i=0; i<p; i++) {
        dst[i] = lpCoeff[i];
        dst[i+p] = refCoeff[i];
      }
      dst += 2*p;
    } else {
      for (i=0; i<p; i++) {
        dst[i] = refCoeff[i];
      }
      dst += p;
    }    
  } else {
    if (saveLPCoeff || residual || lpSpectrum || lpGain) {
      gain = calcLpc(src, Nsrc, lpCoeff, p, refCoeff);
    }
    if (saveLPCoeff) {
      for (i=0; i<p; i++) {
        dst[i] = lpCoeff[i];
      }
      dst += p;
    }
  }

  if (lpGain) {
    dst[0] = gain;
    dst++;
  }

  if (lpSpectrum) {
    /*
      we compute the lp spectrum by zero padding and fft of the lp coefficients
      the number of 0's we pad with determines our frequency resolution
    */
     // config parameters: lpSpecDeltaF & lpSpecBins (N or -1 for = nLpc)
    //double fftN = (1.0/T) / lpSpecDeltaF;
    if (lSpec == NULL) lSpec = (FLOAT_TYPE_FFT*)malloc(sizeof(FLOAT_TYPE_FFT) * lpSpecBins * 2);

    // create padded vector
    for (i=0; i<lpSpecBins*2; i++) {
      lSpec[i] = 0.0;
    }
    lSpec[0] = 1.0;
    for (i=1; i<=p; i++) {
      lSpec[i] = (FLOAT_TYPE_FFT)lpCoeff[i-1];
    }

    // transform
    if (_ip==NULL) _ip = (int *)calloc(1,sizeof(int)*(lpSpecBins*2+2));
    if (_w==NULL) _w = (FLOAT_TYPE_FFT *)calloc(1,sizeof(FLOAT_TYPE_FFT)*(lpSpecBins*2*5)/4+2);
    //perform FFT
    rdft(lpSpecBins*2, 1, lSpec, _ip, _w);

    // compute magnitude
    int n=0;
    if (forwardLPspec) {
      // in order to obtain the forward filter, we need to invert the amplitudes
      *(dst++) = (FLOAT_DMEM)fabs((FLOAT_DMEM)1.0 / (lSpec[0] + forwardLPspecFloor)); /* DC part */
      for (i=2; i<(lpSpecBins-1)*2; i += 2) {
        // save in output vector
        *(dst++) = (FLOAT_DMEM)1.0 / ((FLOAT_DMEM)sqrt(lSpec[i]*lSpec[i] + lSpec[i+1]*lSpec[i+1]) + forwardLPspecFloor);
      }
      *(dst++) = (FLOAT_DMEM)fabs((FLOAT_DMEM)1.0 / (lSpec[1] + forwardLPspecFloor));; /* Nyquist freq. */
    } else {
      *(dst++) = (FLOAT_DMEM)fabs( lSpec[0] ); /* DC part */
      for (i=2; i<(lpSpecBins-1)*2; i += 2) {
        // save in output vector
        *(dst++) = (FLOAT_DMEM)sqrt( lSpec[i]*lSpec[i] + lSpec[i+1]*lSpec[i+1] );
      }
      *(dst++) = (FLOAT_DMEM)fabs( lSpec[1] ); /* Nyquist freq. */
    }
  }

  if (residual) {
    if (forwardRes) { // apply forward LPC filter (recursive)
      for (i=0; i<Nsrc; i++) {
       dst[i] = smileDsp_invLattice(refCoeff, latB, p, src[i]);
      }
    } else { // apply inverse LPC filter (this yields the actual residual)
      // alternative: lattice filter with reflection coefficients:
      for (i=0; i<Nsrc; i++) {
        FLOAT_DMEM tmp = smileDsp_lattice(refCoeff, latB, p, src[i], NULL);
        if (residualGainScale) {
          if (gain > 0.0) {
            dst[i] = tmp / gain;
          } else {
            dst[i] = 0.0;
          }
        } else {
          dst[i] = tmp;
        }
      }
    }
    lastGain = gain;
  }

  return 1;
}


cLpc::~cLpc()
{
  if (acf != NULL) free(acf);
  if (lpCoeff != NULL) free(lpCoeff);
  if (lastLpCoeff != NULL) free(lastLpCoeff);
  if (refCoeff != NULL) free(refCoeff);
  if (lSpec != NULL) free(lSpec);
  if (latB != NULL) free(latB);
  if (gbb != NULL) free(gbb);
  if (gb2 != NULL) free(gb2);
  if (gaa != NULL) free(gaa);
  if (_ip!=NULL) free(_ip);
  if (_w!=NULL) free(_w);
}

