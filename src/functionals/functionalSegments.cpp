/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: number of segments based on delta thresholding

*/

// TODO: support turnFrameTime message based upon detected segments, in order to apply functionals to voiced segments,
// for example. We then only need to support second round processing of functionals that aggregate the functionals of
// the segments in case of frameMode=full for the aggregation and the segments code.
// For the frameMode = var method we might need to ensure that segments (or at least the status)
// at segment boundaries is handled correct.

#include <functionals/functionalSegments.hpp>

#define MODULE "cFunctionalSegments"


#define FUNCT_NUMSEGMENTS      0  // number of segments (relative to maxNumSeg)
#define FUNCT_SEGMEANLEN       1  // mean length of segment (relative to input length)
#define FUNCT_SEGMAXLEN        2  // max length of segment (relative to input length)
#define FUNCT_SEGMINLEN        3  // min length of segment (relative to input length)
#define FUNCT_SEGLENSTDDEV     4  // standard deviation of the length of segments (relative to input length)
/// TODO: gap statistics, for segmentationAlgorithms *X (except chX)
#define FUNCT_GAPMEANLEN       5  // mean length of segment (relative to input length)
#define FUNCT_GAPMAXLEN        6  // max length of segment (relative to input length)
#define FUNCT_GAPMINLEN        7  // min length of segment (relative to input length)
#define FUNCT_GAPLENSTDDEV     8  // standard deviation of the length of segments (relative to input length)

#define N_FUNCTS  9

#define NAMES     "numSegments","meanSegLen","maxSegLen","minSegLen","segLenStddev","meanGapLen","maxGapLen","minGapLen","gapLenStddev"

const char *segmentsNames[] = {NAMES};  

SMILECOMPONENT_STATICS(cFunctionalSegments)

SMILECOMPONENT_REGCOMP(cFunctionalSegments)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFUNCTIONALSEGMENTS;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALSEGMENTS;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("maxNumSeg","Maximum number of segments to detect. Use growDynSegBuffer=1 to automatically increase this size if more segments are detected. Use norm=seconds or norm=frames in this time to avoid having inconsistent results.", 20);
    ct->setField("segmentationAlgorithm","Method to use for segmenting the input contour. Possible values are:\n  delta : new segments start when the signal changes more than 'rangeRelThreshold' when the current frame is compared to a running average computed over a length of ravgLng = Nin/(maxNumSeg/2) frames (Nin is the length of the input contour in frames).\n  (m)(NA)relTh : segment boundaries each time the short term running average (over 3 frames) of the signal rises above predefined relative (to the signal range) value thresholds (NA version: don't use running average, use signal directly instead; m version: relative thresholds are relative to the arithmetic mean).\n  (NA)absTh : segment boundaries each time the short-time running average (3 frames) of the signal rises above predefined absolute value thresholds (NA version: don't use running average, use signal values directly instead).\n  chX : segments are regions of continuous input samples of value X and continuous segments of non-X samples, i.e. segment boundaries are at changes from X to non-X values.\n  nonX : segment boundaries are at changes from X to non-X, but only non-X value sequences are considered as segments.\n  eqX : segment boundaries are at changes from X to non-X, but only equal to X value sequences are considered as segments.\n  ltX : segment boundaries are at changes from greater equal X to smaller X, but only smaller X sequences are considered as segments.\n  gtX :  segment boundaries are at changes from smaller equal X to greater X, but only greater X sequences are considered as segments.\n  geqX :  segment boundaries are at changes from greater equal X to smaller X, but only greater equal X sequences are considered as segments.\n  leqX : segment boundaries are at changes from smaller equal X to greater X, but only smaller equal X sequences are considered as segments.\n ","delta");
    ct->setField("ravgLng","If set to a value > 0, forces the length of the running average window to this value (for the delta thresholding method).",0);
    ct->setField("thresholds","An array of thresholds, used if 'segmentationAlgorithm' is set to either 'relTh' or 'absTh'. The values specified here are then either relative thresholds (relative to the range of the input), or absolute value thresholds.",0,ARRAY_TYPE);
    ct->setField("X","The value of X for the 'chX','nonX','eqX','ltX','gtX','geqX', and 'leqX' segmentation methods/algorithms.",0);
    ct->setField("XisRel","1= X is a threshold relative to the range of the input / 0= X is an absolute valued threshold.",0);
	  ct->setField("rangeRelThreshold","The segment threshold relative to the signal's range (max-min), when 'segmentationAlgorithm' is set to 'delta'.",0.2);
    ct->setField("numSegments","1/0=enable/disable output of the number of segments (output is relative to maxNumSeg if norm=segment or the absolute value if norm=frames, and for norm=seconds the output is the number of segments per second)",0);
    ct->setField("meanSegLen","1/0=enable/disable output of the mean segment length (relative to the input segment length, in seconds, or in frames, see the 'norm' option or the 'masterTimeNorm' option of the cFunctionals parent component)",0);
    ct->setField("maxSegLen","1/0=enable/disable output of the maximum segment length (relative to the input segment length, in seconds, or in frames, see the 'norm' option or the 'masterTimeNorm' option of the cFunctionals parent component)",0);
    ct->setField("minSegLen","1/0=enable/disable output of the minimum segment length (relative to the input segment length, in seconds, or in frames, see the 'norm' option or the 'masterTimeNorm' option of the cFunctionals parent component)",0);
    ct->setField("segLenStddev","1/0=enable/disable output of the standard deviation of the segment lengths (relative to the input segment length, in seconds, or in frames, see the 'norm' option or the 'masterTimeNorm' option of the cFunctionals parent component)",0);
    ct->setField("norm","This option specifies how this component should normalise times (if it generates output values related to durations): \n   'segment' (or: 'turn') : normalise to the range 0..1, the result is the relative length wrt. to the segment length )\n   'second'  (absolute time in seconds) \n   'frame' (absolute time in number of frames of input level)","segment",0,0);
    ct->setField("dbgPrint","1= enable debug output with segment boundary begin and end coordinates",0);
    ct->setField("segMinLng", "Minimum segment length in input level frames. The segmentation algorithms EqX, NonX, and ChX always use this value. The old (buggy) versions of these algorithms (if useOldBuggyChX = 1), never use this value, they instead compute it as InputLength/maxNumSeg - 1. All other algorithms compute the value segMinLng as inputLength/maxNumSeg if this option is not set explicitly, otherwise they use the value this option is set to (in this case it overrides maxNumSeg, i.e. the maximum number of segments that can be detected might differ from the value maxNumSeg is set to).", 3);
    ct->setField("pauseMinLng", "Minimum length of a pause for the segmentation algorithms EqX and NonX in input level frames.", 2);
    ct->setField("useOldBuggyChX", "1 = Use old buggy version of the ChX, EqX and NonX code (configs up to 12.06.2012); available only for compatibility. Do not use in new configs!", 0);
    ct->setField("growDynSegBuffer", "1 = Dynamically grow the segment buffer (i.e. maxNumSeg = infinite) by maxNumSeg segments at a time if more segments are detected.", 0);
  ) 
  FLOAT_DMEM meanFallingSlope = (FLOAT_DMEM)0.0; 
  FLOAT_DMEM minRisingSlope = (FLOAT_DMEM)0.0;
  FLOAT_DMEM maxRisingSlope = (FLOAT_DMEM)0.0;
  FLOAT_DMEM minFallingSlope = (FLOAT_DMEM)0.0;
  FLOAT_DMEM maxFallingSlope = (FLOAT_DMEM)0.0;
  FLOAT_DMEM stddevRisingSlope = (FLOAT_DMEM)0.0;
  FLOAT_DMEM stddevFallingSlope = (FLOAT_DMEM)0.0;

  SMILECOMPONENT_MAKEINFO_NODMEM(cFunctionalSegments);
}

