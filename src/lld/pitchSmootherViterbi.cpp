/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

// TODO: discard pitch < 80 Hz, if 2 or less frames only (i.e. set to 0, unvoiced)
// TODO: discard pitch < 60 Hz, if 5 or less frames only (i.e. set to 0, unvoiced)

/*  openSMILE component:

viterbi pitch smoothing. 
Loosely based on:
I. Luengo (Basque Univ., Bilbao), "Evaluation of Pitch Detection Algorithms Under Real Conditions", Proc. ICASSP 2007

*/


#include <lld/pitchSmootherViterbi.hpp>

#define MODULE "cPitchSmootherViterbi"

SMILECOMPONENT_STATICS(cPitchSmootherViterbi)

SMILECOMPONENT_REGCOMP(cPitchSmootherViterbi)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CPITCHSMOOTHERVITERBI;
  sdescription = COMPONENT_DESCRIPTION_CPITCHSMOOTHERVITERBI;

  // we inherit cDataProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")
  
  if (ct->setField("reader2", "Configuration of the dataMemory reader sub-component which is used to read input frames with a certain lag (max. bufferLength!).",
                    _confman->getTypeObj("cDataReader"), NO_ARRAY, DONT_FREE) == -1) {
       rA=1; // if subtype not yet found, request , re-register in the next iteration
  }

  SMILECOMPONENT_IFNOTREGAGAIN(
   ct->setField("bufferLength","The length of the delay buffer in (input) frames. This is the amount of data that will be used for the Viterbi smoothing, and it is also the lag which the output is behind the input. The input level buffer must be at least bufferLength+1 in size!.",30);

   // new data
   ct->setField("F0final","1 = Enable output of final (corrected and smoothed) F0 -- linear scale", 1);
   ct->setField("F0finalLog","1 = Enable output of final (corrected and smoothed) F0 in logarithmic representation (semitone scale with base note 27.5 Hz - a linear F0 equal to and below 29.136 Hz (= 1 on the semitone scale) will be clipped to an output value of 1, since 0 is reserved for unvoiced).", 0);
   ct->setField("F0finalEnv","1 = Enable output of envelope of final smoothed F0 (i.e. there will be no 0 values (except for the beginning). Envelope method is to hold the last valid sample, no interpolation is performed. [EXPERIMENTAL!]",0);
   ct->setField("F0finalEnvLog","1 = Enable output of envelope of final smoothed F0 (i.e. there will be no 0 values (except for end and beginning)) in a logarithmic (semitone, base note 27.5 Hz - a linear F0 equal to and below 29.136 Hz (= 1 on the semitone scale) will be clipped to an output value of 1, since 0 is reserved for unvoiced) frequency scale. Envelope method is sample and hold, no interpolation is performed. [EXPERIMENTAL!]",0);

   ct->setField("no0f0","1 = enable 'no zero F0', output data only when F0>0, i.e. a voiced frame is detected. This may cause problem with some functionals and framer components, which don't support this variable length data yet...",0);

   ct->setField("voicingFinalClipped","1 = Enable output of final smoothed and clipped voicing (pseudo) probability. 'Clipped' means that the voicing probability is set to 0 for unvoiced regions, i.e. where the probability lies below the voicing threshold.",0);
   ct->setField("voicingFinalUnclipped","1 = Enable output of final smoothed, raw voicing (pseudo) probability (UNclipped: not set to 0 during unvoiced regions).",0);

   // data forwarded from pitch detector
   ct->setField("F0raw","1 = Enable output of 'F0raw' copied from input",0);
   ct->setField("voicingC1","1 = Enable output of 'voicingC1' copied from input",0);
   ct->setField("voicingClip","1 = Enable output of 'voicingClip' copied from input",0);

   //viterbi parameters
   ct->setField("wLocal","Viterbi weight for local log. voice probs. A higher weight here will favour candidates with a high voicing probability.",2.0);
   ct->setField("wTvv","Viterbi weight for voiced-voiced transition. A higher weight here will favour a flatter pitch curve (less jumps)",10.0);
   ct->setField("wTvvd","Viterbi weight for smoothness of voiced-voiced transition. A higher weight here will favour a flatter pitch curve (less jumps)",5.0);
   ct->setField("wTvuv","Viterbi cost for voiced-unvoiced transitions. A higher value will reduce the number of voiced-unvoiced transitions.",10.0);
   ct->setField("wThr","Viterbi cost bias for voice prob. crossing the voicing threshold. A higher value here will force voiced/unvoiced decisions by the Viterbi algorithm to be more close to the threshold based decision. A lower value, e.g. 0, will ignore the voicing threshold completely (not recommended).",4.0);
   ct->setField("wRange","Viterbi weight for frequency range constraint. A higher value will enforce the given frequency weighting more strictly, i.e. favour pitch frequencies between 100 Hz and 300 Hz.",1.0);
   ct->setField("wTuu","Viterbi cost for unvoiced-unvoiced transitions. There should be no need to change the default value of 0.",0);

  )
  
  SMILECOMPONENT_MAKEINFO(cPitchSmootherViterbi);
}


