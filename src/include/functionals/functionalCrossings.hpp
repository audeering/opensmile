/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: zero-crossings, mean-crossings, arithmetic mean

*/


#ifndef __CFUNCTIONALCROSSINGS_HPP
#define __CFUNCTIONALCROSSINGS_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALCROSSINGS "  zero-crossing rate, mean crossing rate, dc offset, min, and max value"
#define COMPONENT_NAME_CFUNCTIONALCROSSINGS "cFunctionalCrossings"

class cFunctionalCrossings : public cFunctionalComponent {
  private:
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    virtual void myFetchConfig() override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFunctionalCrossings(const char *_name);
    // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
    virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout) override;

//    virtual long getNoutputValues() override { return nEnab; }
    virtual int getRequireSorted() override { return 0; }

    virtual ~cFunctionalCrossings();
};




#endif // __CFUNCTIONALCROSSINGS_HPP
