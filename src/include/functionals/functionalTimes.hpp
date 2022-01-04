/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: rise/fall times, up/down-level times

*/

#ifndef __CFUNCTIONALTIMES_HPP
#define __CFUNCTIONALTIMES_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALTIMES "  up- and down-level times + rise and fall, left- and right-curve times, duration, etc."
#define COMPONENT_NAME_CFUNCTIONALTIMES "cFunctionalTimes"

class cFunctionalTimes : public cFunctionalComponent {
  private:
    int nUltime, nDltime;
    double *ultime, *dltime;
    char *tmpstr;
    int varFctIdx;
    int buggySecNorm;
    int useRobustPercentileRange_;
    FLOAT_DMEM pctlRangeMargin_;
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    virtual void myFetchConfig() override;
    FLOAT_DMEM getPctlMin(FLOAT_DMEM *sorted, long N);
    FLOAT_DMEM getPctlMax(FLOAT_DMEM *sorted, long N);

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFunctionalTimes(const char *name);
    // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
    virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM min, FLOAT_DMEM max, FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout) override;

    //virtual long getNoutputValues() override { return nEnab; }
    virtual const char* getValueName(long i) override;
    virtual int getRequireSorted() override {
      if (useRobustPercentileRange_)
        return 1;
      return 0;
    }

    virtual ~cFunctionalTimes();
};




#endif // __CFUNCTIONALTIMES_HPP