SMILECOMPONENT_CREATE(cFunctionalSegments)

//-----

cFunctionalSegments::cFunctionalSegments(const char *name) :
  cFunctionalComponent(name, N_FUNCTS, segmentsNames),
  maxNumSeg(20), thresholds(NULL), thresholdsTemp(NULL), dbgPrint(0)
{
}

void cFunctionalSegments::myFetchConfig()
{
  parseTimeNormOption();

  if (getInt("numSegments")) enab[FUNCT_NUMSEGMENTS] = 1;
  if (getInt("meanSegLen")) enab[FUNCT_SEGMEANLEN] = 1;
  if (getInt("maxSegLen")) enab[FUNCT_SEGMAXLEN] = 1;
  if (getInt("minSegLen")) enab[FUNCT_SEGMINLEN] = 1;
  if (getInt("segLenStddev")) enab[FUNCT_SEGLENSTDDEV] = 1;

  dbgPrint = getInt("dbgPrint");
  manualRavgLng = getInt("ravgLng");

  growDynSegBuffer = getInt("growDynSegBuffer");
  if (growDynSegBuffer && timeNorm == TIMENORM_SEGMENT && enab[FUNCT_NUMSEGMENTS] == 1) {
    SMILE_IWRN(1, "growDynSegBuffer=1 and time norm=segment (which means that numSegments is normalised to maxNumSeg, which - however - will grow dynamically, so the outputs will not be consistent - use a different time norm to solve this (seconds or frames))");
  }
  maxNumSeg = getInt("maxNumSeg");
  maxNumSeg0 = maxNumSeg;
  SMILE_IDBG(2,"maxNumSeg = %i",maxNumSeg);

  rangeRelThreshold = (FLOAT_DMEM)getDouble("rangeRelThreshold");
  SMILE_IDBG(2,"rangeRelThreshold = %f",rangeRelThreshold);

  const char * segmentationAlgorithmStr = getStr("segmentationAlgorithm");
  SMILE_IDBG(2,"segmentationAlgorithm = '%s'",segmentationAlgorithmStr);
  if (!strncmp(segmentationAlgorithmStr,"delta",5)) {
    segmentationAlgorithm = SEG_DELTA;
  } else if (!strncmp(segmentationAlgorithmStr,"delt2",5)) {
    segmentationAlgorithm = SEG_DELTA2;
  } else if (!strncmp(segmentationAlgorithmStr,"relTh",5)) {
    segmentationAlgorithm = SEG_RELTH;
  } else if (!strncmp(segmentationAlgorithmStr,"mrelTh",6)) {
    segmentationAlgorithm = SEG_MRELTH;
  } else if (!strncmp(segmentationAlgorithmStr,"absTh",5)) {
    segmentationAlgorithm = SEG_ABSTH;
  } else if (!strncmp(segmentationAlgorithmStr,"NArelTh",7)) {
    segmentationAlgorithm = SEG_RELTH_NOAVG;
  } else if (!strncmp(segmentationAlgorithmStr,"mNArelTh",8)) {
    segmentationAlgorithm = SEG_MRELTH_NOAVG;
  } else if (!strncmp(segmentationAlgorithmStr,"NAmrelTh",8)) {
    segmentationAlgorithm = SEG_MRELTH_NOAVG;
  } else if (!strncmp(segmentationAlgorithmStr,"NAabsTh",7)) {
    segmentationAlgorithm = SEG_ABSTH_NOAVG;
  } else if (!strncmp(segmentationAlgorithmStr,"chX",3)) {
    segmentationAlgorithm = SEG_CHX;
  } else if (!strncmp(segmentationAlgorithmStr,"nonX",4)) {
    segmentationAlgorithm = SEG_NONX;
  } else if (!strncmp(segmentationAlgorithmStr,"eqX",3)) {
    segmentationAlgorithm = SEG_EQX;
  } else if (!strncmp(segmentationAlgorithmStr,"ltX",3)) {
    segmentationAlgorithm = SEG_LTX;
  } else if (!strncmp(segmentationAlgorithmStr,"gtX",3)) {
    segmentationAlgorithm = SEG_GTX;
  } else if (!strncmp(segmentationAlgorithmStr,"geqX",3)) {
    segmentationAlgorithm = SEG_GEQX;
  } else if (!strncmp(segmentationAlgorithmStr,"leqX",3)) {
    segmentationAlgorithm = SEG_LEQX;
  } else {
    SMILE_IERR(1,"unknown segmentationAlgorithm : '%s' (or algorithm not yet implemented) , see '-H cFunctionalSegments' for a list of available methods. Using 'delta' threshold method as fallback default.",segmentationAlgorithmStr)
    segmentationAlgorithm = SEG_DELTA;
  }

  useOldBuggyChX = (int)getInt("useOldBuggyChX");
  if (useOldBuggyChX) {
    SMILE_IDBG(2, "Note: old (broken and buggy) version of ChX, EqX and NonX segmentation methods are enabled! Use only for compatibility with old configs/models!");
  }

  segMinLng = getInt("segMinLng");
  if (segMinLng < 1) segMinLng = 1;
  if (isSet("segMinLng")) {
    SMILE_IDBG(2, "Using segMinLng = %i\n", segMinLng);
    autoSegMinLng = 0;
  } else {
    autoSegMinLng = 1;
  }
  pauseMinLng = getInt("pauseMinLng");
  if (pauseMinLng < 1) pauseMinLng = 1;

  X = (FLOAT_DMEM)getDouble("X");
  SMILE_IDBG(2,"X = %f",X);

  XisRel = (int)getInt("XisRel");
  SMILE_IDBG(2,"XisRel = %i",XisRel);

  if ((segmentationAlgorithm == SEG_RELTH)||(segmentationAlgorithm == SEG_RELTH_NOAVG)||(segmentationAlgorithm == SEG_ABSTH_NOAVG)||
      (segmentationAlgorithm == SEG_MRELTH)||(segmentationAlgorithm == SEG_MRELTH_NOAVG)) {
    // read thresholds array
    nThresholds = getArraySize("thresholds");
    int i;
    thresholds = (FLOAT_DMEM*) calloc(1,sizeof(FLOAT_DMEM)*nThresholds);
    if ((segmentationAlgorithm == SEG_RELTH)||(segmentationAlgorithm == SEG_MRELTH)||(segmentationAlgorithm == SEG_MRELTH_NOAVG)||(segmentationAlgorithm == SEG_RELTH_NOAVG))
        thresholdsTemp = (FLOAT_DMEM*) calloc(1,sizeof(FLOAT_DMEM)*nThresholds);
    else thresholdsTemp = thresholds;
    for (i=0; i<nThresholds; i++) {
      thresholds[i] = (FLOAT_DMEM)getDouble_f(myvprint("thresholds[%i]",i));
      if ((segmentationAlgorithm == SEG_RELTH)||(segmentationAlgorithm == SEG_RELTH_NOAVG)) { // check threshold range 0..1
        if (thresholds[i] < 0) {
          thresholds[i] = 0.0;
          SMILE_IERR(1,"Relative threshold cannot be smaller than 0. It must be in the range 0..1. Setting threshold to 0.0.");
        }
        else if (thresholds[i] > 1) {
          thresholds[i] = 1.0;
          SMILE_IERR(1,"Relative threshold cannot be greater than 1. It must be in the range 0..1. Setting threshold to 1.0.");
        }
      }
    }
  }
  cFunctionalComponent::myFetchConfig();
}

