/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example for dataProcessor descendant

*/


#ifndef __CPITCHSMOOTHERVITERBI_HPP
#define __CPITCHSMOOTHERVITERBI_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>

#define BUILD_COMPONENT_PitchSmootherViterbi
#define COMPONENT_DESCRIPTION_CPITCHSMOOTHERVITERBI "Viterbi algorithm to smooth pitch contours and remove octave jumps."
#define COMPONENT_NAME_CPITCHSMOOTHERVITERBI "cPitchSmootherViterbi"

class cSmileViterbi
{
private:
  /* Memory: buflen input data (candidates&scores and (from inp: voicingC1, F0raw, voicingClip)) in ring buffer, NOT full matrix type of full input vector)
   *
   * Memory: n best paths:
   *   n x buflen matrix
   *   n dim array for current costs
   * previous paths will always have to be copied at each step: attention! don't overwrite paths still needed by other nodes!? (-> double buffering with 2 matricies?)
   */
  long wrIdx, rdIdx;
  long buflen; /* length of viterbi workspace / input buffer */
  int nStates; /* number of pitch candidates and scores supplied in input vector */

  int frameSize;
  FLOAT_DMEM * buf; /* pointer to (2 * nCandidates + 3) x buflen  array of FLOAT_DMEM (inputs) */
  FLOAT_DMEM * prev; /* pointer to last frame for quick referencing */

  int pathBuf; /* index of current path buffer (double buffering) */
  long pathIdx; /* index of current position in path buffer */
  long convIdx; /* index where paths merge, will be -1 for no convergence at all */
  int * paths[2]; /* pointers to two  nCandidates x buflen matricies to hold best paths */
  int * bestPath; /* converged (or forced/flushed) best path */
  double * pathCosts; /* array with current path costs, size: nCandidates */
  double * pathCostsNew; /* workspace array for path cost updates, size: nCandidates */
  double * pathCostsTemp; /* workspace array for path cost updates, size: nCandidates */

protected:
  /***** user definable re-factoring hooks *****/
   virtual double localCost(int i, FLOAT_DMEM * frame) /* compute local cost for state i for current frame */
   {
     return getStateValueFromFrame(i,frame);
   }

   /* compute transition cost from state i (previous frame) to state j (current frame) */
   virtual double transitionCost(int i, int j, FLOAT_DMEM * previousFrame, FLOAT_DMEM * currentFrame)
   {
     return 0.0;
   }

   virtual FLOAT_DMEM getStateValueFromFrame(int i, FLOAT_DMEM *frame)
   {
     return frame[i];
   }

   /*****************************/

   int getnStates() { return nStates; }

public:
  cSmileViterbi(int _nStates, long _buflen, int _frameSize) :
    nStates(_nStates), buflen(_buflen), frameSize(_frameSize), wrIdx(0), rdIdx(0), prev(NULL), convIdx(-1)
  {
    /* allocate memory for buffer */
    buf = (FLOAT_DMEM *)malloc(sizeof(FLOAT_DMEM)*(frameSize)*buflen);
    paths[0] = (int *)malloc(sizeof(int)*nStates*buflen);
    paths[1] = (int *)malloc(sizeof(int)*nStates*buflen);
    bestPath = (int *)malloc(sizeof(int)*nStates*buflen);
    pathIdx = 0; pathBuf = 0;
    pathCosts = (double *)calloc(1,sizeof(double)*nStates);
    pathCostsNew = (double *)calloc(1,sizeof(double)*nStates);
    pathCostsTemp = (double *)calloc(1,sizeof(double)*nStates);
  }

  /* add a new data frame (size: frameSize), and compute updated path costs and best paths (perform one step of the incremental viterbi algorithm)
   * returns >0 if new output frames are available (return val = number of output frames available)
   * returns 0 if no output frames are available
   * returns -1 if the input buffer is full and the user must first call getNextOutputFrame
   * */
  long addFrame(FLOAT_DMEM * frame);

  /* get the number of available output frames, where the viterbi algorithm has converged on one path */
  long getNAvail() { return (convIdx+1 - rdIdx); }

