/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Functionals: number of peaks and various measures associated with peaks
New and improved algorithm for peak detection, as compared to cFunctionalPeaks component

*/


#include <functionals/functionalPeaks2.hpp>

#define MODULE "cFunctionalPeaks2"

#define FUNCT_NUMPEAKS          0  // number of peaks
#define FUNCT_MEANPEAKDIST      1  // mean distance between peaks
#define FUNCT_MEANPEAKDISTDELTA 2  // mean of difference of consecutive peak distances
#define FUNCT_PEAKDISTSTDDEV    3  // standard deviation of inter peak distances
#define FUNCT_PEAKRANGEABS      4  // absolute peak amplitude range (max peak - min peak)
#define FUNCT_PEAKRANGEREL      5  // peak amplitude range normalised to the input contour's arithmetic mean
#define FUNCT_PEAKMEAN          6  // arithmetic mean of peak amplitudes
#define FUNCT_PEAKMEANMEANDIST  7  // arithmetic mean of peak amplitudes - arithmetic mean
#define FUNCT_PEAKMEANMEANRATIO 8  // arithmetic mean of peak amplitudes / arithmetic mean
#define FUNCT_PTPAMPMEANABS     9  // mean of peak to peak amplitude differences
#define FUNCT_PTPAMPMEANREL     10  // mean of peak to peak amplitude differences / arithmetic mean
#define FUNCT_PTPAMPSTDDEVABS   11  // standard deviation of peak to peak amplitude differences
#define FUNCT_PTPAMPSTDDEVREL   12  // standard deviation of peak to peak amplitude differences / arithmetic mean

#define FUNCT_MINRANGEABS       13  // absolute minima amplitude range (max peak - min peak)
#define FUNCT_MINRANGEREL       14  // minima amplitude range normalised to the input contour's arithmetic mean
#define FUNCT_MINMEAN           15  // arithmetic mean of minima amplitudes
#define FUNCT_MINMEANMEANDIST   16  // arithmetic mean of minima amplitudes - arithmetic mean
#define FUNCT_MINMEANMEANRATIO  17  // arithmetic mean of minima amplitudes / arithmetic mean
#define FUNCT_MTMAMPMEANABS     18  // mean of minima to minima amplitude differences
#define FUNCT_MTMAMPMEANREL     19  // mean of minima to minima amplitude differences / arithmetic mean
#define FUNCT_MTMAMPSTDDEVABS   20  // standard deviation of min to min amplitude differences
#define FUNCT_MTMAMPSTDDEVREL   21  // standard deviation of min to min amplitude differences / arithmetic mean

#define FUNCT_MEANRISINGSLOPE     22
#define FUNCT_MAXRISINGSLOPE      23
#define FUNCT_MINRISINGSLOPE      24
#define FUNCT_STDDEVRISINGSLOPE   25

#define FUNCT_MEANFALLINGSLOPE    26
#define FUNCT_MAXFALLINGSLOPE     27
#define FUNCT_MINFALLINGSLOPE     28
#define FUNCT_STDDEVFALLINGSLOPE  29

#define FUNCT_COVFALLINGSLOPE  30
#define FUNCT_COVRISINGSLOPE   31

#define N_FUNCTS  32

#define NAMES     "numPeaks","meanPeakDist","meanPeakDistDelta","peakDistStddev",\
  "peakRangeAbs","peakRangeRel","peakMeanAbs","peakMeanMeanDist",\
  "peakMeanRel","ptpAmpMeanAbs","ptpAmpMeanRel","ptpAmpStddevAbs",\
  "ptpAmpStddevRel","minRangeAbs","minRangeRel","minMeanAbs",\
  "minMeanMeanDist","minMeanRel","mtmAmpMeanAbs","mtmAmpMeanRel",\
  "mtmAmpStddevAbs","mtmAmpStddevRel","meanRisingSlope","maxRisingSlope",\
  "minRisingSlope","stddevRisingSlope","meanFallingSlope","maxFallingSlope",\
  "minFallingSlope","stddevFallingSlope","covFallingSlope","covRisingSlope"

const char *peaks2Names[] = {NAMES};

SMILECOMPONENT_STATICS(cFunctionalPeaks2)

