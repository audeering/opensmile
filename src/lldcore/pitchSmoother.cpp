/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:


*/



#include <lldcore/pitchSmoother.hpp>

#define MODULE "cPitchSmoother"


SMILECOMPONENT_STATICS(cPitchSmoother)

SMILECOMPONENT_REGCOMP(cPitchSmoother)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CPITCHSMOOTHER;
  sdescription = COMPONENT_DESCRIPTION_CPITCHSMOOTHER;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")

  // if the inherited config type was found, we register our configuration variables
  SMILECOMPONENT_IFNOTREGAGAIN( {} // <- this is only to avoid compiler warnings...
    // name append has a special role: it is defined in cDataProcessor, and can be overwritten here:
	  // if you set description to NULL, the existing description will be used, thus the following call can
  	// be used to update the default value:
    //ct->setField("nameAppend",NULL,"processed");

    ct->setField("medianFilter0","Apply median filtering of candidates as the FIRST processing step; filter length is 'medianFilter0' if > 0",0);
    ct->setField("postSmoothing","Apply post processing (median and spike remover) over 'postSmoothing' frames (0=no smoothing or use default set by postSmoothingMethod)",0);
    ct->setField("postSmoothingMethod","Post processing method to use. One of the following:\n   'none' disable post smoothing\n   'simple' simple post smoothing using only 1 frame delay (will smooth out 1 frame octave spikes)\n   'median' will apply a median filter to the output values (length = value of 'postProcessing')","simple");
    ct->setField("octaveCorrection","Enable intelligent cross candidate octave correction",1);
    
    // new data
    ct->setField("F0final","1 = Enable output of final (corrected and smoothed) F0",1);
    ct->setField("F0finalEnv","1 = Enable output of envelope of final smoothed F0 (i.e. there will be no 0 values (except for end and beginning))",0);
    ct->setField("no0f0","1 = enable 'no zero F0', output data only when F0>0, i.e. a voiced frame is detected. This may cause problem with some functionals and framer components, which don't support this variable length data yet...",0);

    ct->setField("voicingFinalClipped","1 = Enable output of final smoothed and clipped voicing (pseudo) probability. 'Clipped' means that the voicing probability is set to 0 for unvoiced regions, i.e. where the probability lies below the voicing threshold.",0);
    ct->setField("voicingFinalUnclipped","1 = Enable output of final smoothed, raw voicing (pseudo) probability (UNclipped: not set to 0 during unvoiced regions).",0);
    
    // data forwarded from pitch detector
    ct->setField("F0raw","1 = Enable output of 'F0raw' copied from input",0);
    ct->setField("voicingC1","1 = Enable output of 'voicingC1' copied from input",0);
    ct->setField("voicingClip","1 = Enable output of 'voicingClip' copied from input",0);

    ct->setField("processArrayFields",NULL,0);
  )

  // The configType gets automatically registered with the config manger by the SMILECOMPONENT_IFNOTREGAGAIN macro
  
  // we now create out sComponentInfo, including name, description, success status, etc. and return that
  SMILECOMPONENT_MAKEINFO(cPitchSmoother);
}

SMILECOMPONENT_CREATE(cPitchSmoother)

//-----

cPitchSmoother::cPitchSmoother(const char *_name) :
  cVectorProcessor(_name),
  nCands(NULL), nCandidates(NULL), 
  f0cand(NULL), candVoice(NULL), candScore(NULL),
  voicingCutoff(NULL),
  f0candI(NULL), candVoiceI(NULL), candScoreI(NULL),
  voicingC1I(NULL), F0rawI(NULL), voicingClipI(NULL),
  median0WorkspaceF0cand(NULL), lastFinal(NULL),
  onsFlag(0), onsFlagO(0), firstFrame(1),
  lastVoice(0), lastFinalF0(0), pitchEnv(0.0)
{

}

