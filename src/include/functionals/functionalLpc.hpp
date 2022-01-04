/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

number of segments based on delta thresholding

*/


#ifndef __CFUNCTIONALLPC_HPP
#define __CFUNCTIONALLPC_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALLPC "  LP coefficients as functionals"
#define COMPONENT_NAME_CFUNCTIONALLPC "cFunctionalLpc"

class cFunctionalLpc : public cFunctionalComponent {
private:
  int firstCoeff, lastCoeff, order;
  FLOAT_DMEM *acf, *lpc;
  char * tmpstr;

protected:
  SMILECOMPONENT_STATIC_DECL_PR
    virtual void myFetchConfig() override;

public:
  SMILECOMPONENT_STATIC_DECL

    cFunctionalLpc(const char *_name);
  // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
  virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout) override;
  virtual const char * getValueName(long i) override;

  virtual long getNoutputValues() override { return nEnab; }
  virtual int getRequireSorted() override { return 0; }

  virtual ~cFunctionalLpc();
};




#endif // __CFUNCTIONALLPC_HPP