long cFunctionalSegments::addNewSegment(long i, long lastSeg, sSegData *result)
{
  long segLen = i-lastSeg;
  if (growDynSegBuffer && result->nSegments >= maxNumSeg) {
    result->segLens = (long *)crealloc(result->segLens,
        (maxNumSeg+maxNumSeg0)*sizeof(long), maxNumSeg*sizeof(long));
    SMILE_IMSG(3, "increasing maxNumSeg from %i to %i (growDynSegBuffer = 1).",
        maxNumSeg, maxNumSeg + maxNumSeg0);
    maxNumSeg += maxNumSeg0;
  }
  if (result->nSegments < maxNumSeg) {
    result->meanSegLen += segLen;
    result->segLens[result->nSegments] = segLen;
    result->nSegments++;
    if (segLen > result->maxSegLen) result->maxSegLen = segLen;
    if ((result->minSegLen==0)||(segLen<result->minSegLen)) result->minSegLen = segLen;
  } else {
    SMILE_IWRN(3, "Maximum number of segments (%i) reached (frame %ld). Final result will be inaccurate for this instance.", maxNumSeg, i);
  }
  return i;
}

int cFunctionalSegments::process_SegDelta(FLOAT_DMEM *in,
    FLOAT_DMEM *out, long Nin, long Nout, sSegData *result)
{
   FLOAT_DMEM segThresh = result->range * rangeRelThreshold;

   if (autoSegMinLng) {
     segMinLng = Nin/maxNumSeg-1;
     if (segMinLng < 2) segMinLng = 2;
   }
   long ravgLng;

   if (manualRavgLng > 0) ravgLng = manualRavgLng;
   else ravgLng = Nin/(maxNumSeg/2);

   long lastSeg = -segMinLng/2;

   long i;
   FLOAT_DMEM ravg = 0.0;

   for (i=0; i<Nin; i++) {
     ravg += in[i];
     if (i>=ravgLng) ravg -= in[i-ravgLng];

     FLOAT_DMEM ravgLngCur = (FLOAT_DMEM)( MIN(i+1,ravgLng) ); // bugfix by feyben: i+1 to avoid division by zero
     FLOAT_DMEM ra = ravg / ravgLngCur;

     if ((in[i]-ra > segThresh)&&(i - lastSeg > segMinLng) )
     { // found new segment begin
       lastSeg = addNewSegment(i, lastSeg, result);
       if (dbgPrint) {
         printf("XXXX_SEG_border: x=%ld y=%f\n",i,in[i]);
       }
     }

   }

   return 1;
}