void cPitchSmoother::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();

  medianFilter0 = getInt("medianFilter0");
  SMILE_IDBG(2,"medianFilter0 = %i",medianFilter0);

  postSmoothing = getInt("postSmoothing");
  const char * postSmoothingMethodStr = getStr("postSmoothingMethod");
  if (postSmoothingMethodStr != NULL){
  if (!strncasecmp(postSmoothingMethodStr,"none",4)) {
    postSmoothingMethod = POSTSMOOTHING_NONE;
    postSmoothing = 0;
  } else if (!strncasecmp(postSmoothingMethodStr,"simp",4)) {
    postSmoothingMethod = POSTSMOOTHING_SIMPLE;
    postSmoothing = 1;
  } else if (!strncasecmp(postSmoothingMethodStr,"medi",4)) {
    postSmoothingMethod = POSTSMOOTHING_MEDIAN;
    if (postSmoothing < 2) postSmoothing = 2; 
  } else {
    SMILE_IERR(1,"unknown post smoothing method '%s'",postSmoothingMethodStr);
    postSmoothingMethod = POSTSMOOTHING_NONE;
  }
  }
  SMILE_IDBG(2,"postSmoothing = %i",postSmoothing);
  if (postSmoothing>0) {
    lastFinal = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*postSmoothing);
  }
  
  octaveCorrection = getInt("octaveCorrection");
  SMILE_IDBG(2,"octaveCorrection = %i",octaveCorrection);

  
  F0final = getInt("F0final");
  SMILE_IDBG(2,"F0final = %i",F0final);
  F0finalEnv = getInt("F0finalEnv");
  SMILE_IDBG(2,"F0finalEnv = %i",F0finalEnv);
  no0f0 = getInt("no0f0");
  SMILE_IDBG(2,"no0f0 = %i (not yet well supported)",no0f0);

  voicingFinalClipped = getInt("voicingFinalClipped");
  SMILE_IDBG(2,"voicingFinalClipped = %i",voicingFinalClipped);
  voicingFinalUnclipped = getInt("voicingFinalUnclipped");
  SMILE_IDBG(2,"voicingFinalUnclipped = %i",voicingFinalUnclipped);

  F0raw = getInt("F0raw");
  SMILE_IDBG(2,"F0raw = %i",F0raw);
  voicingC1 = getInt("voicingC1");
  SMILE_IDBG(2,"voicingC1 = %i",voicingC1);
  voicingClip = getInt("voicingClip");
  SMILE_IDBG(2,"voicingClip = %i",voicingClip);

}


int cPitchSmoother::setupNewNames(long nEl)
{

  int n=0;
    //TODO: usw addNameAppendField to preserve base name
  // final (smoothed) output
  if (F0final) { writer_->addField("F0final", 1); n++; } 
  if (F0finalEnv) { writer_->addField("F0finEnv", 1); n++; } 
  if (voicingFinalClipped) { writer_->addField("voicingFinalClipped", 1); n++; } 
  if (voicingFinalUnclipped) { writer_->addField("voicingFinalUnclipped", 1); n++; } 

  // copied from input:
  if (voicingC1) { writer_->addField( "voicingC1", 1 ); n++;}
  if (F0raw) { writer_->addField( "F0raw", 1 ); n++; }
  if (voicingClip) { writer_->addField( "voicingClip", 1 ); n++; }

  long i,idx,nC;
  nInputLevels = reader_->getNLevels();
  if (nInputLevels<1) nInputLevels=1;
  voicingCutoff = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*nInputLevels);
  nCandidates = (int*)calloc(1,sizeof(int)*nInputLevels);
  nCands = (int*)calloc(1,sizeof(int)*nInputLevels);
  f0candI = (int*)calloc(1,sizeof(int)*nInputLevels);
  candVoiceI = (int*)calloc(1,sizeof(int)*nInputLevels);
  candScoreI = (int*)calloc(1,sizeof(int)*nInputLevels);
  F0rawI = (int*)calloc(1,sizeof(int)*nInputLevels);
  voicingClipI = (int*)calloc(1,sizeof(int)*nInputLevels);
  voicingC1I = (int*)calloc(1,sizeof(int)*nInputLevels);

  totalCands = 0; 
  int more=0; int moreV=0; int moreS=0;
  int moreC1=0; int moreRa=0; int moreVc=0;
  
  for (i=0; i<nInputLevels; i++) {

    cVectorMeta *mdata = reader_->getLevelMetaDataPtr(i);
    if (mdata != NULL) {
      // TODO : check mdata ID
      voicingCutoff[i] = mdata->fData[0];
      SMILE_IMSG(3,"voicing cutoff read from input level %i = %f\n",i,voicingCutoff[i]);
    }
    
    if (F0raw) {
      int idxRa = findField("F0raw",0,NULL,NULL,-1,&moreRa); // TODO: check if nC == 1 here
      //printf("idxRa %i moreRa %i  i %i\n",idxRa,moreRa,i);
      F0rawI[i] = idxRa;
      if (moreRa>0) { moreRa = idxRa+1; }
    }
    if (voicingClip) {
      int idxVc = findField("voicingClip",0,NULL,NULL,-1,&moreVc); // TODO: check if nC == 1 here
      voicingClipI[i] = idxVc;
      if (moreVc>0) { moreVc = idxVc+1; }
    }
    if (voicingC1) {
      int idxC1 = findField("voicingC1",0,NULL,NULL,-1,&moreC1); // TODO: check if nC == 1 here
      voicingC1I[i] = idxC1;
      if (moreC1>0) { moreC1 = idxC1+1; }
    }

    idx = findField("F0Cand",0,&nC,NULL,-1,&more);
    f0candI[i] = idx;
    //printf("idx %i\n",idx);
    if (idx >= 0) {
      int idxV = findField("candVoicing",0,NULL,NULL,-1,&moreV); 
      candVoiceI[i] = idxV;
      if (moreV>0) { moreV = idxV+1; }
      int idxS = findField("candScore",0,NULL,NULL,-1,&moreS); // TODO: check if nC == N here
      candScoreI[i] = idxS;
      if (moreS>0) { moreS = idxS+1; }
      nCandidates[i] = nC;
      if (more>0) { more = idx+1; }
      else {
        totalCands += nC;
        nInputLevels = i+1;
        break;
      }
    } else {
      nCandidates[i] = 0;
      nInputLevels = i;
      break;
    }
    totalCands += nC;
    
    
  }

  if (f0cand == NULL) f0cand = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*totalCands*3);
  candVoice = f0cand+totalCands; //(FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*totalCands);
  candScore = f0cand+totalCands*2; //(FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*totalCands);

  if (medianFilter0 > 0) {
    median0WorkspaceF0cand = smileUtil_temporalMedianFilterInitSl(totalCands, 2, medianFilter0);
  }

  namesAreSet_ = 1;
  return n;
}