SMILECOMPONENT_CREATE(cPitchSmootherViterbi)

//-----

long cSmileViterbi::addFrame(FLOAT_DMEM *frame)
{
  int i,j;

  /* check for free space in the buffer, if no free space, return -1, the user needs to call getNextOutput first */
  if (wrIdx-rdIdx < buflen) {
    /* add the frame to the buffer */
    FLOAT_DMEM * b = buf+((wrIdx%buflen)*frameSize);
    memcpy(b,frame,sizeof(FLOAT_DMEM)*frameSize);
    wrIdx++;
    FLOAT_DMEM * a = prev; prev = b;

    /* perform one step viterbi */

    if (pathIdx == 0 || prev == NULL) {
      pathIdx = 0; convIdx = -1;
      /* trellis initialisation (no transp from previous states) */
      for (i=0; i<nStates; i++) {
        pathCosts[i] = localCost(i,frame);
        paths[pathBuf][i*buflen] = i;
      }
    } else {
      /* trellis updates */
      int pathBufNew = (pathBuf+1)%2;
//printf("XX Cost: ");
      for (i=0; i<nStates; i++) {
        /*  update all the state probabilities for the current timestep */

        /* find best path into current state */
        int minState = 0;
        double minCost;
        minCost = pathCostsTemp[0] = transitionCost(i, 0, a, b) + pathCosts[0];
        for (j=1; j<nStates; j++) {
          pathCostsTemp[j] = transitionCost(i, j, a, b) + pathCosts[j];
          if (pathCostsTemp[j] < minCost) {
            minState = j;
            minCost = pathCostsTemp[j];
          }
        }

        /* update new cost */
        pathCostsNew[i] = minCost + localCost(i,frame);
//printf("%f ",pathCostsNew[i]);
        /* copy and update path */
        memcpy(paths[pathBufNew] + i*buflen, paths[pathBuf] + minState*buflen, buflen*sizeof(int));
        paths[pathBufNew][i*buflen + pathIdx%buflen] = i;
      }
//printf("\n");

      /* copy new costs to pathCosts */
      double * tmp = pathCosts;
      pathCosts = pathCostsNew;
      pathCostsNew = tmp;

      /* swap path buffers */
      pathBuf = pathBufNew;
    }

    pathIdx++;
    if (pathIdx-convIdx > buflen) { /* > ok here, since convIdx starts at -1 */
      /*
       *  forced decision for the first (oldest) element in the path, based upon best final path
       *
       */
      SMILE_MSG(4,"cSmileViterbi: Forced viterbi trellis flush. If you get this message often, increase the 'bufferLength' option.\n");

      /* find current path with minimal cost */
      int minState = 0;
      for (i=1; i<nStates; i++) {
        if (pathCosts[i] < pathCosts[minState]) {
          minState = i;
        }
      }

      convIdx++;
      int x = paths[pathBuf][minState*buflen + (convIdx)%buflen];
      bestPath[convIdx%buflen] = x;

    } else {
      /* find point where all paths merge */
      long n;
      for (n=convIdx+1; n<pathIdx; n++) {
        int x = paths[pathBuf][n%buflen];
        int match = 1;
        for (i=1; i<nStates; i++) {
          if (x != paths[pathBuf][i*buflen+n%buflen]) {
            match = 0;
            break;
          }
        }
        if (!match) { break; }
        else {
          convIdx++;
          bestPath[convIdx%buflen] = x;
        }
        // TODO: shouldn't we choose the best path here and not the first??
      }

    }

    return getNAvail();

  } else {
    return -1;
  }


}

/* return next output value, optionally returns a pointer to the full input frame
 * !this pointer is only valid until the next call to any function in this class!
 */
FLOAT_DMEM cSmileViterbi::getNextOutputFrame(FLOAT_DMEM ** frame=NULL, int *avail=NULL, int *state=NULL)
{
  FLOAT_DMEM result=0.0;

  long av = getNAvail();
  if (avail != 0) *avail = av;

  if (av > 0) {
    /* find best path state */
    int s = bestPath[rdIdx%buflen];
    if (state != NULL) *state = s;

    FLOAT_DMEM * b =  buf+((rdIdx%buflen)*frameSize);
    result = getStateValueFromFrame(s,b);

    /* return full frame */
    if (frame != NULL) {
      *frame = b;
    }

    rdIdx++;
  }

  return result;
}