int cFunctionalSegments::process_SegDelta2(FLOAT_DMEM *in,
    FLOAT_DMEM *out, long Nin, long Nout, sSegData *result)
{
   FLOAT_DMEM segThresh = result->range * rangeRelThreshold;

   if (autoSegMinLng) {
     segMinLng = Nin/maxNumSeg-1;
     if (segMinLng < 2) segMinLng = 2;
   }
   long ravgLng;

   if (manualRavgLng > 0) ravgLng = manualRavgLng;
   else ravgLng = Nin/(maxNumSeg/2);

   long lastSeg = -segMinLng/2;

   long i;
   FLOAT_DMEM ravg = in[0];
   FLOAT_DMEM raLast = 0.0;

   for (i=1; i<Nin; i++) {
     ravg += in[i];
     if (i>=ravgLng) ravg -= in[i-ravgLng];

     FLOAT_DMEM ravgLngCur = (FLOAT_DMEM)( MIN(i+1,ravgLng) ); // bugfix by feyben: i+1 to avoid division by zero
     FLOAT_DMEM ra = ravg / ravgLngCur;

     if (dbgPrint) { printf("X_RA: %f\n",ra); }

     if ((in[i-1]-raLast <= segThresh)&&(in[i]-ra > segThresh)&&(i - lastSeg > segMinLng) )
     { // found new segment begin
       lastSeg = addNewSegment(i, lastSeg, result);
       if (dbgPrint) {
         printf("XXXX_SEG_border: x=%ld y=%f\n",i,in[i]);
       }
     }
     raLast = ra;

   }

   return 1;
}

int cFunctionalSegments::process_SegThresh(FLOAT_DMEM *in,
    FLOAT_DMEM *out, long Nin, long Nout, sSegData *result)
{
  long i,j;
  if (segmentationAlgorithm == SEG_RELTH) {
    // convert relative thresholds to absolute, given the range
    for (i=0; i<nThresholds; i++) {
      thresholdsTemp[i] = result->min + result->range * thresholds[i];
      //SMILE_IMSG(3,"thresh[%i]=%f",i,thresholdsTemp[i]);
    }
  }
  else if (segmentationAlgorithm == SEG_MRELTH) {
    // convert relative thresholds to absolute, given the mean
    for (i=0; i<nThresholds; i++) {
      thresholdsTemp[i] = result->mean * thresholds[i];
      //SMILE_IMSG(3,"thresh[%i]=%f",i,thresholdsTemp[i]);
    }
  }

  if (autoSegMinLng) {
    segMinLng = Nin/maxNumSeg-1;
    if (segMinLng < 2) segMinLng = 2;
  }

  long ravgLng = 3;
  long lastSeg = -segMinLng/2;

  FLOAT_DMEM ravg = 0.0;
  FLOAT_DMEM raLast = 0.0;

  for (i=0; i<Nin; i++) {
     ravg += in[i];
     if (i>=ravgLng) ravg -= in[i-ravgLng];

     FLOAT_DMEM ravgLngCur = (FLOAT_DMEM)( MIN(i+1,ravgLng) );
     FLOAT_DMEM ra = ravg / ravgLngCur;

     int threshCross=0;
     for (j=0; j<nThresholds; j++) {
       if ((ra > thresholdsTemp[j] && raLast <= thresholdsTemp[j]) || (ra < thresholdsTemp[j] && raLast >= thresholdsTemp[j])) {
         threshCross = 1;
         //printf("xxx: tc  ra=%f  raLast=%f\n",ra,raLast);
       }
     }
     raLast = ra;
     //printf("X_RA: %f\n",ra);

     if ((threshCross)&&(i - lastSeg > segMinLng) )
     { // found new segment begin
       lastSeg = addNewSegment(i, lastSeg, result);
       if (dbgPrint) {
         printf("XXXX_SEG_border: x=%ld y=%f\n",i,in[i]);
       }
     }

   }

   return 1;
}

