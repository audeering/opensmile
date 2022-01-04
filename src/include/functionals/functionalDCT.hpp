/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

number of segments based on delta thresholding

*/


#ifndef __CFUNCTIONALDCT_HPP
#define __CFUNCTIONALDCT_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALDCT "  DCT coefficients"
#define COMPONENT_NAME_CFUNCTIONALDCT "cFunctionalDCT"

class cFunctionalDCT : public cFunctionalComponent {
private:
  int firstCoeff, lastCoeff;
  int nCo;
  long N;
  long costableNin;
  FLOAT_DMEM * costable;
  FLOAT_DMEM factor;
  char *tmpstr;

protected:
  SMILECOMPONENT_STATIC_DECL_PR
    virtual void myFetchConfig() override;
  virtual void initCostable(long Nin, long Nout);

public:
  SMILECOMPONENT_STATIC_DECL

    cFunctionalDCT(const char *_name);
  // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
  virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout) override;
  virtual const char * getValueName(long i) override;

  virtual long getNoutputValues() override { return nEnab; }
  virtual int getRequireSorted() override { return 0; }

  virtual ~cFunctionalDCT();
};




#endif // __CFUNCTIONALDCT_HPP