/* TODO!!!!
 *
 * some pitch candidates may no be valid, so we don't want the viterbi algo's paths to go through them...
 *  a) we fill them with 0's and ensure these will be penalised enough in the trans&local probs.
 *  b) if a candidate is missing for a few frames? interpolate it or not? right now: no... later on, maybe find an intelligent way to determine which one is missing and use the previous value for it...
 */

//-------

cPitchSmootherViterbi::cPitchSmootherViterbi(const char *_name) :
  cDataProcessor(_name), viterbi(NULL), framePtr(NULL), vecO(NULL), buflen(0), outpVecSize(0),
  nInputLevels(1), lastValidf0(0.0), reader2(NULL)
{
  char *tmp = myvprint("%s.reader2",getInstName());
  reader2 = (cDataReader *)(cDataReader::create(tmp));
  if (reader2 == NULL) {
    COMP_ERR("Error creating dataReader '%s'",tmp);
  }
  if (tmp!=NULL) free(tmp);
}

void cPitchSmootherViterbi::myFetchConfig() {
  cDataProcessor::myFetchConfig();
  reader2->fetchConfig();

  // load all configuration parameters you will later require for fast and easy access to here:

  buflen = getInt("bufferLength");
  // note, that it is "polite" to output the loaded parameters at debug level 2:
  SMILE_IDBG(2,"bufferLength = %f",buflen);

  F0final = getInt("F0final");
  SMILE_IDBG(2,"F0final = %i",F0final);
  F0finalLog = getInt("F0finalLog");
  F0finalEnv = getInt("F0finalEnv");
  SMILE_IDBG(2,"F0finalEnv = %i",F0finalEnv);
  F0finalEnvLog = getInt("F0finalEnvLog");
  //no0f0 = getInt("no0f0");
  //SMILE_IDBG(2,"no0f0 = %i (not yet well supported)",no0f0);

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

  wLocal = getDouble("wLocal");
  wTvv = getDouble("wTvv");
  wTvvd = getDouble("wTvvd");
  wTvuv = getDouble("wTvuv");
  wThr = getDouble("wThr");
  wRange = getDouble("wRange");
  wTuu = getDouble("wTuu");

}

/*  This method is rarely used. It is only there to improve readability of component code.
    It is called from cDataProcessor::myFinaliseInstance just before the call to configureWriter.
    I.e. you can do everything that you would do here, also in configureWriter()
    However, if you implement the method, it must return 1 in order for the configure process to continue!
*/

int cPitchSmootherViterbi::configureReader(const sDmLevelConfig &c)
{
  int ret = cDataProcessor::configureReader(c);
  reader2->setBlocksize(buflen); /* this will ensure enough frames in the input level buffer */
  return ret;
}

void cPitchSmootherViterbi::mySetEnvironment()
{
  cDataProcessor::mySetEnvironment();
  reader2->setComponentEnvironment(getCompMan(), -1, this);
}

int cPitchSmootherViterbi::myRegisterInstance(int *runMe)
{
  int ret = cDataProcessor::myRegisterInstance(runMe);
  ret *= reader2->registerInstance();
  return ret;
}

int cPitchSmootherViterbi::myConfigureInstance()
{
  if (!(reader2->configureInstance())) return 0;
  if (!(reader2->finaliseInstance())) return 0;

  int ret = cDataProcessor::myConfigureInstance();
  return ret;
}


int cPitchSmootherViterbi::configureWriter(sDmLevelConfig &c) 
{
  c.blocksizeWriter = buflen;
  c.blocksizeIsSet = 1;
  return 1;
}


/* 
  Use setupNewNames() to freely set the data elements and their names in the output level
  The input names are available at this point, you can get them via reader->getFrameMeta()
  Please set "namesAreSet" to 1, when you do set names
*/