SMILECOMPONENT_REGCOMP(cFunctionalPeaks2)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFUNCTIONALPEAKS2;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALPEAKS2;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("numPeaks","1/0=enable/disable output of number of peaks (if norm=segment or frame) and rate of peaks per time (1 second) if norm=second.",0);
    ct->setField("meanPeakDist","1/0=enable/disable output of mean distance between peaks (relative to the input segment length, in seconds, or in frames, see the 'norm' option or the 'masterTimeNorm' option of the cFunctionals parent component)",0);
    ct->setField("meanPeakDistDelta","1/0=enable/disable output of mean difference between consecutive inter peak distances (relative to the input segment length, in seconds, or in frames, see the 'norm' option or the 'masterTimeNorm' option of the cFunctionals parent component) [NOT YET IMPLEMENTED!]",0);
    ct->setField("peakDistStddev","1/0=enable/disable output of standard deviation of inter peak distances",0);

    ct->setField("peakRangeAbs","1/0=enable/disable output of peak range (max peak value - min peak value)",0);
    ct->setField("peakRangeRel","1/0=enable/disable output of peak range (max peak value - min peak value) / arithmetic mean",0);
    ct->setField("peakMeanAbs","1/0=enable/disable output of arithmetic mean of peaks (local maxima)",0);
    ct->setField("peakMeanMeanDist","1/0=enable/disable output of arithmetic mean of peaks - arithmetic mean of all values (mean of peaks to signal mean distance)",0);
    ct->setField("peakMeanRel","1/0=enable/disable output of arithmetic mean of peaks (local maxima) / arithmetic mean of all values (mean of peaks to signal mean ratio ~ peakMeanMeanRatio)",0);
    ct->setField("ptpAmpMeanAbs","1/0=enable/disable output of mean peak to peak (amplitude) difference",0);
    ct->setField("ptpAmpMeanRel","1/0=enable/disable output of mean peak to peak (amplitude) difference / range of signal",0);
    ct->setField("ptpAmpStddevAbs","1/0=enable/disable output of mean peak to peak (amplitude) standard deviation",0);
    ct->setField("ptpAmpStddevRel","1/0=enable/disable output of mean peak to peak (amplitude) standard deviation / range of signal",0);

    ct->setField("minRangeAbs","1/0=enable/disable output of local minima range (max minmum value - min minimum value)",0);
    ct->setField("minRangeRel","1/0=enable/disable output of local minima range (max minmum value - min minimum value) / arithmetic mean",0);
    ct->setField("minMeanAbs","1/0=enable/disable output of arithmetic mean of local minima",0);
    ct->setField("minMeanMeanDist","1/0=enable/disable output of arithmetic mean of local minima - arithmetic mean of all values",0);
    ct->setField("minMeanRel","1/0=enable/disable output of arithmetic mean of local minima / arithmetic mean",0);
    ct->setField("mtmAmpMeanAbs","1/0=enable/disable output of mean minimum to minimum (amplitude) difference",0);
    ct->setField("mtmAmpMeanRel","1/0=enable/disable output of mean minimum to minimum (amplitude) difference / range of signal",0);
    ct->setField("mtmAmpStddevAbs","1/0=enable/disable output of mean minimum to minimum (amplitude) standard deviation",0);
    ct->setField("mtmAmpStddevRel","1/0=enable/disable output of mean minimum to minimum (amplitude) standard deviation / range of signal",0);

    ct->setField("meanRisingSlope","1/0=enable/disable output of the mean of the rising slopes (rising slope is the slope of the line connecting a local minimum (or the beginning of input sample) with the following local maximum/peak or the end of input sample)",0);
    ct->setField("maxRisingSlope","1/0=enable/disable output of maximum rising slope",0);
    ct->setField("minRisingSlope","1/0=enable/disable output of minimum rising slope",0);
    ct->setField("stddevRisingSlope","1/0=enable/disable output of the standard deviation of the rising slopes",0);
    ct->setField("covRisingSlope","1/0=enable/disable output of the coefficient of variation (std. dev. divided by arithmetic mean) of the rising slopes",0);
    ct->setField("meanFallingSlope","1/0=enable/disable output of the mean of the falling slopes (falling slope is the slope of the line connecting a local maximum/peak (or the beginning of input sample) with the following local minimum (or the end of input sample))",0);
    ct->setField("maxFallingSlope","1/0=enable/disable output of maximum falling slope.",0);
    ct->setField("minFallingSlope","1/0=enable/disable output of minimum falling slope",0);
    ct->setField("stddevFallingSlope","1/0=enable/disable output of the standard deviation of the falling slopes",0);
    ct->setField("covFallingSlope","1/0=enable/disable output of the coefficient of variation (std. dev. divided by arithmetic mean) of the falling slopes",0);

    ct->setField("norm","This option specifies how this component should normalise times (if it generates output values related to durations): \n   'segment' (or: 'turn') : normalise to the range 0..1, the result is the relative length wrt. to the segment length )\n   'second'  (absolute time in seconds) \n   'frame' (absolute time in number of frames of input level)","frames");
    ct->setField("noClearPeakList", "1 = don't clear the peak list when a new segment is processed. This should never be used, unless you need compatibility with old (buggy!) behaviour. If you are using a config file with peak2 functionals from before 05. Sept. 2012, you need to enable this option to be compatible with old models and results. ", 0);
    //TODO?: unified time norm handling for all functionals components: frame, seconds, turn !

    ct->setField("absThresh","Gives an absolute threshold for the minimum peak height. Use with caution, use only if you know what you are doing. If this option is not set, relThresh will be used.",0.0);
    ct->setField("relThresh","Gives the threshold relative to the input contour range, which is used to remove peaks and minimima below this threshold. Valid values: 0..1, a higher value will remove more peaks, while a lower value will keep more and less salient peaks. If not using dynRelThresh=1 you should use a default value of ~0.10 otherwise a default of ~0.35",0.1);
    ct->setField("dynRelThresh","1/0 = enable disable dynamic relative threshold. Instead of converting the relThresh to an absolute threshold relThresh*range, the threshold is applied as: abs(a/b-1.0) < relThresh , where a is always larger than b.",0);

    ct->setField("posDbgOutp","Filename for debug output of peak positions. The file will be created initially (unless 'posDbgAppend' is set to 1) and values for consecutive input frames will be appended, separated by a '---' marker line.",(const char *)NULL);
    ct->setField("posDbgAppend","Append to debug ouptut file instead of overwriting it at startup. If the file does not exist, it will be created, even if this option is set to 1.",0);
    ct->setField("consoleDbg","Debug output of peak positions to console if no output file is given (an output file will override this option).",0);
    ct->setField("doRatioLimit", "(1/0) = yes/no. Apply soft limiting of ratio features (mean relative etc.) in order to avoid high uncontrolled output values if the denominator is close to 0. For strict compatibility with pre 2.2 openSMILE releases (also release candidates 2.2rc1), set it to 0. Default in new versions is 1 (enabled).", 1);
  )

  SMILECOMPONENT_MAKEINFO_NODMEM(cFunctionalPeaks2);
}

