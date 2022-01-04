/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: number of peaks and various measures associated with peaks

*/


#include <functionals/functionalPeaks.hpp>

#define MODULE "cFunctionalPeaks"


#define FUNCT_NUMPEAKS         0  // number of peaks
#define FUNCT_MEANPEAKDIST     1  // mean distance between peaks
#define FUNCT_PEAKMEAN         2  // arithmetic mean of peaks
#define FUNCT_PEAKMEANMEANDIST 3  // aritmetic mean of peaks - aritmetic mean
#define FUNCT_PEAKDISTSTDDEV   4  // standard deviation of inter peak distances

#define N_FUNCTS  5

#define NAMES     "numPeaks","meanPeakDist","peakMean","peakMeanMeanDist","peakDistStddev"

#define PEAKDIST_BLOCKALLOC 50

const char *peaksNames[] = {NAMES};  

SMILECOMPONENT_STATICS(cFunctionalPeaks)

SMILECOMPONENT_REGCOMP(cFunctionalPeaks)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFUNCTIONALPEAKS;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALPEAKS;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("numPeaks","1/0=enable/disable output of number of peaks [output name: numPeaks]",1);
    ct->setField("meanPeakDist","1/0=enable/disable output of mean distance between peaks (relative to the input segment length, in seconds, or in frames, see the 'norm' option or the 'masterTimeNorm' option of the cFunctionals parent component) [output name: meanPeakDist]",1);
    ct->setField("peakMean","1/0=enable/disable output of arithmetic mean of peaks [output name: peakMean]",1);
    ct->setField("peakMeanMeanDist","1/0=enable/disable output of arithmetic mean of peaks - arithmetic mean of all values [output name: peakMeanMeanDist]",1);
    ct->setField("peakDistStddev","1/0=enable/disable output of standard deviation of inter peak distances [output name: peakDistStddev]",0);
    ct->setField("norm","This option specifies how this component should normalise times (if it generates output values related to durations): \n   'segment' (or: 'turn') : normalise to the range 0..1, the result is the relative length wrt. to the segment length )\n   'second'  (absolute time in seconds) \n   'frame' (absolute time in number of frames of input level)","frames");
    ct->setField("overlapFlag","1/0=yes/no frames overlap (i.e. compute peaks locally only)",1,0,0);
    //TODO: unified time norm handling for all functionals components: frame, seconds, turn !
  )

  SMILECOMPONENT_MAKEINFO_NODMEM(cFunctionalPeaks);
}

SMILECOMPONENT_CREATE(cFunctionalPeaks)

//-----

cFunctionalPeaks::cFunctionalPeaks(const char *name) :
  cFunctionalComponent(name, N_FUNCTS, peaksNames),
  lastlastVal(0.0),
  lastVal(0.0),
  overlapFlag(0),
  peakdists(NULL), nPeakdists(0)
{
}

void cFunctionalPeaks::myFetchConfig()
{
  parseTimeNormOption();

  if (getInt("numPeaks")) enab[FUNCT_NUMPEAKS] = 1;
  if (getInt("meanPeakDist")) enab[FUNCT_MEANPEAKDIST] = 1;
  if (getInt("peakMean")) enab[FUNCT_PEAKMEAN] = 1;
  if (getInt("peakMeanMeanDist")) enab[FUNCT_PEAKMEANMEANDIST] = 1;
  if (getInt("peakDistStddev")) enab[FUNCT_PEAKDISTSTDDEV] = 1;
  
  overlapFlag = getInt("overlapFlag");

  cFunctionalComponent::myFetchConfig();
}

void cFunctionalPeaks::addPeakDist(int idx, long dist)
{
  if (peakdists == NULL) {
    peakdists = (long*)calloc(1,sizeof(long)*(idx+PEAKDIST_BLOCKALLOC));
    nPeakdists = idx+PEAKDIST_BLOCKALLOC;
  } else if (idx >= nPeakdists) {
    peakdists = (long*)crealloc(peakdists,sizeof(long)*(idx+PEAKDIST_BLOCKALLOC),sizeof(long)*nPeakdists);
    nPeakdists = idx+PEAKDIST_BLOCKALLOC;
  }
  peakdists[idx] = dist;
}

