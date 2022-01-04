/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

number of segments based on delta thresholding

*/


#ifndef __CFUNCTIONALONSET_HPP
#define __CFUNCTIONALONSET_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALONSET "  relative position of the first onset and the last offset based on simple thresholding. Number of onsets and offsets can also be computed."
#define COMPONENT_NAME_CFUNCTIONALONSET "cFunctionalOnset"

class cFunctionalOnset : public cFunctionalComponent {
  private:
    int useAbsVal;
    FLOAT_DMEM thresholdOnset, thresholdOffset;
	//int overlapFlag;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    virtual void myFetchConfig() override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFunctionalOnset(const char *_name);
    // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
    virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout) override;

    virtual long getNoutputValues() override { return nEnab; }
    virtual int getRequireSorted() override { return 0; }

    virtual ~cFunctionalOnset();
};




#endif // __CFUNCTIONALONSET_HPP