SMILECOMPONENT_CREATE(cFunctionalPeaks2)

//-----

cFunctionalPeaks2::cFunctionalPeaks2(const char *name) :
  cFunctionalComponent(name,N_FUNCTS,peaks2Names),
  enabSlope(0), mmlistFirst(NULL), mmlistLast(NULL),
  dbg(NULL), consoleDbg(0), useAbsThresh(0),
  noClearPeakList(0), doRatioLimit_(1)
{
}

void cFunctionalPeaks2::myFetchConfig()
{
  parseTimeNormOption();
  noClearPeakList = getInt("noClearPeakList");

  if (getInt("numPeaks")) enab[FUNCT_NUMPEAKS] = 1;
  if (getInt("meanPeakDist")) enab[FUNCT_MEANPEAKDIST] = 1;
  if (getInt("meanPeakDistDelta")) enab[FUNCT_MEANPEAKDISTDELTA] = 1;
  if (getInt("peakDistStddev")) enab[FUNCT_PEAKDISTSTDDEV] = 1;

  if (getInt("peakRangeAbs")) enab[FUNCT_PEAKRANGEABS] = 1;
  if (getInt("peakRangeRel")) enab[FUNCT_PEAKRANGEREL] = 1;
  if (getInt("peakMeanAbs")) enab[FUNCT_PEAKMEAN] = 1;
  if (getInt("peakMeanMeanDist")) enab[FUNCT_PEAKMEANMEANDIST] = 1;
  if (getInt("peakMeanRel")) enab[FUNCT_PEAKMEANMEANRATIO] = 1;
  if (getInt("ptpAmpMeanAbs")) enab[FUNCT_PTPAMPMEANABS] = 1;
  if (getInt("ptpAmpMeanRel")) enab[FUNCT_PTPAMPMEANREL] = 1;
  if (getInt("ptpAmpStddevAbs")) enab[FUNCT_PTPAMPSTDDEVABS] = 1;
  if (getInt("ptpAmpStddevRel")) enab[FUNCT_PTPAMPSTDDEVREL] = 1;

  if (getInt("minRangeAbs")) enab[FUNCT_MINRANGEABS] = 1;
  if (getInt("minRangeRel")) enab[FUNCT_MINRANGEREL] = 1;
  if (getInt("minMeanAbs")) enab[FUNCT_MINMEAN] = 1;
  if (getInt("minMeanMeanDist")) enab[FUNCT_MINMEANMEANDIST] = 1;
  if (getInt("minMeanRel")) enab[FUNCT_MINMEANMEANRATIO] = 1;
  if (getInt("mtmAmpMeanAbs")) enab[FUNCT_MTMAMPMEANABS] = 1;
  if (getInt("mtmAmpMeanRel")) enab[FUNCT_MTMAMPMEANREL] = 1;
  if (getInt("mtmAmpStddevAbs")) enab[FUNCT_MTMAMPSTDDEVABS] = 1;
  if (getInt("mtmAmpStddevRel")) enab[FUNCT_MTMAMPSTDDEVREL] = 1;

  if (getInt("meanRisingSlope")) { enab[FUNCT_MEANRISINGSLOPE] = 1; enabSlope = 1; }
  if (getInt("maxRisingSlope")) { enab[FUNCT_MAXRISINGSLOPE] = 1; enabSlope = 1; }
  if (getInt("minRisingSlope")) { enab[FUNCT_MINRISINGSLOPE] = 1; enabSlope = 1; }
  if (getInt("stddevRisingSlope")) { enab[FUNCT_STDDEVRISINGSLOPE] = 1; enabSlope = 1; }
  if (getInt("meanFallingSlope")) { enab[FUNCT_MEANFALLINGSLOPE] = 1; enabSlope = 1; }
  if (getInt("maxFallingSlope")) { enab[FUNCT_MAXFALLINGSLOPE] = 1; enabSlope = 1; }
  if (getInt("minFallingSlope")) { enab[FUNCT_MINFALLINGSLOPE] = 1; enabSlope = 1; }
  if (getInt("stddevFallingSlope")) { enab[FUNCT_STDDEVFALLINGSLOPE] = 1; enabSlope = 1; }
  if (getInt("covFallingSlope")) { enab[FUNCT_COVFALLINGSLOPE] = 1; enabSlope = 1; }
  if (getInt("covRisingSlope")) { enab[FUNCT_COVRISINGSLOPE] = 1; enabSlope = 1; }

  relThresh = (FLOAT_DMEM)getDouble("relThresh");
  dynRelThresh = getInt("dynRelThresh");
  if (relThresh < 0) {
    SMILE_IWRN(1,"relThresh must be in the range [0..1]!  A value of %f is invalid. Setting to 0.0.",relThresh);
    relThresh = 0.0;
  } else if (relThresh > 1.0 && !dynRelThresh) {
    SMILE_IWRN(1,"relThresh must be in the range [0..1]!  A value of %f is invalid. Setting to 1.0.",relThresh);
    relThresh = 1.0;
  }

  if (isSet("absThresh")) {
    useAbsThresh = 1;
    absThresh = (FLOAT_DMEM)getDouble("absThresh");
    if (dynRelThresh) {
      SMILE_IWRN(1,"set absThresh overwrites dynRelThresh to 0!");
      dynRelThresh = 0;
    }
  }

  posDbgOutp = getStr("posDbgOutp");
  posDbgAppend = getInt("posDbgAppend");
  consoleDbg = getInt("consoleDbg");
  doRatioLimit_ = getInt("doRatioLimit");

  cFunctionalComponent::myFetchConfig();
}