int cFunctionalSegments::process_SegThreshNoavg(FLOAT_DMEM *in,
    FLOAT_DMEM *out, long Nin, long Nout, sSegData *result)
{
  long i,j;
  if (segmentationAlgorithm == SEG_RELTH_NOAVG) {
    // convert relative thresholds to absolute, given the range
    for (i=0; i<nThresholds; i++) {
      thresholdsTemp[i] = result->min + result->range * thresholds[i];
      //SMILE_IMSG(3,"thresh[%i]=%f",i,thresholdsTemp[i]);
    }
  }
  else if (segmentationAlgorithm == SEG_MRELTH_NOAVG) {
    // convert relative thresholds to absolute, given the mean
    for (i=0; i<nThresholds; i++) {
      thresholdsTemp[i] = result->mean * thresholds[i];
      //SMILE_IMSG(3,"thresh[%i]=%f",i,thresholdsTemp[i]);
    }
  }

  if (autoSegMinLng) {
    segMinLng = Nin/maxNumSeg-1;
    if (segMinLng < 2) segMinLng = 2;
  }

  long lastSeg = -segMinLng/2;

  for (i=1; i<Nin; i++) {
     int threshCross=0;
     for (j=0; j<nThresholds; j++) {
       if ((in[i] > thresholdsTemp[j] && in[i-1] <= thresholdsTemp[j]) || (in[i] < thresholdsTemp[j] && in[i-1] >= thresholdsTemp[j])) {
         threshCross = 1;
         //printf("xxx: tc  ra=%f  raLast=%f\n",ra,raLast);
       }
     }

     if ((threshCross)&&(i - lastSeg > segMinLng) )
     { // found new segment begin
       lastSeg = addNewSegment(i, lastSeg, result);
       if (dbgPrint) printf("XXXX_SEG_border: x=%ld y=%f\n",i,in[i]);
     }

   }

   return 1;
}

int cFunctionalSegments::process_SegChX_oldBuggy(FLOAT_DMEM *in,
    FLOAT_DMEM *out, long Nin, long Nout, sSegData *result)
{
  long i;

  // convert relative thresholds to the current absolute value, if the config option to do so is set
  FLOAT_DMEM Xtmp;
  if (XisRel) Xtmp = result->min + result->range * X;
  else Xtmp = X;

  segMinLng = Nin/maxNumSeg-1;
  if (segMinLng < 2) segMinLng = 2;

  long ravgLng = 3;
  long lastSeg = -segMinLng/2;

  FLOAT_DMEM ravg = 0.0;
  FLOAT_DMEM raLast = 0.0;

  for (i=0; i<Nin; i++) {
     ravg += in[i];
     if (i>=ravgLng) ravg -= in[i-ravgLng];

     FLOAT_DMEM ravgLngCur = (FLOAT_DMEM)( MIN(i+1,ravgLng) );
     FLOAT_DMEM ra = ravg / ravgLngCur;

     int threshCross=0;
     if ( (ra != Xtmp && raLast == Xtmp) || (ra == Xtmp && raLast != Xtmp)) {
       threshCross = 1;
     }
     raLast = ra;

     if ((threshCross)&&(i - lastSeg > segMinLng) )
     { // found new segment begin
       lastSeg = addNewSegment(i, lastSeg, result);
       if (dbgPrint) printf("XXXX_SEG_border: x=%ld y=%f\n",i,in[i]);
      }

   }

   return 1;
}

int cFunctionalSegments::process_SegNonX_oldBuggy(FLOAT_DMEM *in,
    FLOAT_DMEM *out, long Nin, long Nout, sSegData *result)
{
  long i;

  // convert relative thresholds to the current absolute value, if the config option to do so is set
  FLOAT_DMEM Xtmp;
  if (XisRel) Xtmp = result->min + result->range * X;
  else Xtmp = X;

  segMinLng = Nin/maxNumSeg-1;
  if (segMinLng < 2) segMinLng = 2;

  long ravgLng = 3;
  long lastSeg = -segMinLng/2;

  FLOAT_DMEM ravg = 0.0;
  FLOAT_DMEM raLast = 0.0;

  for (i=0; i<Nin; i++) {
     ravg += in[i];
     if (i>=ravgLng) ravg -= in[i-ravgLng];

     FLOAT_DMEM ravgLngCur = (FLOAT_DMEM)( MIN(i+1,ravgLng) );
     FLOAT_DMEM ra = ravg / ravgLngCur;

     int segStart=0;
     int segEnd=0;

     if (ra != Xtmp && raLast == Xtmp) {
       segStart = 1;
     }
     if (ra == Xtmp && raLast != Xtmp) {
       segEnd = 1;
     }
     raLast = ra;

     if (segStart) {
       lastSeg = i;
     }
     if ((segEnd)&&(i - lastSeg > segMinLng) )
     { // found new segment end
       if (dbgPrint) printf("XXXX_SEG_border: end=%ld start=%ld\n",i,lastSeg);
       lastSeg = addNewSegment(i-1, lastSeg, result);
     }

   }

   return 1;
}

