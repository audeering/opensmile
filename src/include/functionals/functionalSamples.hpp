/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: rise/fall times, up/down-level times

*/

#ifndef __CFUNCTIONALSAMPLES_HPP
#define __CFUNCTIONALSAMPLES_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALSAMPLES "sampled values at equidistant frames"
#define COMPONENT_NAME_CFUNCTIONALSAMPLES "cFunctionalSamples"

class cFunctionalSamples : public cFunctionalComponent {
  private:
    double* samplepos;
    int nSamples;
    char *tmpstr;
    /*int nUltime, nDltime;
    double *ultime, *dltime;
    int norm;
    int varFctIdx;*/
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    virtual void myFetchConfig() override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFunctionalSamples(const char *name);
    // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
    virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM min, FLOAT_DMEM max, FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout) override;

    //virtual long getNoutputValues() override { return nEnab; }
    virtual const char* getValueName(long i) override;
    virtual int getRequireSorted() override { return 0; }

    virtual ~cFunctionalSamples();
};




#endif // __CFUNCTIONALSAMPLES_HPP