int cPitchSmootherViterbi::setupNewNames(long nEl) 
{
  ///// set-up the output fields

  // final (smoothed) output

  if (F0final) {
    writer_->addField("F0final", 1); outpVecSize++;
  }
  if (F0finalLog) {
    writer_->addField("F0finalLog", 1); outpVecSize++;
  }

  if (F0finalEnv) {
    writer_->addField("F0finEnv", 1); outpVecSize++;
  }
  if (F0finalEnvLog) {
    writer_->addField("F0finEnvLog", 1); outpVecSize++;
  }

  if (voicingFinalClipped) { writer_->addField("voicingFinalClipped", 1); outpVecSize++; }
  if (voicingFinalUnclipped) { writer_->addField("voicingFinalUnclipped", 1); outpVecSize++; }

  // copied from input:
  if (F0raw) { writer_->addField( "F0raw", 1 ); outpVecSize++; }
  if (voicingC1) { writer_->addField( "voicingC1", 1 ); outpVecSize++;}
  if (voicingClip) { writer_->addField( "voicingClip", 1 ); outpVecSize++; }

   long i,idx;
   nInputLevels = reader_->getNLevels();
   if (nInputLevels<1) nInputLevels=1;
   voicingCutoff = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*nInputLevels);
   nCandidates = (long*)calloc(1,sizeof(long)*nInputLevels);
   //nCands = (int*)calloc(1,sizeof(int)*nInputLevels);
   f0candI = (int*)calloc(1,sizeof(int)*nInputLevels);
   candVoiceI = (int*)calloc(1,sizeof(int)*nInputLevels);
   candScoreI = (int*)calloc(1,sizeof(int)*nInputLevels);
   F0rawI = (int*)calloc(1,sizeof(int)*nInputLevels);
   voicingClipI = (int*)calloc(1,sizeof(int)*nInputLevels);
   voicingC1I = (int*)calloc(1,sizeof(int)*nInputLevels);

  // totalCands = 0;
   int more=0; int moreV=0; int moreS=0;
   int moreC1=0; int moreRa=0; int moreVc=0;

   ////// find the input fields by their names...
   for (i=0; i<nInputLevels; i++) {

     cVectorMeta *mdata = reader_->getLevelMetaDataPtr(i);
     if (mdata != NULL) {
       // TODO : check mdata ID
       voicingCutoff[i] = mdata->fData[0];
       SMILE_IMSG(3,"voicing cutoff read from input level %i = %f",i,voicingCutoff[i]);
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

     idx = findField("F0Cand",0,nCandidates+i,NULL,-1,&more);
     f0candI[i] = idx;
//printf("XXX idx %i  ncand[%i]=%i\n",idx,i,nCandidates[i]);
     if (idx >= 0) {
       int idxV = findField("candVoicing",0,NULL,NULL,-1,&moreV);
       candVoiceI[i] = idxV;
       if (moreV>0) { moreV = idxV+1; }
       int idxS = findField("candScore",0,NULL,NULL,-1,&moreS); // TODO: check if nC == N here
       candScoreI[i] = idxS;
       if (moreS>0) { moreS = idxS+1; }
       //nCandidates[i] = nC;
       if (more>0) { more = idx+1; }
       else {
         //totalCands += nC;
         nInputLevels = i+1;
         break;
       }
     } else {
       nCandidates[i] = 0;
       nInputLevels = i;
       break;
     }
     //totalCands += nC;

     //if (nCandidates[0] > 0 && nCandidates[0] < 1000 /* upper limit to prevent out of memory when invalid values for nCandidates are received*/ ) {
// ???
    // }
   }

//printf("XXXX ncand0 = %i , nInputLevels = %i\n",nCandidates[0],reader->getNLevels());

   namesAreSet_ = 1;
   return outpVecSize;
}


int cPitchSmootherViterbi::myFinaliseInstance()
{
  int ret = cDataProcessor::myFinaliseInstance();
  if (ret) {
    /* create viterbi object */
    voiceThresh = voicingCutoff[0];
    viterbi = new cSmileViterbiPitchSmooth(nCandidates[0], buflen, nCandidates[0]*2 + 4, voiceThresh);
    viterbi->setWeights(wLocal, wTvv, wTvvd, wTvuv, wThr, wRange, wTuu);
    framePtr = (FLOAT_DMEM *)malloc(sizeof(FLOAT_DMEM) * (nCandidates[0] * 2 + 4) );
  }
  return ret;
}


