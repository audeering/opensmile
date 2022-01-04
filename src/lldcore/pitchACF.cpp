/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

pitch detection via ACF and Cepstrum

*/



#include <lldcore/pitchACF.hpp>

#define MODULE "cPitchACF"


SMILECOMPONENT_STATICS(cPitchACF)

SMILECOMPONENT_REGCOMP(cPitchACF)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CPITCHACF;
  sdescription = COMPONENT_DESCRIPTION_CPITCHACF;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")

  // if the inherited config type was found, we register our configuration variables
  SMILECOMPONENT_IFNOTREGAGAIN( {} // <- this is only to avoid compiler warnings...
    // name append has a special role: it is defined in cDataProcessor, and can be overwritten here:
	// if you set description to NULL, the existing description will be used, thus the following call can
	// be used to update the default value:
    //ct->setField("nameAppend",NULL,"processed");
    ct->setField("maxPitch","Maximum detectable pitch in Hz", 500.0);  
    ct->setField("voiceProb","1/0 = on/off: output of voicing probability", 1);
    ct->setField("voiceQual","1/0 = on/off: output of voice 'quality' (= ZCR of ACF)", 0);
    ct->setField("HNR","1/0 = on/off; output of log (base e) harmonics-to-noise ratio (HNR) computed from the ACF, pre 2.0 compatibility.", 0);
    ct->setField("HNRdB","1/0 = on/off; output of log (dB, base 10) harmonics-to-noise ratio (HNR) computed from the ACF. Recommended.", 0);
    ct->setField("linHNR","1/0 = on/off; output of linear harmonics-to-noise ratio (HNR) computed from the ACF.", 0);
    ct->setField("F0","1/0 = on/off: output of F0 (pitch) (Note: the pitch output is 0 in unvoiced segments)", 0);
    ct->setField("F0raw","1/0 = on/off: output of raw F0 candidate without thresholding in unvoiced segments", 0);
    ct->setField("F0env","1/0 = on/off: output of F0 envelope (expontial decay smoothing) (Note: this differs from the envelope computed by the cPitchBase descendant components, such as cPitchSHS!)", 0);
	  ct->setField("voicingCutoff","This sets the voicing probability threshold for pitch detection [0.0 - 1.0]. Frames with voicing probability values above this threshold will be considered as voiced.", 0.55);
	  ct->setField("processArrayFields",NULL,0);
  )

  // The configType gets automatically registered with the config manger by the SMILECOMPONENT_IFNOTREGAGAIN macro
  
  // we now create out sComponentInfo, including name, description, success status, etc. and return that
  SMILECOMPONENT_MAKEINFO(cPitchACF);
}

SMILECOMPONENT_CREATE(cPitchACF)

//-----

cPitchACF::cPitchACF(const char *_name) :
  cVectorProcessor(_name),
  fsSec(-1.0),
  lastPitch(0.0),
  lastlastPitch(0.0),
  glMeanPitch(0.0),
  onsFlag(0),
  pitchEnv(0.0),
  F0raw(0)
{

}

void cPitchACF::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();
  
  HNR = getInt("HNR");
  SMILE_IDBG(2,"HNR = %i",HNR);
  HNRdB = getInt("HNRdB");
  SMILE_IDBG(2,"HNRdB = %i",HNRdB);
  linHNR = getInt("linHNR");
  SMILE_IDBG(2,"linHNR = %i",linHNR);
  voiceProb = getInt("voiceProb");
  SMILE_IDBG(2,"voiceProb = %i",voiceProb);
  voiceQual = getInt("voiceQual");
  SMILE_IDBG(2,"voiceQual = %i",voiceQual);
  F0 = getInt("F0");
  SMILE_IDBG(2,"F0 = %i",F0);
  F0raw = getInt("F0raw");
  SMILE_IDBG(2,"F0raw = %i",F0raw);
  F0env = getInt("F0env");
  SMILE_IDBG(2,"F0env = %i",F0env);
  
  voicingCutoff = getDouble("voicingCutoff");
  if (voicingCutoff > 1.0) voicingCutoff = 1.0;
  if (voicingCutoff < 0.0) voicingCutoff = 0.0;
  SMILE_IDBG(2,"voicingCutoff = %i",voicingCutoff);

  maxPitch = getDouble("maxPitch");
  if (maxPitch < 0.0) maxPitch = 0.0;
  SMILE_IDBG(2,"maxPitch = %lf",maxPitch);
  

}


// optional