int cFunctionalSegments::process_SegEqX_oldBuggy(FLOAT_DMEM *in,
    FLOAT_DMEM *out, long Nin, long Nout, sSegData *result)
{
  long i;

  // convert relative thresholds to the current absolute value, if the config option to do so is set
  FLOAT_DMEM Xtmp;
  if (XisRel) Xtmp = result->min + result->range * X;
  else Xtmp = X;

  segMinLng = Nin/maxNumSeg-1;
  if (segMinLng < 2) segMinLng = 2;

  long ravgLng = 3;
  long lastSeg = -segMinLng/2;

  FLOAT_DMEM ravg = 0.0;
  FLOAT_DMEM raLast = 0.0;

  for (i=0; i<Nin; i++) {
     ravg += in[i];
     if (i>=ravgLng) ravg -= in[i-ravgLng];

     FLOAT_DMEM ravgLngCur = (FLOAT_DMEM)( MIN(i+1,ravgLng) );
     FLOAT_DMEM ra = ravg / ravgLngCur;

     int segStart=0;
     int segEnd=0;
     if (ra != Xtmp && raLast == Xtmp) {
       segEnd = 1;
     }
     if (ra == Xtmp && raLast != Xtmp) {
       segStart = 1;
     }
     raLast = ra;

     if (segStart) {
       lastSeg = i;
     }
     if ((segEnd)&&(i - lastSeg > segMinLng) )
     { // found new segment end
       lastSeg = addNewSegment(i, lastSeg, result);
       if (dbgPrint) printf("XXXX_SEG_border: end=%ld start=%ld\n",i,lastSeg);
     }

   }

   return 1;
}

// this version actually works as expected, because it does not enforce ra==Xtmp, as the old one did
int cFunctionalSegments::process_SegChX(FLOAT_DMEM *in,
    FLOAT_DMEM *out, long Nin, long Nout, sSegData *result)
{
  // convert relative thresholds to the current absolute value, if the config option to do so is set
  FLOAT_DMEM Xtmp;
  if (XisRel) Xtmp = result->min + result->range * X;
  else Xtmp = X;

  long segStartIndex = 0;
  long segEndIndex = 0;
  int inSegment = 0;
  int segStart = 0;
  int segEnd = 0;
  long i;

  /* Note: running average over 3 frames was removed,
     as it will lead to missing the threshold 
     due to round-off errors */

  for (i = 0; i < Nin; i++) {

     if (in[i] != Xtmp) {
       if (inSegment == 1) {  // possible segment candidate
         segEnd = 0;
         segStart++;
         if (segStart >= segMinLng) {
           inSegment = 2;
           if (dbgPrint)
             printf("XXXX_SEG_eqX: end=%ld start=%ld\n",
               segStartIndex - 1, segEndIndex);
           addNewSegment(segStartIndex - 1, segEndIndex, result);
           segStart = 0;
         }
       } else if (inSegment == 0) {  // currently in pause (in == threshold X)
         segStart++;
         segStartIndex = i;
         inSegment = 1;
       } else if (inSegment == 2) {
         segEnd = 0;
       } else if (inSegment == 3) {
         segStart++;
         if (segStart >= segMinLng) {
           inSegment = 2;
           segEnd = 0;
           segStart = 0;
         }
       }
     }
     if (in[i] == Xtmp) {
       if (inSegment == 3) {
         segStart = 0;
         segEnd++;
         if (segEnd >= segMinLng) {
           inSegment = 0;
           if (dbgPrint)
             printf("XXXX_SEG_nonX: end=%ld start=%ld\n",
               segEndIndex - 1, segStartIndex);
           addNewSegment(segEndIndex - 1, segStartIndex, result);
           segEnd = 0;
         }
       } else if (inSegment == 2) {  // currently in pause (in == threshold X)
         segEnd++;
         segEndIndex = i;
         inSegment = 3;
       } else if (inSegment == 0) {
         segStart = 0;
       } else if (inSegment == 1) {
         segEnd++;
         if (segEnd >= pauseMinLng) {
           inSegment = 0;
           segEnd = 0;
           segStart = 0;
         }
       }
     }
   }

   // end a valid segment at the end of the data array
   if (inSegment == 2) {
     segEnd++;
     if (dbgPrint)
       printf("XXXX_SEG_nonX: end=%ld start=%ld\n",
         segEndIndex - 1, segStartIndex);
     addNewSegment(segEndIndex - 1, segStartIndex, result);
   } else if (inSegment == 0) {
     segStart++;
     if (dbgPrint)
       printf("XXXX_SEG_eqX: end=%ld start=%ld\n",
         segStartIndex - 1, segEndIndex);
     addNewSegment(segStartIndex - 1, segEndIndex, result);
   }

   return 1;
}