eTickResult cPitchSmootherViterbi::myTick(long long t)
{
  /*
   * TODO: pitch range weighting (either on the fly, message based data, or pre-loaded, or standard range (built-in))
   */

  long avail=0;
  long i;

  if (isEOI()) {
    /* flush trellis on EOI */
    viterbi->flushTrellis();
    avail = viterbi->getNAvail();
  } else {
    // get next frame from dataMemory
    cVector *vec = reader_->getNextFrame();
    if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;

    // fill framePtr: candidates first, then 3 values from input..
    for (i=0; i<nCandidates[0]; i++) {
      // TODO : add candidates from other input levels...
      framePtr[i*2] = vec->data[f0candI[0]+i];
      framePtr[i*2+1] = vec->data[candVoiceI[0]+i];
    }
    if (F0rawI[0] >= 0) framePtr[nCandidates[0]*2] = vec->data[F0rawI[0]];
    else framePtr[nCandidates[0]*2] = 0.0;

    if (voicingClipI[0] > 0) framePtr[nCandidates[0]*2+1] = vec->data[voicingClipI[0]];
    else framePtr[nCandidates[0]*2+1] = 0.0;

    if (voicingC1I[0] > 0) framePtr[nCandidates[0]*2+2] = vec->data[voicingC1I[0]];
    else framePtr[nCandidates[0]*2+2] = 0.0;

    /* TODO: store vIdx as long in framePtr... */
    *(framePtr + (nCandidates[0]*2+3)) = (FLOAT_DMEM)vec->tmeta->vIdx;

    avail = viterbi->addFrame(framePtr);
  }

  if (avail == 0) 
    return TICK_INACTIVE;

  if (vecO == NULL) {
    vecO = new cVector(outpVecSize);
  }

  /* read all available output frames and send them to the writer */
  for (i=0; i<avail; i++) {    
    if (!writer_->checkWrite(1)) {
      // return TICK_SUCCESS if we could write at least one frame, otherwise return TICK_DEST_NO_SPACE
      return i == 0 ? TICK_DEST_NO_SPACE : TICK_SUCCESS;
    }

    int av, state;
    FLOAT_DMEM *frameOptr=NULL;
    FLOAT_DMEM f0 = viterbi->getNextOutputFrame(&frameOptr, &av, &state);
    long n=0;
    if (F0final) {
      vecO->data[n++] = f0;
    }
    if (F0finalLog) {
      FLOAT_DMEM f0scaled = 0.0;
      if (f0 > 29.136) {
        // semi-tone scale, base tone 27.5 Hz
        f0scaled = (FLOAT_DMEM)12.0 * log(f0 / (FLOAT_DMEM)27.5) / log((FLOAT_DMEM)2.0);
      } else if (f0 > 0.0) {
        f0scaled = 1.0;
      }
      vecO->data[n++] = f0scaled;
    }
    if (F0finalEnv || F0finalEnvLog) {
      if (f0 <= 0.0) {
        f0 = lastValidf0;
      } else {
        lastValidf0 = f0;
      }
      if (F0finalEnv) {
        vecO->data[n++] = f0;
      }
      if (F0finalEnvLog) {
        FLOAT_DMEM f0scaled = 0.0;
        if (f0 > 29.136) {
          // semi-tone scale, base tone 27.5 Hz
          f0scaled = (FLOAT_DMEM)12.0 * log(f0 / (FLOAT_DMEM)27.5) / log((FLOAT_DMEM)2.0);
        } else if (f0 > 0.0) {
          f0scaled = 1.0;
        }
        vecO->data[n++] = f0scaled;
      }
    }
    FLOAT_DMEM vp = 0.0;
    if (state < nCandidates[0]) { vp = frameOptr[state*2+1]; }
    else { vp = frameOptr[1]; }
    if (voicingFinalClipped) {
      if (vp >= voiceThresh) vecO->data[n++] = vp;
      else vecO->data[n++] = 0.0;
    }

    if (voicingFinalUnclipped) vecO->data[n++] = vp;

    if (F0raw) vecO->data[n++] = frameOptr[nCandidates[0]*2];
    if (voicingC1) vecO->data[n++] = frameOptr[nCandidates[0]*2+1];
    if (voicingClip) vecO->data[n++] = frameOptr[nCandidates[0]*2+2];

    /* read frames from second reader to obtain correct tmeta object */
    long vid = (long)(frameOptr[nCandidates[0]*2+3]);
    cVector *vecIn = reader2->getFrame(vid);
//      printf("XX vid %i\n",vid);

    vecO->setTimeMeta(vecIn->tmeta);
    writer_->setNextFrame(vecO);
  }
  return TICK_SUCCESS;  
}


cPitchSmootherViterbi::~cPitchSmootherViterbi()
{
  // cleanup...
  if (framePtr != NULL) free(framePtr);
  if (viterbi != NULL) delete viterbi;
  if (vecO != NULL) delete vecO;

  if (voicingCutoff != NULL) free(voicingCutoff);
  if (nCandidates != NULL) free(nCandidates);
  if (f0candI != NULL) free(f0candI);
  if (candVoiceI != NULL) free(candVoiceI);
  if (candScoreI != NULL) free(candScoreI);
  if (F0rawI != NULL) free(F0rawI);
  if (voicingClipI != NULL) free(voicingClipI);
  if (voicingC1I != NULL) free(voicingC1I);

  if (reader2 != NULL) delete reader2;
}

