/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: number of peaks and various measures associated with peaks

*/


#ifndef __CFUNCTIONALPEAKS_HPP
#define __CFUNCTIONALPEAKS_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALPEAKS "  number of peaks and various measures associated with peaks, such as mean of peaks, mean distance between peaks, etc. Peak finding is based on : x(t-1) < x(t) > x(t+1)."
#define COMPONENT_NAME_CFUNCTIONALPEAKS "cFunctionalPeaks"


class cFunctionalPeaks : public cFunctionalComponent {
  private:
    FLOAT_DMEM lastVal, lastlastVal;
	  int overlapFlag;
	  int nPeakdists;
	  long *peakdists;
	  void addPeakDist(int idx, long dist);

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    virtual void myFetchConfig() override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFunctionalPeaks(const char *_name);
    // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
    virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout) override;

    virtual long getNoutputValues() override { return nEnab; }
    virtual int getRequireSorted() override { return 0; }

    virtual ~cFunctionalPeaks();
};




#endif // __CFUNCTIONALPEAKS_HPP