// the new and proper code, always use this in new configs
int cFunctionalSegments::process_SegNonX(FLOAT_DMEM *in,
    FLOAT_DMEM *out, long Nin, long Nout, sSegData *result)
{
  // convert relative thresholds to the current absolute value, if the config option to do so is set
  FLOAT_DMEM Xtmp;
  if (XisRel) Xtmp = result->min + result->range * X;
  else Xtmp = X;

  long segStartIndex = 0;
  int inSegment = 0;
  int segStart = 0;
  int segEnd = 0;
  long i;

  /* Note: running average over 3 frames was removed,
     as it will lead to missing the threshold 
     due to round-off errors */

  for (i = 0; i < Nin; i++) {

     if (in[i] != Xtmp) {
       if (inSegment == 1) {  // possible segment candidate
         segEnd = 0;
         segStart++;
         if (segStart >= segMinLng) {
           segStart = 0;
           inSegment = 2;
         }
       } else if (inSegment == 0) {  // currently in pause (in == threshold X)
         segStart++;
         segStartIndex = i;
         inSegment = 1;
       } else if (inSegment == 2) {
         segEnd = 0;
       }
     }
     if (in[i] == Xtmp) {
       if (inSegment == 2) {
         segStart = 0;
         segEnd++;
         if (segEnd >= pauseMinLng) {
           inSegment = 0;
           if (dbgPrint)
             printf("XXXX_SEG_nonX: end=%ld start=%ld\n",
               i - segEnd, segStartIndex);
           addNewSegment(i - segEnd, segStartIndex, result);
           segEnd = 0;
         }
       } else if (inSegment == 1) {
         segEnd++;
         if (segEnd >= pauseMinLng) {
           inSegment = 0;
           segEnd = 0;
           segStart = 0;
         }
       }
     }
   }

   // end a valid segment at the end of the data array
   if (inSegment == 2) {
     segEnd++;
     if (dbgPrint)
       printf("XXXX_SEG_nonX: x=%ld y=%ld\n",
           i - segEnd, segStartIndex);
     addNewSegment(i - segEnd, segStartIndex, result);
   }

   return 1;
}

// the new and proper code, always use this in new configs
int cFunctionalSegments::process_SegEqX(FLOAT_DMEM *in,
    FLOAT_DMEM *out, long Nin, long Nout, sSegData *result)
{
  // convert relative thresholds to the current absolute value, if the config option to do so is set
  FLOAT_DMEM Xtmp;
  if (XisRel) Xtmp = result->min + result->range * X;
  else Xtmp = X;

  long segStartIndex = 0;
  int inSegment = 0;
  int segStart = 0;
  int segEnd = 0;
  long i;

  /* Note: running average over 3 frames was removed,
     as it will lead to missing the threshold 
     due to round-off errors */

  for (i = 0; i < Nin; i++) {

     if (in[i] == Xtmp) {
       if (inSegment == 1) {  // possible segment candidate
         segEnd = 0;
         segStart++;
         if (segStart >= segMinLng) {
           segStart = 0;
           inSegment = 2;
         }
       } else if (inSegment == 0) {  // currently in pause (in == threshold X)
         segStart++;
         segStartIndex = i;
         inSegment = 1;
       } else if (inSegment == 2) {
         segEnd = 0;
       }
     }
     if (in[i] != Xtmp) {
       if (inSegment == 2) {
         segStart = 0;
         segEnd++;
         if (segEnd >= pauseMinLng) {
           inSegment = 0;
           if (dbgPrint)
             printf("XXXX_SEG_eqX: end=%ld start=%ld\n",
                 i - segEnd, segStartIndex);
           addNewSegment(i - segEnd, segStartIndex, result);
           segEnd = 0;
         }
       } else if (inSegment == 1) {
         segEnd++;
         if (segEnd >= pauseMinLng) {
           inSegment = 0;
           segEnd = 0;
           segStart = 0;
         }
       }
     }
   }

   // end a valid segment at the end of the data array
   if (inSegment == 2) {
     segEnd++;
     if (dbgPrint)
       printf("XXXX_SEG_eqX: end=%ld start=%ld\n",
           i - segEnd, segStartIndex);
     addNewSegment(i - segEnd, segStartIndex, result);
   }

   return 1;
}