  /*
   * flush the trellis, by applying current best path up to current time
   * after calling this function you may read all the data in the buffer with getNextOutputFrame()
   * the function returns the number of frames that can now be read from the buffer
   */
  long flushTrellis() {
    long i;

    /* find current path with minimal cost */
    int minState = 0;
    for (i=1; i<nStates; i++) {
      if (pathCosts[i] < pathCosts[minState]) {
        minState = i;
      }
    }

    /* update best path */
    for (i=convIdx+1; i<pathIdx; i++) {
      convIdx++;
      int x = paths[pathBuf][minState*buflen + (convIdx)%buflen];
      bestPath[convIdx%buflen] = x;
    }

    // TODO: put proper value here...
    return 1;
  }

  /* reset the trellis and the input buffer (you should call flushTrellis() and read all data before that) */
  void resetTrellis() {
    pathIdx = 0;
    convIdx=-1;
    wrIdx=0;
    rdIdx=0;
    pathBuf = 0;
  }

  /* return next output value, optionally returns a pointer to the full input frame
   * !this pointer is only valid until the next call to any function in this class!
   *  */
  FLOAT_DMEM getNextOutputFrame(FLOAT_DMEM ** frame, int *avail, int *state);



  ~cSmileViterbi() {
    if (buf != NULL) free(buf);
    if (paths[0] != NULL) free(paths[0]);
    if (paths[1] != NULL) free(paths[1]);
    if (pathCosts != NULL) free(pathCosts);
    if (bestPath != NULL) free(bestPath);
    if (pathCostsNew != NULL) free(pathCostsNew);
    if (pathCostsTemp != NULL) free(pathCostsTemp);
  }

};


//-----------------------------------

class cSmileViterbiPitchSmooth : public cSmileViterbi
{
private:
  FLOAT_DMEM voiceThresh;
  double wLocal, wTvv, wTvvd, wTvuv, wTuu, wThr, wRange;
  double lastChange;

protected:

  virtual double getFweight(FLOAT_DMEM f) {
    //default built-in weighting, piecewise linear
    /* old:
    if (f>0.0 && f<100.0) {
      return -(1.0/100.0)*f + 1.0;
    } else if (f>=100.0 && f<500.0) {
      return ((1.0/400.0) * f - 0.25);
    } else if (f > 500.0) { return 1.1; } else if (f<=0) { return 2.0; }
     */
    if (f>0.0 && f<100.0) {
      return -(1.0/100.0)*f + 1.0;
    } else if (f>=100.0 && f<350.0) {
      return 0.0;
    } else if (f>=350.0 && f<600.0) {
      return (((f-350.0)/250.0));
    } else
      if (f >= 600.0) { return 1.2; } else if (f<=0) { return 2.0; } // penalty for out of bound frequencies

    /*
    if (f>0.0 && f<90.0) {
      return -(0.8/90.0)*f + 1.0;
    } else if (f>=90.0 && f<200) {
      return (- (0.2/110.0) * f + (40.0/110.0));
    } else if (f>=200 && f < 350) {
      return ((0.2/150.0)*f - 4.0/15.0);
    } else if (f>=350 && f < 600) {
      return ((0.8/250.0)*f - 4.0/15.0);
    } else if (f > 600.0) { return 1.0; } else if (f<=0) { return 2.0; }
    */
      return 0.0;
  }

  /* frame structure:
   * [f0, voiceprob0, f1, voiceprob1, ... , fn, voiceprobn]
   */
  virtual double localCost(int i, FLOAT_DMEM * frame) /* compute local cost for state i for current frame */
  {
    double pv = (double)frame[i*2+1];
    double thr = 0.0;
    if (pv < 0.01) pv=0.01;
    if (pv > 1.00) pv=1.00;
    if (pv < voiceThresh) thr = wThr;
    if (i<getnStates()-1) {
      // TODO: speaker dependent frequency range weighting
      double fWght = getFweight(frame[i*2]); /* frequency range weight */
      return (-log(pv) + thr)*wLocal + fWght * wRange;
    }
    else {
      int j; double flag = 0.0;
      for (j=0; j < getnStates(); j++) {
        if (frame[j*2+1] >= voiceThresh) { flag = wThr; break; }
      }
      return wLocal*(double)flag;
    }
  }