void cFunctionalPeaks2::clearList()
{
  while (mmlistFirst != NULL) {
    struct peakMinMaxListEl * list_next = mmlistFirst->next;
    free(mmlistFirst);
    mmlistFirst = list_next;
  }
  mmlistFirst = NULL;
  mmlistLast = NULL;
}

// add a minimum or maximum to the minmax list
// 1 = max, 0 = min
void cFunctionalPeaks2::addMinMax(int type, FLOAT_DMEM y, long x)
{
  struct peakMinMaxListEl * listEl = (struct peakMinMaxListEl*)malloc(sizeof(struct peakMinMaxListEl));
  listEl->type = type;
  listEl->x = x;
  listEl->y = y;
  listEl->next = NULL;
  listEl->prev = NULL;

  if (mmlistFirst == NULL) {
    mmlistFirst = listEl;
    mmlistLast = listEl;
  } else {
    mmlistLast->next = listEl;
    listEl->prev = mmlistLast;
    mmlistLast = listEl;
  }
}

void cFunctionalPeaks2::removeFromMinMaxList( struct peakMinMaxListEl * listEl )
{
  if (listEl->prev != NULL) {
    listEl->prev->next = listEl->next;
    if (listEl-> next != NULL) {
      listEl->next->prev = listEl->prev;
    } else {
      mmlistLast = listEl->prev;
    }
  } else {
    mmlistFirst = listEl->next;
    if (listEl-> next != NULL) {
      listEl->next->prev = NULL;
    } else {
      mmlistLast = NULL;
    }
  }
  //  NOTE: the caller is responsible for freeing the listEl pointer after calling this function
}

void cFunctionalPeaks2::dbgPrintMinMaxList(struct peakMinMaxListEl * listEl)
{
  if (dbg != NULL) {
    fprintf(dbg,"---\n");
    while (listEl != NULL) {
      if (listEl->type == 1)
        fprintf(dbg,"XXXX_MAX: x=%ld y=%f\n",listEl->x,listEl->y);
      else
        fprintf(dbg,"XXXX_MIN: x=%ld y=%f\n",listEl->x,listEl->y);
      listEl = listEl->next;
    }
  } else if (consoleDbg) {
    printf("---\n");
    while (listEl != NULL) {
      if (listEl->type == 1)
        printf("XXXX_MAX: x=%ld y=%f\n",listEl->x,listEl->y);
      else
        printf("XXXX_MIN: x=%ld y=%f\n",listEl->x,listEl->y);
      listEl = listEl->next;
    }
  }
}

int cFunctionalPeaks2::isBelowThresh(FLOAT_DMEM diff, FLOAT_DMEM base)
{
  if (dynRelThresh) {

    if (base == 0.0) {
      if (diff != 0.0) return 1;
      else return 0;
    }
    if (fabs(diff/base) < relThresh) {
      return 1;
    }
    return 0;

  } else {

    if (diff < absThresh) {
      return 1;
    }
    return 0;

  }
}