int cPitchACF::setupNewNames(long nEl)
{
  if (fsSec == -1.0) {
    const sDmLevelConfig *c = reader_->getLevelConfig();
    fsSec = (float)(c->frameSizeSec);
  }

  int n=0;
  if (voiceProb) { writer_->addField( "voiceProb", 1 ); n++; }
  if (HNR) { writer_->addField( "HNR", 1 ); n++;}
  if (HNRdB) { writer_->addField( "HNRdBacf", 1 ); n++;}
  if (linHNR) { writer_->addField( "linearHNRacf", 1 ); n++;}
  if (voiceQual) { writer_->addField( "voiceQual", 1 ); n++; }
  if (F0) { writer_->addField( "F0", 1 ); n++; }
  if (F0raw) { writer_->addField( "F0raw", 1 ); n++; }
  if (F0env) { writer_->addField( "F0env", 1 ); n++; }

  namesAreSet_ = 1;

  return n;
//  return cVectorProcessor::setupNamesForField(i,name,nEl);
}



// implementation of the actual processing
int cPitchACF::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // do domething to data in *src, save result to *dst
  // NOTE: *src and *dst may be the same...
  long maxIdx;
  double voicing;
  double hnr = 0.0;
  double hnrDB = 0.0;
  double hnrLin = 0.0;
  double acfZcr = 0.0;
  //long N = (int)floor(Nsrc/2.0);
  long N = (int)floor(Nsrc/2.0); // length of ACF: assumes ACF+Cepstrum input array, ACF is half size of cepstrum
  //long N = Nsrc/2;

  double Nd = (double)(Nsrc);  // assumes ACF+Cepstrum input array, ACF is half size of cepstrum, where cepstrum is full frame size in samples
  double F0fft = 1.0/fsSec;
  double Tsamp = fsSec/Nd;
  // NO!!! don't hardcode samplerate! ;-)  double Tsamp = 1.0/44100.0;  // sampling period  

  int preskip;
  if (maxPitch <= 0.0)
    preskip = 0;
  else preskip = (int)(1.0 / (maxPitch*Tsamp));  // this variable determines the maximal pitch detectable
  int halflen = (int)(Nd/2.0)-preskip+1;
  // detect voicing prob. and zero crossing rate (clarity) from normal acf:
  voicing = voicingProb(src,N,preskip,&acfZcr);
  // find pitch peak in log acf (cepstrum):
  maxIdx = pitchPeak(src+N,N,preskip+1);
  // compute HNR
  if (HNR) {
    hnr = computeHNR(src,maxIdx);
  }
  if (HNRdB) {
    hnrDB = computeHNR_dB(src,maxIdx);
  }
  if (linHNR) {
    hnrLin = computeHNR_lin(src,maxIdx);
  }

  int n=0;

  if (voiceProb) dst[n++] = (FLOAT_DMEM)voicing;
  if (HNR) dst[n++] = (FLOAT_DMEM)hnr;
  if (HNRdB) dst[n++] = (FLOAT_DMEM)hnrDB;
  if (linHNR) dst[n++] = (FLOAT_DMEM)hnrLin;
  if (F0 || F0env || voiceQual || F0raw) {
    FLOAT_DMEM vq = ((FLOAT_DMEM)maxPitch - (FLOAT_DMEM)fabs((acfZcr*maxPitch)-((FLOAT_DMEM)1.0/((FLOAT_DMEM)(maxIdx)*(FLOAT_DMEM)Tsamp))) )* (FLOAT_DMEM)voicing;
    if (maxIdx == 0.0) vq = 0.0;
    if (voiceQual) dst[n++] = vq;
    FLOAT_DMEM pitch = 0.0;
    FLOAT_DMEM rawF0 = 0.0;
    if (maxIdx > 0) {
      pitch = (FLOAT_DMEM)1.0/((FLOAT_DMEM)(maxIdx)*(FLOAT_DMEM)Tsamp);
      rawF0 = pitch;
    }
    if (voicing < voicingCutoff) { maxIdx = 0; pitch = 0.0; }  //vq/maxPitch




    // very simple pitch contour smoothing:

    if ((lastPitch == 0.0)&&(pitch>0.0)) onsFlag = 1;
    if ((lastPitch > 0.0)&&(pitch==0.0)&&(onsFlag==0)) onsFlag = -1;
    if ((lastPitch > 0.0)&&(pitch>0.0)) onsFlag = 0;
    if ((lastPitch == 0.0)&&(pitch==0.0)) onsFlag = 0;

    if ((pitch==0.0)&&(onsFlag==1)) { lastPitch = 0.0; }

    FLOAT_DMEM oPitch = pitch;
    FLOAT_DMEM tol = (FLOAT_DMEM)0.4;
    FLOAT_DMEM alpha = (FLOAT_DMEM)0.3;

    if (pitch > 0.0) {
      if (glMeanPitch == 0.0) glMeanPitch = pitch;
      //if ((lastPitch == 0.0)||(onsFlag==1)) tol = 0.1;
      if (!((pitch < ((FLOAT_DMEM)1.0+tol)*glMeanPitch)&&(pitch > ((FLOAT_DMEM)1.0-tol) * glMeanPitch))) {
        pitch = glMeanPitch;
        alpha /= (FLOAT_DMEM)3.0;
      } 
      if (onsFlag&&(lastPitch > pitch)) lastPitch *= (FLOAT_DMEM)0.85;
    } 
    if ((pitch>0.0)&&(onsFlag==-1)) { lastPitch = pitch; }


    if (oPitch > (FLOAT_DMEM)0.0)
      glMeanPitch = ((FLOAT_DMEM)1.0-alpha)*glMeanPitch + alpha*oPitch;    // TODO: pitch decay constant!


    FLOAT_DMEM out;
    if ((lastlastPitch!=(FLOAT_DMEM)0.0)&&(lastPitch!=0.0))
      out = (FLOAT_DMEM)0.5*(lastlastPitch + lastPitch);
    else 
      out = lastPitch;

    if (F0) dst[n++] = out;   // TODO: normPitch option

    if (F0raw) dst[n++] = rawF0;   // TODO: normPitch option

    lastlastPitch = lastPitch;
    lastPitch = pitch;

    if (F0env) {
      if (out > 0.0)
        pitchEnv = (FLOAT_DMEM)0.75*pitchEnv + (FLOAT_DMEM)0.25*out;
      dst[n++] = pitchEnv;
    }
  }

  return n;
}