long cFunctionalPeaks::process(FLOAT_DMEM *in,
    FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout)
{
  int i;
  if ((Nin>0)&&(out!=NULL)) {
    FLOAT_DMEM max = *in;
    FLOAT_DMEM min = *in;
    FLOAT_DMEM mean = *in;

    FLOAT_DMEM peakDist = (FLOAT_DMEM)0.0;
    long nPeakDist = 0;
    FLOAT_DMEM peakMean = (FLOAT_DMEM)0.0;
    long nPeaks = 0;

    FLOAT_DMEM lastMin=(FLOAT_DMEM)0.0; // in[0];
    FLOAT_DMEM lastMax=(FLOAT_DMEM)0.0; //in[0];
    long curmaxPos=0, lastmaxPos=-1;

    int peakflag = 0;  

    // range, min, max, mean
    for (i=1; i<Nin; i++) {
      if (in[i]<min) min=in[i];
      if (in[i]>max) max=in[i];
      mean += in[i];
    }
    mean /= (FLOAT_DMEM)Nin;
    FLOAT_DMEM range = max-min;

    if (overlapFlag) i=2;
    else i=0;

    if (overlapFlag) {
      lastlastVal=in[0];
      lastVal=in[1];
    }

    // advanced peak detection
    for (; i<Nin; i++) {
      // first, find ALL peaks:
      if ((lastlastVal < lastVal)&&(lastVal > in[i])) { // max
        if (!peakflag) lastMax = in[i];
        else { if (in[i] > lastMax) { lastMax = in[i]; curmaxPos = i; } }

        if (lastMax - lastMin > 0.11*range) { 
          peakflag = 1; curmaxPos = i;
        }

      } else {
        if ((lastlastVal > lastVal)&&(lastVal < in[i])) { // min
          lastMin = in[i];
        } 
      }

      // detect peak only, if x[i] falls below lastmax - 0.09*range
      if ((peakflag)&&( (in[i] < lastMax-0.09*range) || (i==Nin-1)) ) {
        nPeaks++;
        peakMean += lastMax;
        if (lastmaxPos >= 0) {
          FLOAT_DMEM dist = (FLOAT_DMEM)(curmaxPos-lastmaxPos);
          peakDist += dist;
          // add dist to list for variance computation
          addPeakDist(nPeakDist,(long)dist);
          nPeakDist++;
        }
        lastmaxPos = curmaxPos;
        peakflag = 0;
      }

      lastlastVal = lastVal;
      lastVal = in[i];
    }


    FLOAT_DMEM stddev = 0.0;

    if (nPeakDist > 0.0) {
      peakDist /= (FLOAT_DMEM)nPeakDist;
      // compute peak distance variance / standard deviation
      for (i=0; i<nPeakDist; i++) {
        stddev += (peakdists[i]-peakDist)*(peakdists[i]-peakDist);
      }
      stddev /= (FLOAT_DMEM)nPeakDist;
      stddev = sqrt(stddev);
    } else {
      peakDist = (FLOAT_DMEM)(Nin+1);
      stddev = 0.0;
    }


    int n=0;
    if (enab[FUNCT_NUMPEAKS]) out[n++]=(FLOAT_DMEM)nPeaks;

    if (timeNorm==TIMENORM_SECONDS) {
      peakDist *= (FLOAT_DMEM)getInputPeriod();
      stddev *= (FLOAT_DMEM)getInputPeriod();
    } else if (timeNorm==TIMENORM_SEGMENT) {
      peakDist /= (FLOAT_DMEM)Nin;
      stddev /= (FLOAT_DMEM)Nin;
    }
    if (enab[FUNCT_MEANPEAKDIST]) out[n++]=peakDist;

    if (nPeaks > 0.0) {
      peakMean /= (FLOAT_DMEM)nPeaks;
    } else {
      peakMean = (FLOAT_DMEM)0.0;
    }
    if (enab[FUNCT_PEAKMEAN]) out[n++] = peakMean;

    if (enab[FUNCT_PEAKMEANMEANDIST]) out[n++]=peakMean-mean;

    if (enab[FUNCT_PEAKDISTSTDDEV]) out[n++]=stddev;

    return n;
  }
  return 0;
}

cFunctionalPeaks::~cFunctionalPeaks()
{
}