long cFunctionalPeaks2::process(FLOAT_DMEM *in,
    FLOAT_DMEM *inSorted, FLOAT_DMEM min, FLOAT_DMEM max,
    FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout)
{
  long i;
  int n=0;
  if ((Nin>0)&&(out!=NULL)) {
    if (posDbgOutp != NULL && dbg == NULL) {
      if (posDbgAppend) {
        dbg = fopen(posDbgOutp,"a");
      } else {
        dbg = fopen(posDbgOutp,"w");
      }
    }

    /* algorithm:
     *   1st: detect all local min/max
         2nd: enforce constraint of minimum rise/fall and discard min/max from list
         3rd: enforce constraint of alternating min/max (if double candidates, choose the highest/lowest max/min)
     *
     */

    // clear peak list from last segment
    if (noClearPeakList != 1) {
      clearList();
    }

    FLOAT_DMEM range = max-min;
    if (!useAbsThresh) absThresh = relThresh*range;

    // step 1: detect ALL local min/max
    for (i=2; i<Nin-2; i++) {
      if (in[i] > in[i-1] && in[i] > in[i+1]) { // max
        addMinMax(1, in[i], i);
      } else if (in[i] < in[i-1] && in[i] < in[i+1]) { // min
        addMinMax(0, in[i], i);
      }
    }


    // step 2a: enforce constraint of mutual minimum rise/fall and discard small peaks from list
    struct peakMinMaxListEl * listEl = mmlistFirst;
    FLOAT_DMEM lastVal = in[0];
    FLOAT_DMEM lastMin = in[0];
    FLOAT_DMEM lastMax = in[0];
    int maxFlag=0, minFlag=0;
    struct peakMinMaxListEl * lastMaxPtr=NULL;
    struct peakMinMaxListEl * lastMinPtr=NULL;
    while (listEl != NULL) { // iterate through list elements
      int doFree = 0;
      if (listEl->type == 1) { //max
        if (isBelowThresh( fabs(listEl->y-lastVal), MIN(listEl->y,lastVal))) {
          if (isBelowThresh( listEl->y - lastMin, lastMin)) {
            // discard this item from the list...
            removeFromMinMaxList(listEl);
            doFree=1;
          } else {
            // next local-global candidate
            if (listEl->y > lastMax*1.05) {
              // discard previous max
              if (lastMaxPtr != NULL) {
                removeFromMinMaxList(lastMaxPtr);
                if (lastMaxPtr != listEl) free(lastMaxPtr);
                else doFree = 1;
              }
              lastMax = listEl->y;
              lastMaxPtr = listEl;
            } else {
              if (minFlag) {
                lastMax = listEl->y;
                lastMaxPtr = listEl;
              } else {
                // discard this, keep last
                removeFromMinMaxList(listEl);
                doFree=1;
              }
            }
            maxFlag = 1;
            minFlag = 0;
          }
        } else {
          // manage the flags and pointers...
          maxFlag = 1; minFlag = 0;
          lastMax = listEl->y;
          lastMaxPtr = listEl;
        }

      } else if (listEl->type == 0) { //min
        if (!isBelowThresh( fabs(listEl->y-lastVal), MIN(listEl->y,lastVal))) {
          // manage the flags and pointers...
          minFlag = 1; maxFlag = 0;
          lastMin = listEl->y;
          lastMinPtr = listEl;
        }

      }
      lastVal = listEl->y;
      struct peakMinMaxListEl * nextPtr =listEl->next;
      if (doFree) free(listEl);
      listEl = nextPtr;
    }

    // step 2b: eliminate small minima
    listEl = mmlistFirst;
    lastMax = in[0];
    while (listEl != NULL) { // iterate through list elements
      int doFree = 0;
      if (listEl->type == 0) { //min
        if (isBelowThresh(lastMax - listEl->y, listEl->y)) {
          // discard this item from the list...
          removeFromMinMaxList(listEl);
          doFree=1;
        }
      } else if (listEl->type == 1) { //max
        lastMax = listEl->y;
      }

      struct peakMinMaxListEl * nextPtr =listEl->next;
      if (doFree) free(listEl);
      listEl = nextPtr;
    }

    // step 3: eliminate duplicate maxima / minima (i.e. with no min/max in between)
    listEl = mmlistFirst;
    lastMax = in[0]; lastMin = in[0];
    minFlag = 0; maxFlag = 0;
    int init = 1;
    while (listEl != NULL) { // iterate through list elements
      int doFree = 0;
      if (listEl->type == 0) { //min
        if (!minFlag || init) { // change of type..
          lastMin = listEl->y;
          lastMinPtr = listEl;
          minFlag = 1;
          init = 0;
        } else {
          if (listEl->y >= lastMin) {
            // eliminate this one
            removeFromMinMaxList(listEl);
            doFree=1;
          } else {
           // eliminate the last one , if not the current one
           if (lastMinPtr != listEl) {
             removeFromMinMaxList(lastMinPtr);
             free(lastMinPtr);
             lastMinPtr = listEl;
             lastMin = listEl->y;
           }
          }
        }
      } else if (listEl->type == 1) { //max
        if (minFlag||init) { // change of type...
          lastMax = listEl->y;
          lastMaxPtr = listEl;
          minFlag = 0;
          init = 0;
        } else {
          if (listEl->y <= lastMax) {
            // eliminate this one
            removeFromMinMaxList(listEl);
            doFree=1;
          } else {
           // eliminate the last one , if not the current one
           if (lastMaxPtr != listEl) {
             removeFromMinMaxList(lastMaxPtr);
             free(lastMaxPtr);
             lastMaxPtr = listEl;
             lastMax = listEl->y;
           }
          }
        }
      }

      struct peakMinMaxListEl * nextPtr =listEl->next;
      if (doFree) free(listEl);
      listEl = nextPtr;
    }

    // write peak positions to console or debug file
    dbgPrintMinMaxList(mmlistFirst);
//    return 1;

    ////////////////////////////////// now collect statistics
    FLOAT_DMEM peakMax = (FLOAT_DMEM)0.0, peakMin = (FLOAT_DMEM)0.0;
    FLOAT_DMEM peakDist = (FLOAT_DMEM)0.0;
    FLOAT_DMEM peakDiff = (FLOAT_DMEM)0.0;
    FLOAT_DMEM peakStddevDist = (FLOAT_DMEM)0.0;
    FLOAT_DMEM peakStddevDiff = (FLOAT_DMEM)0.0;
    long nPeakDist = 0;
    FLOAT_DMEM peakMean = (FLOAT_DMEM)0.0;
    long nPeaks = 0;

    FLOAT_DMEM minMax = (FLOAT_DMEM)0.0, minMin = (FLOAT_DMEM)0.0;
    FLOAT_DMEM minDist = (FLOAT_DMEM)0.0;
    FLOAT_DMEM minDiff = (FLOAT_DMEM)0.0;
    FLOAT_DMEM minStddevDist = (FLOAT_DMEM)0.0;
    FLOAT_DMEM minStddevDiff = (FLOAT_DMEM)0.0;
    long nMinDist = 0;
    FLOAT_DMEM minMean = (FLOAT_DMEM)0.0;
    long nMins = 0;

    // iterate through list elements, 1st pass computations
    listEl = mmlistFirst;
    lastMaxPtr=NULL;
    lastMinPtr=NULL;
    while (listEl != NULL) {
      if (listEl->type == 0) { //min
        // distances and amp. diff., min/max for range
        if (lastMinPtr == NULL) {
          lastMinPtr = listEl;
          minMin = listEl->y;
          minMax = listEl->y;
        } else  {
          nMinDist++;
          minDist += (FLOAT_DMEM)(listEl->x - lastMinPtr->x);
          minDiff += (FLOAT_DMEM)fabs(listEl->y - lastMinPtr->y);
          if (minMin > listEl->y) minMin = listEl->y;
          if (minMax < listEl->y) minMax = listEl->y;
          lastMinPtr = listEl;
        }
        // mean
        minMean += listEl->y;
        nMins++;
      } else { //max
        // distances and amp. diff., min/max for range
        if (lastMaxPtr == NULL) {
          lastMaxPtr = listEl;
          peakMin = listEl->y;
          peakMax = listEl->y;
        } else  {
          nPeakDist++;
          peakDist += (FLOAT_DMEM)(listEl->x - lastMaxPtr->x);
          peakDiff += (FLOAT_DMEM)fabs(listEl->y - lastMaxPtr->y);
          if (peakMin > listEl->y) peakMin = listEl->y;
          if (peakMax < listEl->y) peakMax = listEl->y;
          lastMaxPtr = listEl;
        }
        // mean
        peakMean += listEl->y;
        nPeaks++;
      }

      struct peakMinMaxListEl * nextPtr =listEl->next;
      listEl = nextPtr;
    }

    // mean computation:
    if (nPeaks > 1) {
      peakMean /= (FLOAT_DMEM)nPeaks;
      if (nPeakDist > 1) {
        peakDist /= (FLOAT_DMEM)nPeakDist;
        peakDiff /= (FLOAT_DMEM)nPeakDist;
      }
    }
    if (nMins > 0) {
      minMean /= (FLOAT_DMEM)nMins;
      if (nMinDist > 1) {
        minDist /= (FLOAT_DMEM)nMinDist;
        minDiff /= (FLOAT_DMEM)nMinDist;
      }
    }

    // iterate through list elements, 2nd pass computations (stddev)
    listEl = mmlistFirst;
    lastMaxPtr=NULL;
    lastMinPtr=NULL;
    while (listEl != NULL) {
      if (listEl->type == 0) { //min
        // stddev of distances and amp. diffs.
        if (lastMinPtr == NULL) {
          lastMinPtr = listEl;
        } else  {
          minStddevDist += ((FLOAT_DMEM)(listEl->x - lastMinPtr->x)
              - minDist) * ((FLOAT_DMEM)(listEl->x - lastMinPtr->x) - minDist);
          minStddevDiff += ((FLOAT_DMEM)fabs(listEl->y - lastMinPtr->y)
              - minDiff) * ((FLOAT_DMEM)fabs(listEl->y - lastMinPtr->y) - minDiff);
          lastMinPtr = listEl;
        }
      } else { //max
        // stddev of distances and amp. diffs.
        if (lastMaxPtr == NULL) {
          lastMaxPtr = listEl;
        } else  {
          peakStddevDist += ((FLOAT_DMEM)(listEl->x - lastMinPtr->x)
              - peakDist) * ((FLOAT_DMEM)(listEl->x - lastMinPtr->x) - peakDist);
          peakStddevDiff += ((FLOAT_DMEM)fabs(listEl->y - lastMinPtr->y)
              - peakDiff) * ((FLOAT_DMEM)fabs(listEl->y - lastMinPtr->y) - peakDiff);
          lastMaxPtr = listEl;
        }
      }

      struct peakMinMaxListEl * nextPtr =listEl->next;
      listEl = nextPtr;
    }

    // normalise stddev:
    if (nPeakDist > 1) {
      peakStddevDist /= (FLOAT_DMEM)nPeakDist;
      peakStddevDiff /= (FLOAT_DMEM)nPeakDist;
    }
    if (peakStddevDist > 0.0) peakStddevDist = sqrt(peakStddevDist);
    else peakStddevDist = 0.0;
    if (peakStddevDiff > 0.0) peakStddevDiff = sqrt(peakStddevDiff);
    else peakStddevDiff = 0.0;

    if (nMinDist > 1) {
      minStddevDist /= (FLOAT_DMEM)nMinDist;
      minStddevDiff /= (FLOAT_DMEM)nMinDist;
    }
    if (minStddevDist > 0.0) minStddevDist = sqrt(minStddevDist);
    else minStddevDist = 0.0;
    if (minStddevDiff > 0.0) minStddevDiff = sqrt(minStddevDiff);
    else minStddevDiff = 0.0;


    /// slopes....

    FLOAT_DMEM meanRisingSlope = (FLOAT_DMEM)0.0; int nRising=0;
    FLOAT_DMEM meanFallingSlope = (FLOAT_DMEM)0.0; int nFalling=0;
    FLOAT_DMEM minRisingSlope = (FLOAT_DMEM)0.0;
    FLOAT_DMEM maxRisingSlope = (FLOAT_DMEM)0.0;
    FLOAT_DMEM minFallingSlope = (FLOAT_DMEM)0.0;
    FLOAT_DMEM maxFallingSlope = (FLOAT_DMEM)0.0;
    FLOAT_DMEM stddevRisingSlope = (FLOAT_DMEM)0.0;
    FLOAT_DMEM stddevFallingSlope = (FLOAT_DMEM)0.0;

    int lastIsMax=-1;

    if (enabSlope) {
      FLOAT_DMEM T = (FLOAT_DMEM)getInputPeriod();
      // TODO: T should be controllable by the norm option! Make default backwards compatible...
      // T = 1 for norm frame
      // T = input period for form sec  (new default)
      // T = 1/Nin  for norm segment
      // TODO: option to discard first and last value as pseudo max/min!!
      // iterate through list elements, slope statistics computation
      listEl = mmlistFirst;
      lastMax = in[0]; long lastMaxPos = 0;
      lastMin = in[0]; long lastMinPos = 0;
      while (listEl != NULL) {
        if (listEl->type == 0) { //min
          lastMin = listEl->y;
          lastMinPos = listEl->x;
          if (lastMinPos - lastMaxPos > 0) {
            FLOAT_DMEM slope = (lastMax - lastMin)/((FLOAT_DMEM)(lastMinPos - lastMaxPos)*T);
            meanFallingSlope += slope;
            if (nFalling == 0) {
              minFallingSlope = slope;
              maxFallingSlope = slope;
            } else {
              if (slope < minFallingSlope) minFallingSlope = slope;
              if (slope > maxFallingSlope) maxFallingSlope = slope;
            }
            nFalling++; lastIsMax = 0;
          }
        } else { //max
          lastMax = listEl->y;
          lastMaxPos = listEl->x;
          if (lastMaxPos - lastMinPos > 0) {
            FLOAT_DMEM slope = (lastMax - lastMin)/((FLOAT_DMEM)(lastMaxPos - lastMinPos)*T);
            meanRisingSlope += slope;
            if (nRising == 0) {
              minRisingSlope = slope;
              maxRisingSlope = slope;
            } else {
              if (slope < minRisingSlope) minRisingSlope = slope;
              if (slope > maxRisingSlope) maxRisingSlope = slope;
            }
            nRising++; lastIsMax = 1;
          }
        }

        struct peakMinMaxListEl * nextPtr =listEl->next;
        listEl = nextPtr;
      }

      // compute slope at the end of the input
      if (lastIsMax==1) {
        if (Nin - 1 - lastMaxPos > 0) {
          FLOAT_DMEM slope = (in[Nin-1] - lastMax)/((FLOAT_DMEM)(Nin - 1 - lastMaxPos)*T);
          meanFallingSlope += slope;
          if (nFalling == 0) {
            minFallingSlope = slope;
            maxFallingSlope = slope;
          } else {
            if (slope < minFallingSlope) minFallingSlope = slope;
            if (slope > maxFallingSlope) maxFallingSlope = slope;
          }
          nFalling++;
        }
      } else if (lastIsMax==0) {
        if (Nin - 1 - lastMinPos > 0) {
          FLOAT_DMEM slope = (in[Nin-1] - lastMin)/((FLOAT_DMEM)(Nin - 1 - lastMinPos)*T);
          meanRisingSlope += slope;
          if (nRising == 0) {
            minRisingSlope = slope;
            maxRisingSlope = slope;
          } else {
            if (slope < minRisingSlope) minRisingSlope = slope;
            if (slope > maxRisingSlope) maxRisingSlope = slope;
          }
          nRising++;
        }
      } else if (lastIsMax==-1) {  // no max/min at all...
        FLOAT_DMEM slope = (in[Nin-1]-in[0])/(FLOAT_DMEM)Nin;
        if (slope > 0) { meanRisingSlope = maxRisingSlope = minRisingSlope = slope; nRising = 1; }
        else if (slope < 0) { meanFallingSlope = maxFallingSlope = minFallingSlope = slope; nFalling = 1; }
      }

      if (nRising > 1) meanRisingSlope /= (FLOAT_DMEM)nRising;
      if (nFalling > 1) meanFallingSlope /= (FLOAT_DMEM)nFalling;

      // iterate through list elements, slope statistics computation, 2nd pass for stddev
      listEl = mmlistFirst;
      lastMax = in[0]; lastMaxPos = 0;
      lastMin = in[0]; lastMinPos = 0;
      while (listEl != NULL) {
        if (listEl->type == 0) { //min
          lastMin = listEl->y;
          lastMinPos = listEl->x;
          if (lastMinPos - lastMaxPos > 0) {
            FLOAT_DMEM slope = (lastMax - lastMin)/((FLOAT_DMEM)(lastMinPos - lastMaxPos)*T);
            stddevFallingSlope += (slope-meanFallingSlope)*(slope-meanFallingSlope);
          }
        } else { //max
          lastMax = listEl->y;
          lastMaxPos = listEl->x;
          if (lastMaxPos - lastMinPos) {
            FLOAT_DMEM slope = (lastMax - lastMin)/((FLOAT_DMEM)(lastMaxPos - lastMinPos)*T);
            stddevRisingSlope += (slope-meanRisingSlope)*(slope-meanRisingSlope);
          }
        }

        struct peakMinMaxListEl * nextPtr =listEl->next;
        listEl = nextPtr;
      }

      if (nRising > 1) stddevRisingSlope /= (FLOAT_DMEM)nRising;
      if (nFalling > 1) stddevFallingSlope /= (FLOAT_DMEM)nFalling;

      if (stddevRisingSlope > 0.0) stddevRisingSlope = sqrt(stddevRisingSlope);
      else stddevRisingSlope = 0.0;
      if (stddevFallingSlope > 0.0) stddevFallingSlope = sqrt(stddevFallingSlope);
      else stddevFallingSlope = 0.0;
    }

    //// normalisation
    if (timeNorm==TIMENORM_SECONDS) {
      peakDist *= (FLOAT_DMEM)getInputPeriod();
      peakStddevDist *= (FLOAT_DMEM)getInputPeriod();
      minDist *= (FLOAT_DMEM)getInputPeriod();
      minStddevDist *= (FLOAT_DMEM)getInputPeriod();
    } else if (timeNorm==TIMENORM_SEGMENT) {
      peakDist /= (FLOAT_DMEM)Nin;
      peakStddevDist /= (FLOAT_DMEM)Nin;
      minDist /= (FLOAT_DMEM)Nin;
      minStddevDist /= (FLOAT_DMEM)Nin;
    }

    ///////////// value output


    if (enab[FUNCT_NUMPEAKS]) {
      if (timeNorm == TIMENORM_SECONDS) {
        out[n++]= ((FLOAT_DMEM)nPeaks) / ((FLOAT_DMEM)Nin * (FLOAT_DMEM)getInputPeriod());
      } else {
        out[n++]= (FLOAT_DMEM)nPeaks;
      }
    }
    if (enab[FUNCT_MEANPEAKDIST])
      out[n++]=peakDist;
    if (enab[FUNCT_MEANPEAKDISTDELTA])
      out[n++]=0.0; // TODO!
    if (enab[FUNCT_PEAKDISTSTDDEV])
      out[n++]=peakStddevDist;
    if (enab[FUNCT_PEAKRANGEABS])
      out[n++]=peakMax-peakMin;
    if (enab[FUNCT_PEAKRANGEREL]) {
      if (range != 0.0) {
        out[n++]=ratioLimitUnity((FLOAT_DMEM)fabs((peakMax-peakMin)/range));
      } else {
        out[n++]=peakMax-peakMin;
      }
    }
    if (enab[FUNCT_PEAKMEAN])
      out[n++]=peakMean;
    if (enab[FUNCT_PEAKMEANMEANDIST])
      out[n++]=peakMean - mean;
    // TODO: better: peakMean / range  ratio !?
    if (enab[FUNCT_PEAKMEANMEANRATIO]) {
      if (mean != 0.0) {
        // we need to limit the range of ratio, because
        // if mean is near 0 (and peakMean not) it might have
        // uncontrolled high values
        // NOTE: only applied if doRatioLimit_ == 1
        //printf("PEAK MEAN = %e, -- mean = %e (N %i)\n", peakMean, mean, Nin);
        out[n++] = ratioLimit(peakMean / mean);
      } else {
        // use max of ratio limit in case of mean 0 for continuity
        // peakMean is alternate value in case of compatibility mode (doRatioLimit_ == 0)
        out[n++] = ratioLimitMax(peakMean);
      }
    }
    if (enab[FUNCT_PTPAMPMEANABS])
      out[n++] = peakDiff;
    if (enab[FUNCT_PTPAMPMEANREL]) {
      if (range != 0.0) {
        out[n++] = ratioLimitUnity(peakDiff / range);
      } else {
        out[n++] = peakDiff;
      }
    }
    if (enab[FUNCT_PTPAMPSTDDEVABS])
      out[n++] = peakStddevDiff;
    if (enab[FUNCT_PTPAMPSTDDEVREL]) {
      if (range != 0.0) {
        out[n++] = ratioLimitUnity(peakStddevDiff / range);
      } else {
        out[n++] = peakStddevDiff;
      }
    }

    if (enab[FUNCT_MINRANGEABS])
      out[n++] = minMax - minMin;
    if (enab[FUNCT_MINRANGEREL]) {
      if (range != 0.0) {
        out[n++] = ratioLimitUnity((FLOAT_DMEM)fabs((minMax - minMin) / range));
      } else {
        out[n++] = minMax - minMin;
      }
    }
    if (enab[FUNCT_MINMEAN])
      out[n++] = minMean;
    if (enab[FUNCT_MINMEANMEANDIST]) {
      out[n++] = mean - minMean;
    }
    if (enab[FUNCT_MINMEANMEANRATIO]) {
      if (mean != 0.0) {
        out[n++] = ratioLimit(minMean / mean);
      } else {
        out[n++] = ratioLimitMax(minMean);
      }
    }
    if (enab[FUNCT_MTMAMPMEANABS])
      out[n++] = minDiff;
    if (enab[FUNCT_MTMAMPMEANREL]) {
      if (range != 0.0) {
        out[n++] = ratioLimitUnity(minDiff / range);
      } else {
        out[n++] = minDiff;
      }
    }
    if (enab[FUNCT_MTMAMPSTDDEVABS])
      out[n++] = minStddevDiff;
    if (enab[FUNCT_MTMAMPSTDDEVREL]) {
      if (range != 0.0) {
        out[n++] = ratioLimitUnity(minStddevDiff / range);
      } else {
        out[n++] = minStddevDiff;
      }
    }

    if (enab[FUNCT_MEANRISINGSLOPE])
      out[n++] = meanRisingSlope;
    if (enab[FUNCT_MAXRISINGSLOPE])
      out[n++] = maxRisingSlope;
    if (enab[FUNCT_MINRISINGSLOPE])
      out[n++] = minRisingSlope;
    if (enab[FUNCT_STDDEVRISINGSLOPE])
      out[n++] = stddevRisingSlope;

    if (enab[FUNCT_MEANFALLINGSLOPE])
      out[n++] = meanFallingSlope;
    if (enab[FUNCT_MAXFALLINGSLOPE])
      out[n++] = maxFallingSlope;
    if (enab[FUNCT_MINFALLINGSLOPE])
      out[n++] = minFallingSlope;
    if (enab[FUNCT_STDDEVFALLINGSLOPE])
      out[n++] = stddevFallingSlope;

    if (enab[FUNCT_COVFALLINGSLOPE]) {
      if (meanFallingSlope > (FLOAT_DMEM)0.0) {
        out[n++] = ratioLimit(stddevFallingSlope / meanFallingSlope);
      } else {
        out[n++] = (FLOAT_DMEM)0.0;
      }
    }
    if (enab[FUNCT_COVRISINGSLOPE]) {
      if (meanRisingSlope > (FLOAT_DMEM)0.0) {
        out[n++] = ratioLimit(stddevRisingSlope / meanRisingSlope);
      } else {
        out[n++] = (FLOAT_DMEM)0.0;
      }
    }
    return n;
  }
  return 0;
}

cFunctionalPeaks2::~cFunctionalPeaks2()
{
  // free the local min/max list
  clearList();
  if (dbg != NULL) fclose(dbg);
}

