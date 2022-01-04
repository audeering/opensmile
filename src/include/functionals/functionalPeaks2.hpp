/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: number of peaks and various measures associated with peaks

*/


#ifndef __CFUNCTIONALPEAKS2_HPP
#define __CFUNCTIONALPEAKS2_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALPEAKS2 "  number of peaks and various measures associated with peaks, such as mean of peaks, mean distance between peaks, etc. Peak finding is based on : x(t-1) < x(t) > x(t+1) plus an advanced post filtering of low relative amplitude peaks. See source code for brief description of peak picking algorithm. This component provides a new and improved algorithm for peak detection, as compared to cFunctionalPeaks component."
#define COMPONENT_NAME_CFUNCTIONALPEAKS2 "cFunctionalPeaks2"


struct peakMinMaxListEl {
  int type;
  FLOAT_DMEM y;
  long x;
  struct peakMinMaxListEl * next, *prev;
};

class cFunctionalPeaks2 : public cFunctionalComponent {
  private:
    int enabSlope;
    int noClearPeakList;

    FLOAT_DMEM relThresh;
    FLOAT_DMEM absThresh;
    int useAbsThresh;
    int dynRelThresh;
    int isBelowThresh(FLOAT_DMEM diff, FLOAT_DMEM base);

    struct peakMinMaxListEl * mmlistFirst;
    struct peakMinMaxListEl * mmlistLast;
    void addMinMax(int type, FLOAT_DMEM y, long x);
    void dbgPrintMinMaxList(struct peakMinMaxListEl * listEl);
    void removeFromMinMaxList( struct peakMinMaxListEl * listEl );
    void clearList();

    FILE *dbg;
    int consoleDbg;
    const char *posDbgOutp;
    int posDbgAppend;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    virtual void myFetchConfig() override;
    int doRatioLimit_;

    // soft limit to [-10;10] (linear) and non-linear hard-limit
    // to [-20;20] for inputs [-inf;+inf]
    FLOAT_DMEM ratioLimit(FLOAT_DMEM x) {
      if (doRatioLimit_) {
        return smileMath_ratioLimit(x, 10.0, 10.0);
      }
      return x;
    }
    FLOAT_DMEM ratioLimitMax(FLOAT_DMEM altValue) {
      if (doRatioLimit_) {
        return 20.0;  // sum of limit1 + limit2 in above function
      }
      return altValue;
    }

    // limit to -1..+1
    FLOAT_DMEM ratioLimitUnity(FLOAT_DMEM x) {
      if (doRatioLimit_) {
        //printf("Ratio unity limit: In: %e\n", x);
        if (x > (FLOAT_DMEM)1.0)
          return (FLOAT_DMEM)1.0;
        if (x < (FLOAT_DMEM)-1.0)
          return (FLOAT_DMEM)-1.0;
      }
      return x;
    }

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFunctionalPeaks2(const char *name);
    // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
    virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM min, FLOAT_DMEM max, FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout) override;

    virtual long getNoutputValues() override { return nEnab; }
    virtual int getRequireSorted() override { return 0; }

    virtual ~cFunctionalPeaks2();
};




#endif // __CFUNCTIONALPEAKS2_HPP
