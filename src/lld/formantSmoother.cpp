/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:


*/



#include <lld/formantSmoother.hpp>

#define MODULE "cFormantSmoother"


SMILECOMPONENT_STATICS(cFormantSmoother)

SMILECOMPONENT_REGCOMP(cFormantSmoother)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFORMANTSMOOTHER;
  sdescription = COMPONENT_DESCRIPTION_CFORMANTSMOOTHER;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")

  // if the inherited config type was found, we register our configuration variables
  SMILECOMPONENT_IFNOTREGAGAIN( {} // <- this is only to avoid compiler warnings...
    // name append has a special role: it is defined in cDataProcessor, and can be overwritten here:
	  // if you set description to NULL, the existing description will be used, thus the following call can
  	// be used to update the default value:
    //ct->setField("nameAppend",NULL,"processed");

    ct->setField("medianFilter0","If > 0, applies median filtering of candidates as the FIRST processing step; the filter length is the value of 'medianFilter0' if > 0",0);
    ct->setField("postSmoothing","If > 0, applies post processing (median and spike remover) over 'postSmoothing' frames (0=no smoothing or use default set by postSmoothingMethod)",0);
    ct->setField("postSmoothingMethod","The post processing method to use. One of the following:\n   'none' disable post smoothing\n   'simple' simple post smoothing using only 1 frame delay (will smooth out 1 frame octave spikes)\n   'median' will apply a median filter to the output values (length = value of 'postProcessing')","simple");
    //ct->setField("octaveCorrection","enable intelligent cross candidate octave correction",1);
    
    ct->setField("F0field","The input field containing either F0final or voicingFinalClipped (i.e. a field who's value is 0 for unvoiced frames and != 0 otherwise), (the name you give here is a partial name, i.e. the actual field names will be matched against *'F0field'*). Note: do not use the *Env* (envelope) fields here, they are != 0 for unvoiced frames!","F0final");
    ct->setField("formantBandwidthField","The input field containing formant bandwidths (the name you give here is a partial name, i.e. the actual field names will be matched against *formantBandwidthField*)","formantBand");
    ct->setField("formantFreqField","The input field containing formant frequencies (the name you give here is a partial name, i.e. the actual field names will be matched against *formantFreqField*)","formantFreq");
    ct->setField("formantFrameIntensField","The input field containing formant frame intensity (the name you give here is a partial name, i.e. the actual field names will be matched against *formantFrameIntensField*)","formantFrameIntens");

    // new data
    ct->setField("intensity","If set to 1, output formant intensity",0);
    ct->setField("nFormants","This sets the maximum number of smoothed formants to output (set to 0 to disable the output of formants and bandwidths)",5);
    ct->setField("formants","If set to 1, output formant frequencies (also see 'nFormants' option)",1);
    ct->setField("bandwidths","If set to 1, output formant bandwidths (also see 'nFormants' option)",0);
    ct->setField("saveEnvs","If set to 1, output formant frequency and bandwidth envelopes instead(!) of the actual data (i.e. the last value of a voiced frame is used for the following unvoiced frames).",0);
    ct->setField("no0f0","'no zero F0': if set to 1, output data only when F0>0, i.e. a voiced frame is detected. This may cause problem with some functionals and framer components, which don't support this variable length data yet...",0);

    
    ct->setField("processArrayFields",NULL,0);
  )

  // The configType gets automatically registered with the config manger by the SMILECOMPONENT_IFNOTREGAGAIN macro
  
  // we now create out sComponentInfo, including name, description, success status, etc. and return that
  SMILECOMPONENT_MAKEINFO(cFormantSmoother);
}

SMILECOMPONENT_CREATE(cFormantSmoother)

//-----

cFormantSmoother::cFormantSmoother(const char *_name) :
  cVectorProcessor(_name),
    median0WorkspaceF0cand(NULL), lastFinal(NULL), nFormantsIn(-1),
    fbin(NULL), fbinLastVoiced(NULL)
{

}