// NOTE: pitchSmoother introduces a delay of 1 frame, vIdx will not reflect the correct start time anymore,
//  instead, tmeta->time should be used!
int cPitchSmoother::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  long i, j;
  long n=0;

  // collect all input candidates
  int c = 0;
  for (i=0; i<nInputLevels; i++) {
    for (j=0; j<nCandidates[i]; j++) {
      candVoice[c] = src[candVoiceI[i]+j];
      candScore[c] = src[candScoreI[i]+j];
      f0cand[c++] = src[f0candI[i]+j];
      //printf("f0candI[%i] %i\n",i,f0candI[i]);
    }
  }
  
  // median filter:
  if (medianFilter0 > 0) {
    smileUtil_temporalMedianFilterWslave(f0cand, c /*totalCands*/, 2, median0WorkspaceF0cand);
  }
  
  if (octaveCorrection) {
    // **ocatve correction**:
    // best candidate is always the 0th (see pitchBase), all other candidates are in arbitrary order
    // thus the smoothing here tries to validate the best candidate
    // a) if another candidate (which is lower in f is found, we consider to use it)
    // b) if the other candidates are multiples of half the 0th candidate, we consider to use half the 0th candidate
    // c) we do global optimisation via median filtering (1st step actually)
    // d) we do global optimisation via octave jump costs and evaluating a) and b) at octave jumps + voicing probs
    int cand0ismin = 1;
    FLOAT_DMEM vpMin = 0.0;
    int minC = -1;
    for (i=1; i<c; i++) {
      if ((f0cand[i] > 0.0)&&(f0cand[i] < f0cand[0])) { // a)
        if ((candVoice[i] > 0.9*candVoice[0])&&(candVoice[i] > vpMin)) {
          vpMin = candVoice[i]; minC = i;
        }
        cand0ismin = 0;
      } 
    }
    if (!cand0ismin) {
      if (minC >= 0) { // use the alternate, lower frequency candidate...
        FLOAT_DMEM t = f0cand[0];
        f0cand[0] = f0cand[minC];
        f0cand[minC] = t;
        t = candVoice[0];
        candVoice[0] = candVoice[minC];
        candVoice[minC] = t;
        t = candScore[0];
        candScore[0] = candScore[minC];
        candScore[minC] = t;
      } 
    } else {
      //if (cand0ismin) { // check for b)
      int halfed = 0; j=0;
      while ((!halfed) && j<c-1) {
        for (i=j+1; i<c; i++) {
          if ((f0cand[i] > 0.0)&&(f0cand[j] > 0.0)) {
            FLOAT_DMEM k = fabs(f0cand[i]-f0cand[j])*(FLOAT_DMEM)2.0/f0cand[0];
            k = (FLOAT_DMEM)fabs(k-1.0);
            if (k<0.1) { // b)
              f0cand[0] /= (FLOAT_DMEM)2.0;
              halfed = 1;
              break;
            } 
          }
        }
        j++;
      }
    //} else {
    }
  }


  if (no0f0 && (candVoice[0] <= voicingCutoff[0])) {
    // experimental: no output for 0 pitch.. (unvoiced..)
    return 0;
  }

  /* output */
  FLOAT_DMEM voiceC1 = candVoice[0];
  if (F0final||F0finalEnv) {
    FLOAT_DMEM pitch, pitchOut;
    if (candVoice[0] > voicingCutoff[0]) {
      pitch = f0cand[0]; 
    } else {
      pitch = 0.0;
    }

    // post smoothing:
    if (postSmoothing) {
      if (postSmoothingMethod == POSTSMOOTHING_SIMPLE) {
        if (firstFrame) { firstFrame = 0; return 0; } // for proper synchronisation
        voiceC1 = lastVoice; lastVoice = candVoice[0];

        // simple pitch contour smoothing (delay: 1 frame):

        if ((lastFinal[0] == 0.0)&&(pitch>0.0)) onsFlag = 1;
        if ((lastFinal[0] > 0.0)&&(pitch==0.0)&&(onsFlag==0)) onsFlag = -1;
        if ((lastFinal[0] > 0.0)&&(pitch>0.0)) onsFlag = 0;
        if ((lastFinal[0] == 0.0)&&(pitch==0.0)) onsFlag = 0;

        if ((pitch==0.0)&&(onsFlag==1)) { lastFinal[0] = 0.0; }
        else if ((pitch>0.0)&&(onsFlag==-1)) { lastFinal[0] = pitch; }
        
        
        int doubling = 0; int halfing = 0;
        if ((lastFinal[0]>0.0)&&(pitch>0.0)) {
          FLOAT_DMEM factor = lastFinal[0]/pitch;
          if (factor > 1.2) halfing = 1; /* old: fabs(factor-2.0)<0.15 */
          else if (factor < 0.8) doubling = 1; /* old : fabs(factor-0.5)<0.05 */
        }

        if ((doubling)&&(onsFlagO==-1)) { lastFinal[0] = pitch; }
        else if ((halfing)&&(onsFlagO==1)) { lastFinal[0] = pitch; }

        if (doubling) onsFlagO = 1;
        if (halfing && (onsFlag==0)) onsFlagO = -1;
        if (!(halfing||doubling)) onsFlagO = 0;
        



        pitchOut = lastFinal[0]; // dst[n]
        // shift last final...
        for (i=postSmoothing-1; i>0; i--) {
          lastFinal[i] = lastFinal[i-1];
        }
        lastFinal[0] = pitch;
        
      } else if (postSmoothingMethod == POSTSMOOTHING_MEDIAN) {
        //if (firstFrame) { firstFrame = 0; return 0; }  // for proper synchronisation

        // shift last final...
        for (i=postSmoothing-1; i>0; i--) {
          lastFinal[i] = lastFinal[i-1];
        }
        lastFinal[0] = pitch;
        pitchOut /*dst[n]*/ = smileMath_median( lastFinal, postSmoothing, NULL );
        

      } else { // no smoothing...
        pitchOut/*dst[n]*/ = pitch;
      }
    } else { // no smoothing...
      pitchOut/*dst[n]*/ = pitch;
    }
    if (pitchOut > 0.0) lastFinalF0 = pitchOut;
    if (F0final) {
      dst[n++] = pitchOut;
    }
    if (F0finalEnv) {
      //dst[n++] = lastFinalF0;
      if (pitchOut > 0.0) {
        if (pitchEnv == 0.0) pitchEnv = pitchOut;
        else pitchEnv = (FLOAT_DMEM)0.75*pitchEnv + (FLOAT_DMEM)0.25*pitchOut;
      }
      dst[n++] = pitchEnv;
    }
  }

  if (voicingFinalClipped) {
    if (voiceC1 > voicingCutoff[0]) {
      dst[n] = voiceC1;
    } else {
      dst[n] = 0.0;
    }
    n++;
  }
  if (voicingFinalUnclipped) {
    dst[n] = voiceC1;
    n++;
  }

  // copy raw data from input (if enabled) ( TODO: choose best input level!)
  if (voicingC1) { 
    dst[n] = src[voicingC1I[0]]; n++;
  }
  if (F0raw) { 
    //printf("F0rawI[0] %i\n",F0rawI[0]);
    dst[n] = src[F0rawI[0]]; n++; 
  }
  if (voicingClip) { 
    dst[n] = src[voicingClipI[0]]; n++;
  }


  return n;
}

cPitchSmoother::~cPitchSmoother()
{
  if (voicingCutoff != NULL) free(voicingCutoff);
  if (nCandidates != NULL) free(nCandidates);
  if (nCands != NULL) free(nCands);
  if (f0cand != NULL) free(f0cand);
  //if (candVoice != NULL) free(candVoice);
  //if (candScore != NULL) free(candScore);
  if (f0candI != NULL) free(f0candI);
  if (candVoiceI != NULL) free(candVoiceI);
  if (candScoreI != NULL) free(candScoreI);
  if (F0rawI != NULL) free(F0rawI);
  if (voicingClipI != NULL) free(voicingClipI);
  if (voicingC1I != NULL) free(voicingC1I);
  if (median0WorkspaceF0cand != NULL) smileUtil_temporalMedianFilterFree(median0WorkspaceF0cand);
  if (lastFinal != NULL) free(lastFinal);
}