double cPitchACF::voicingProb(const FLOAT_DMEM *a, int n, int skip, double *Zcr)
{
  int zcr=0, mcr=0;
  double mean=0.0;
  double max;
  
  //n /= 2;
  // zero crossings and max
  max = a[n-1];
  mean = a[skip];
  for (int i=1; i<n; i++) {
    if ( a[i-1]*a[i] < 0 ) zcr++;
    if (i>=skip) {
      if ((a[i] > max) && (a[i-1] < a[i])) max = a[i];
      mean += a[i];
    }
  }
  mean /= (double)(n-skip+1);

  for (int i=1; i<n; i++) {
    if ( (a[i-1]-mean)*(a[i]-mean) < 0 ) mcr++;
  }

  if (Zcr != NULL) {
    if (mcr > zcr) {
      *Zcr = (double)mcr / (double)n;
    } else {
      *Zcr = (double)zcr / (double)n;
    }
  }

  if (a[0] > 0) 
    return max/a[0];  
  else 
    return 0.0;
}

long cPitchACF::pitchPeak(const FLOAT_DMEM *a, long n, long skip)
{
  double max;
  double buf;
  double sum = 0.0;

  // first pass: compute mean and find absolute max
  max = a[n-1];
  for (int i=n-1; i>=0; i--) {
    buf = a[i];
    sum += fabs(buf);
	if (i>=skip) if (buf > max) max = buf;
  }
  sum /= n;

  // second pass: peak detection
  for (int i=skip+1; i<n-1; i++) {
    if (a[i] > (max+sum)*0.6) {
      if ( (a[i-1] < a[i]) && (a[i] > a[i+1]) ) { // we found a peak!
        return i;
      }
    }
  }
  return 0;    
}

double cPitchACF::computeHNR(const FLOAT_DMEM *a, int f0Idx)
{
  double buf;
  double ret;
  if ((a[0]-a[f0Idx]) == 0.0) buf = 100000000000000000000.0;
  else buf = a[f0Idx]/(a[0]-a[f0Idx]);
    
  if (buf>0.00000000001) {
    ret = 10.0*log(buf);
  } else {
    ret = 10.0*log(0.00000000001);
  }
  return ret;
}

/* computes HNR from ACF in dB for a range -100 to +100 dB */
double cPitchACF::computeHNR_dB(const FLOAT_DMEM *a, int f0Idx)
{
  double buf = a[0] - a[f0Idx];
  if (buf == 0.0) {
    buf = 10e10;
  } else {
    buf = a[f0Idx]/buf;
  }
  if (buf <= 10e-10) {
    return -100.0;
  } else if (buf >= 10e10) {
    return +100.0;
  } else {
    return 10.0 * log(buf) / log(10.0);
  }
}

/* computes HNR from ACF in dB for a range -100 to +100 dB */
double cPitchACF::computeHNR_lin(const FLOAT_DMEM *a, int f0Idx)
{
  double buf = a[0] - a[f0Idx];
  if (buf == 0.0) {
    buf = 10e3;
  } else {
    buf = a[f0Idx]/buf;
  }
  if (buf <= 10e-3) {
    return 10e-3;
  } else if (buf >= 10e3) {
    return 10e3;
  } else {
    return buf;
  }
}

cPitchACF::~cPitchACF()
{
}