void cFormantSmoother::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();

  medianFilter0 = getInt("medianFilter0");
  SMILE_IDBG(2,"medianFilter0 = %i",medianFilter0);

  postSmoothing = getInt("postSmoothing");
  const char * postSmoothingMethodStr = getStr("postSmoothingMethod");
  if (!strncasecmp(postSmoothingMethodStr,"none",4)) {
    postSmoothingMethod = FPOSTSMOOTHING_NONE;
    postSmoothing = 0;
  } else if (!strncasecmp(postSmoothingMethodStr,"simp",4)) {
    postSmoothingMethod = FPOSTSMOOTHING_SIMPLE;
    postSmoothing = 1;
  } else if (!strncasecmp(postSmoothingMethodStr,"medi",4)) {
    postSmoothingMethod = FPOSTSMOOTHING_MEDIAN;
    if (postSmoothing < 2) postSmoothing = 2; 
  } else {
    SMILE_IERR(1,"unknown post smoothing method '%s'",postSmoothingMethodStr);
    postSmoothingMethod = FPOSTSMOOTHING_NONE;
  }
  SMILE_IDBG(2,"postSmoothing = %i",postSmoothing);
  
  //octaveCorrection = getInt("octaveCorrection");
  //SMILE_IDBG(2,"octaveCorrection = %i",octaveCorrection);

  formants = getInt("formants");
  SMILE_IDBG(2,"formants = %i",formants);

  nFormants = getInt("nFormants");
  SMILE_IDBG(2,"nFormants = %i",nFormants);
  bandwidths = getInt("bandwidths");
  SMILE_IDBG(2,"bandwidths = %i",bandwidths);
  intensity = getInt("intensity");
  SMILE_IDBG(2,"intensity = %i",intensity);
  saveEnvs = getInt("saveEnvs");
  SMILE_IDBG(2,"saveEnvs = %i",saveEnvs);

  no0f0 = getInt("no0f0");
  SMILE_IDBG(2,"no0f0 = %i (not yet well supported)",no0f0);

  if ((postSmoothing>0)&&(nFormants>0)) {
   lastFinal = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*postSmoothing*nFormants);
  }

}


int cFormantSmoother::setupNewNames(long nEl)
{
  int n=0;
    //TODO: usw addNameAppendField to preserve base name
  // final (smoothed) output
  if (intensity) { writer_->addField("formantFrameIntensity", 1); n++; } 
  if (nFormants > 0) {
    if (saveEnvs) {
      if (formants) { writer_->addField("formantFinFreqEnv", nFormants); n += nFormants; } 
      if (bandwidths) { writer_->addField("formantFinBWEnv", nFormants); n += nFormants; } 
    } else {
      if (formants) { writer_->addField("formantFinalFreq", nFormants); n += nFormants; } 
      if (bandwidths) { writer_->addField("formantFinalBW", nFormants); n += nFormants; } 
    }
  }


  const char *field = getStr("F0field");
  long idx = findField(field, 0);
  if (idx >= 0) { F0fieldIdx = idx; }
  else { 
    SMILE_IERR(1,"input field F0field '%s' not found! Usign 0th field...",field);
    F0fieldIdx = 0; 
  }

  if (intensity) {
    field = getStr("formantFrameIntensField");
    idx = findField(field, 0);
    if (idx >= 0) { formantFrameIntensField = idx; }
    else { 
      SMILE_IERR(1,"input field formantFrameIntensField '%s' not found! Usign 0th field...",field);
      formantFrameIntensField = 0; 
    }
  }

  long N=0;
  if (formants) {
    field = getStr("formantFreqField");
    idx = findField(field, 0, &N, NULL, nEl);
    if (idx >= 0) { 
      formantFreqFieldIdx = idx; 
      nFormantsIn = N;
    }
    else { 
      SMILE_IERR(1,"input field formantFreqField '%s' not found! Usign 0th field...",field);
      formantFreqFieldIdx = 0; 
    }
  }

  if (bandwidths) {
    field = getStr("formantBandwidthField");
    idx = findField(field, 0, &N, NULL, nEl);
    if (idx >= 0) { formantBandwidthFieldIdx = idx; }
    else { 
      SMILE_IERR(1,"input field formantBandwidthField '%s' not found! Usign 0th field...",field);
      formantBandwidthFieldIdx = 0; 
    }
    if (nFormantsIn == -1) nFormantsIn = N;
    else { 
      if (nFormantsIn != N) { 
        SMILE_IERR(1,"size of formantBandwidthField (%i) differs from size of formantFreqField (%i)! Either your config is incorrect or this is a bug!",N,nFormantsIn);
        COMP_ERR("aborting");
      }
    }
  }


  if (medianFilter0 > 0) {
    median0WorkspaceF0cand = smileUtil_temporalMedianFilterInitSl(nFormantsIn, 1, medianFilter0);
  }

  if (nFormantsIn > 0) {
    long bw=0;
    if (formants) bw++;
    if (bandwidths) bw++;
    if (bw > 0) {
      fbin=(FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*nFormantsIn*bw);
      fbinLastVoiced=(FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*nFormantsIn*bw);
    }
  }

  if (nFormants > nFormantsIn) {
    SMILE_IWRN(1,"more output formants requested in config (%i) than there are input formants available (%i)! Limiting number of output formants.",nFormants,nFormantsIn);
    nFormants = nFormantsIn;
  }

  namesAreSet_ = 1;
  return n;
}