long cFunctionalSegments::process(FLOAT_DMEM *in,
    FLOAT_DMEM *inSorted, FLOAT_DMEM min, FLOAT_DMEM max,
    FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout)
{
  int i;
  if ((Nin>0)&&(out!=NULL)) {
    sSegData result;

    result.nSegments = 0;
    result.max = max;
    result.min = min;
    result.maxSegLen = 0;
    result.minSegLen = 0;
    result.meanSegLen = 0;
    result.seglenStddev = 0;
    result.segLens = (long *)calloc(1,sizeof(long)*(maxNumSeg+1));


    result.range = result.max-result.min;
    result.mean = mean;

    if (dbgPrint) {
      printf("---\n");
      printf("range: %f\n",result.range);
    }

    switch (segmentationAlgorithm) {

    case SEG_DELTA:
      process_SegDelta(in, out, Nin, Nout, &result);
      break;
    case SEG_DELTA2:
      process_SegDelta2(in, out, Nin, Nout, &result);
      break;

    case SEG_RELTH:
    case SEG_MRELTH:
    case SEG_ABSTH:
      process_SegThresh(in, out, Nin, Nout, &result);
      break;

    case SEG_RELTH_NOAVG:
    case SEG_MRELTH_NOAVG:
    case SEG_ABSTH_NOAVG:
      process_SegThreshNoavg(in, out, Nin, Nout, &result);
      break;

    case SEG_CHX:
      if (useOldBuggyChX) {
        process_SegChX_oldBuggy(in, out, Nin, Nout, &result);
      } else {
        process_SegChX(in, out, Nin, Nout, &result);
      }
      break;

    case SEG_NONX:
      if (useOldBuggyChX) {
        process_SegNonX_oldBuggy(in, out, Nin, Nout, &result);
      } else {
        process_SegNonX(in, out, Nin, Nout, &result);
      }
      break;

    case SEG_EQX:
      if (useOldBuggyChX) {
        process_SegEqX_oldBuggy(in, out, Nin, Nout, &result);
      } else {
        process_SegEqX(in, out, Nin, Nout, &result);
      }
      break;

    default:
      SMILE_IERR(1,"selected segmentation algorithm not yet implemented! Fallback: delta method");
      process_SegDelta(in, out, Nin, Nout, &result);

    }


    int n=0;

    // compute length variance
    FLOAT_DMEM lenDev = 0.0;
    FLOAT_DMEM mean;
    if (result.nSegments > 1) mean = (FLOAT_DMEM)result.meanSegLen/((FLOAT_DMEM)result.nSegments);
    else mean = (FLOAT_DMEM)result.meanSegLen;

    for (i=0; i<result.nSegments; i++) {
      lenDev += ((FLOAT_DMEM)result.segLens[i] - mean)*((FLOAT_DMEM)result.segLens[i] - mean);
    }
    if (result.nSegments > 1) {
      lenDev /= (FLOAT_DMEM)result.nSegments;
      lenDev = sqrt(lenDev);
    }
    else lenDev = 0.0;

    free(result.segLens);


    // generate output

    if (enab[FUNCT_NUMSEGMENTS]) {
      if (timeNorm == TIMENORM_SECONDS) {
        FLOAT_DMEM _T = (FLOAT_DMEM)getInputPeriod();
        FLOAT_DMEM Norm = 1.0;
        if (_T != 0.0) { Norm = _T; }
        Norm *= (FLOAT_DMEM)Nin;
        out[n++]=(FLOAT_DMEM)result.nSegments/Norm;
      } else if (timeNorm == TIMENORM_SEGMENT) {
        out[n++]=(FLOAT_DMEM)result.nSegments/(FLOAT_DMEM)(maxNumSeg);
      } else {
        out[n++]=(FLOAT_DMEM)result.nSegments;
      }
    }
    
    if (timeNorm==TIMENORM_SEGMENT) {
      if (enab[FUNCT_SEGMEANLEN]) {
        out[n++]=mean/(FLOAT_DMEM)(Nin);
/*        if (result.nSegments > 1)
          //out[n++]=(FLOAT_DMEM)result.meanSegLen/((FLOAT_DMEM)result.nSegments*(FLOAT_DMEM)(Nin));
        else 
          out[n++]=(FLOAT_DMEM)result.meanSegLen/((FLOAT_DMEM)(Nin));*/
      }
      if (enab[FUNCT_SEGMAXLEN]) out[n++]=(FLOAT_DMEM)result.maxSegLen/(FLOAT_DMEM)(Nin);
      if (enab[FUNCT_SEGMINLEN]) out[n++]=(FLOAT_DMEM)result.minSegLen/(FLOAT_DMEM)(Nin);
      if (enab[FUNCT_SEGLENSTDDEV]) out[n++]=lenDev/(FLOAT_DMEM)(Nin);
    } else if (timeNorm == TIMENORM_FRAMES) {
      if (enab[FUNCT_SEGMEANLEN]) {
        out[n++]=mean;
        /*if (result.nSegments > 1)
          out[n++]=mean; //(FLOAT_DMEM)result.meanSegLen/((FLOAT_DMEM)result.nSegments);
        else 
          out[n++]=(FLOAT_DMEM)result.meanSegLen;*/
      }
      if (enab[FUNCT_SEGMAXLEN]) out[n++]=(FLOAT_DMEM)result.maxSegLen;
      if (enab[FUNCT_SEGMINLEN]) out[n++]=(FLOAT_DMEM)result.minSegLen;
      if (enab[FUNCT_SEGLENSTDDEV]) out[n++]=lenDev;
    } else if (timeNorm == TIMENORM_SECONDS) {
      FLOAT_DMEM _T = (FLOAT_DMEM)getInputPeriod();
                
      FLOAT_DMEM Norm = 1.0;
      if (_T != 0.0) { Norm = _T; }

      if (enab[FUNCT_SEGMEANLEN]) {
        out[n++]=mean*Norm;

        /*if (result.nSegments > 1)
          out[n++]=mean*Norm; //(FLOAT_DMEM)result.meanSegLen*Norm/((FLOAT_DMEM)result.nSegments);
        else 
          out[n++]=(FLOAT_DMEM)result.meanSegLen*Norm;*/
      }
      if (enab[FUNCT_SEGMAXLEN]) out[n++]=(FLOAT_DMEM)result.maxSegLen*Norm;
      if (enab[FUNCT_SEGMINLEN]) out[n++]=(FLOAT_DMEM)result.minSegLen*Norm;
      if (enab[FUNCT_SEGLENSTDDEV]) out[n++]=lenDev*Norm;
    }

    return n;
  }
  return 0;
}

cFunctionalSegments::~cFunctionalSegments()
{
  if (thresholdsTemp != NULL && thresholdsTemp != thresholds) free(thresholdsTemp);
  if (thresholds != NULL) free(thresholds);
}

