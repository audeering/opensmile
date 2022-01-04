/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: statistical moments

*/



#ifndef __CFUNCTIONALMOMENTS_HPP
#define __CFUNCTIONALMOMENTS_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALMOMENTS "  statistical moments (standard deviation, variance, skewness, kurtosis)"
#define COMPONENT_NAME_CFUNCTIONALMOMENTS "cFunctionalMoments"

class cFunctionalMoments : public cFunctionalComponent {
  private:
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    virtual void myFetchConfig() override;
    int doRatioLimit_;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFunctionalMoments(const char *_name);
    // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
    virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM min, FLOAT_DMEM max, FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout) override;

    virtual long getNoutputValues() override { return nEnab; }
    virtual int getRequireSorted() override { return 0; }

    virtual ~cFunctionalMoments();
};




#endif // __CFUNCTIONALMOMENTS_HPP