// a derived class should override this method, in order to implement the actual processing
int cFormantSmoother::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  long i;
  long n=0;

  
  // collect all input candidates
  int c = 0;
  if (fbin != NULL) {
    for (i=0; i<nFormantsIn; i++) {
      int offs = 0;
      if (formants) { fbin[i] = src[formantFreqFieldIdx+i]; offs = nFormantsIn; }
      if (bandwidths) fbin[i+offs] = src[formantBandwidthFieldIdx+i];
      if (fbin[i] != 0.0) c++;
    }
  }

  
  // median filter:
  if (medianFilter0 > 0) {
    smileUtil_temporalMedianFilterWslave(fbin, c, 1, median0WorkspaceF0cand);
  }
  
  //apply voiced/unvoiced filter:
  long bw=0;
  if (formants) bw++;
  if (bandwidths) bw++;

  FLOAT_DMEM voice = src[F0fieldIdx];
  if (voice > 0.0) {
    for (i=0; i<nFormantsIn*bw; i++) {
      fbinLastVoiced[i] = fbin[i];
    }
  } else {
    for (i=0; i<nFormantsIn*bw; i++) {
      if (saveEnvs) {
        fbin[i] = fbinLastVoiced[i];
      } else {
        fbin[i] = 0.0;
      }
    }
  }

  
  //output:
  if (intensity) { 
    if (formantFrameIntensField>=0) {
      dst[n++] = src[formantFrameIntensField];
    } else {
      dst[n++] = 0.0; 
    }
  }

  if (nFormants > 0) {
    //if (saveEnvs) {
      int offs = 0;
      if (formants) { 
        for (i=0; i<nFormants; i++) {
          dst[n++] = fbin[i];
        }
        offs = nFormantsIn;
      } 
      if (bandwidths) { 
        for (i=0; i<nFormants; i++) {
          dst[n++] = fbin[i+offs];
        }
      } 
    /*} else {
      int offs = 0;
      if (formants) { 
        for (i=0; i<nFormants; i++) {
          dst[n++] = fbin[i]; 
        }
        offs = nFormantsIn;
      } 
      if (bandwidths) { 
        for (i=0; i<nFormants; i++) {
          dst[n++] = fbin[i+offs];
        }
      } 
    }*/
  }


  return n;
}

cFormantSmoother::~cFormantSmoother()
{
  if (median0WorkspaceF0cand != NULL) smileUtil_temporalMedianFilterFree(median0WorkspaceF0cand);
  if (lastFinal != NULL) free(lastFinal);
  if (fbin != NULL) free(fbin);
  if (fbinLastVoiced != NULL) free(fbinLastVoiced);
}

