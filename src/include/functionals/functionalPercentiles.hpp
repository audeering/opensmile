/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: percentiles and quartiles, and inter-percentile/quartile ranges

*/


#ifndef __CFUNCTIONALPERCENTILES_HPP
#define __CFUNCTIONALPERCENTILES_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALPERCENTILES "  percentile values and inter-percentile ranges (including quartiles, etc.). This component sorts the input array and then chooses the value at the index closest to p*buffer_len for the p-th percentile (p=0..1)."
#define COMPONENT_NAME_CFUNCTIONALPERCENTILES "cFunctionalPercentiles"

class cFunctionalPercentiles : public cFunctionalComponent {
  private:
    int nPctl, nPctlRange, nPctlQuot;
    double *pctl;
    int *pctlr1, *pctlr2;
    int *pctlq1, *pctlq2;
    char *tmpstr;
    int quickAlgo, interp;
    long varFctIdx;

    long getPctlIdx(double p, long N);
    FLOAT_DMEM getInterpPctl(double p, FLOAT_DMEM *sorted, long N);
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    virtual void myFetchConfig() override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFunctionalPercentiles(const char *name);
    // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
    virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout) override;

    //virtual long getNoutputValues() override { return nEnab; }
    virtual const char* getValueName(long i) override;
    virtual int getRequireSorted() override { if (quickAlgo) return 0; else return 1; }

    virtual ~cFunctionalPercentiles();
};




#endif // __CFUNCTIONALPERCENTILES_HPP