  /* compute transition cost from state i (previous frame) to state j (current frame) */
  virtual double transitionCost(int i, int j, FLOAT_DMEM * previousFrame, FLOAT_DMEM * currentFrame)
  {
    //FLOAT_DMEM pv0 = previousFrame[j*2+1];
    //FLOAT_DMEM pv1 = currentFrame[i*2+1];
    /* determine transition type */
    if (i == j == getnStates()-1) { /* u->u */
      return wTuu;
    }
    if (i < getnStates()-1 && j < getnStates()-1) { /* v->v */
      FLOAT_DMEM f0 = previousFrame[j*2];
      FLOAT_DMEM f1 = currentFrame[i*2];
      if (f0 == 0 || f1 == 0) return 999.0 ; //wTvv*wTvuv; /* high cost, so we don't choose empty candidates */
      else {
        double r = log((double)(f1/f0));
        double x = wTvv * fabs(r)  + wTvvd * fabs( r - lastChange ) ;  /* novel delta continuity constraint here.. */
        lastChange = r;
        /* FIXME: the lastChange will always reflect the lastChange of the previous path combination and not the last timestep!
         * it should rather be the lastchange on the most likely path previously
         * OR: easier to implement, the minimum (non-null - if any - otherwise null) lastchange of the previous timestep!
         * */
        return x;
      }
    }
    if ((i==getnStates()-1 && j < getnStates()-1)||(i < getnStates()-1 && j==getnStates()-1))  { /* v->u , u->v */
      lastChange=0.0;
      return wTvuv;
    }
    // should not happen... just in case... (add some penalty to this unknown path...)
    return 1.0;

#if 0
    //???
    if (pv0 < voiceThresh && pv1 < voiceThresh) { /* u->u */
      return wTuu;
    }
    if (pv0 >= voiceThresh && pv1 >= voiceThresh) { /* v->v */
      FLOAT_DMEM f0 = previousFrame[j*2];
      FLOAT_DMEM f1 = currentFrame[i*2];
      return fabs(log());
    }
    if ((pv0 < voiceThresh && pv1 >= voiceThresh)||(pv0 >= voiceThresh && pv1 < voiceThresh))  { /* u->v , v->u */
      return wTvuv;
    }
#endif

  }

  virtual FLOAT_DMEM getStateValueFromFrame(int i, FLOAT_DMEM *frame)
  {
    if (i<getnStates()-1)
      return frame[i*2];
    else
      return 0.0;
  }

public:

  cSmileViterbiPitchSmooth(int _nCandidates, long _buflen, int _frameSize, FLOAT_DMEM thresh=0.5) :
     cSmileViterbi(_nCandidates+1, _buflen, _frameSize),
     wLocal(2.0), wTvv(20.0), wTvvd(0.0), wTvuv(10.0), wTuu(0.0), wThr(5.0), wRange(5.0),
     voiceThresh(thresh), lastChange(1.0)
   {
   }

   /* set the viterbi pitch and voicing scaling weights. Defaults are:
    * wLocal(2.0), wTvv(20.0), wTvuv(10.0), wTuu(0.0), wThr(5.0)
    */
   void setWeights(double local=2.0, double tvv=20.0, double tvvd=20.0, double tvuv=10.0, double thr=5.0, double range=5.0, double tuu=0) {
     wLocal = local;
     wTvv = tvv;
     wTvvd = tvv;
     wTvuv = tvuv;
     wThr = thr;
     wRange=range;
     wTuu = tuu;
   }

};


//-------------------------------

class cPitchSmootherViterbi : public cDataProcessor {
  private:
    cSmileViterbiPitchSmooth *viterbi;
    FLOAT_DMEM *framePtr;
    cVector *vecO;
    FLOAT_DMEM voiceThresh;
    long buflen;
    int outpVecSize;
    int nInputLevels;
    FLOAT_DMEM lastValidf0;

    int F0finalLog, F0final, F0finalEnv, F0finalEnvLog;
    int voicingFinalClipped, voicingFinalUnclipped;
    int F0raw, voicingC1, voicingClip;

    FLOAT_DMEM *voicingCutoff;
    long *nCandidates; // array holding max. number of candidates for each pda; array of size nInputLevels;
    // index lookup:
    int *f0candI, *candVoiceI, *candScoreI;  // array of size nInputLevels;
    int *F0rawI, *voicingClipI, *voicingC1I;

    double wLocal, wTvv, wTvvd, wTvuv, wTuu, wThr, wRange;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    cDataReader * reader2;

    virtual void myFetchConfig() override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int setupNewNames(long nEl) override;
    virtual int configureReader(const sDmLevelConfig &c) override;
    virtual void mySetEnvironment() override;
    virtual int myRegisterInstance(int *runMe) override;
    virtual int configureWriter(sDmLevelConfig &c) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cPitchSmootherViterbi(const char *_name);

    virtual ~cPitchSmootherViterbi();
};




#endif // __CPITCHSMOOTHERVITERBI_HPP
