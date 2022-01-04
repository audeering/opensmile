/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals container

passes unsorted row data array AND (if required) sorted row data array to functional processors

*/


#ifndef __CFUNCTIONALS_HPP
#define __CFUNCTIONALS_HPP

#include <core/smileCommon.hpp>
#include <core/winToVecProcessor.hpp>
#include <functionals/functionalComponent.hpp>

#define BUILD_COMPONENT_Functionals
#define COMPONENT_DESCRIPTION_CFUNCTIONALS "computes functionals from input frames, this component uses various cFunctionalXXXX sub-components, which implement the actual functionality"
#define COMPONENT_NAME_CFUNCTIONALS "cFunctionals"
#define COMPONENT_NAME_CFUNCTIONALS_LENGTH 12

class cFunctionals : public cWinToVecProcessor {
  private:
    int nFunctTp, nFunctTpAlloc;  // number of cFunctionalXXXX types found
    char **functTp;  // type names (without cFunctional prefix)
    int *functTpI;   // index of cFunctionalXXXX type in componentManager
    int *functI;   // index of cFunctionalXXXX type in componentManager
    int *functN;   // number of output values of each functional object
    cFunctionalComponent **functObj;
    int requireSorted;
    int nonZeroFuncts;
    const char * functNameAppend;
    int timeNorm;

  protected:
    int nFunctionalsEnabled;
    int nFunctValues;  // size of output vector

    SMILECOMPONENT_STATIC_DECL_PR

//    virtual void myFetchConfig() override;
    virtual int myConfigureInstance() override;

    //virtual int myFinaliseInstance() override;
    //virtual eTickResult myTick(long long t) override;

    virtual int getMultiplier() override;
    //virtual int configureWriter(const sDmLevelConfig *c) override;
    virtual int setupNamesForElement(int idxi, const char*name, long nEl) override;
    virtual int doProcess(int i, cMatrix *row, FLOAT_DMEM*x) override;
    virtual int doProcessMatrix(int i, cMatrix *in, FLOAT_DMEM *out, long nOut) override;

  public:
    SMILECOMPONENT_STATIC_DECL

    cFunctionals(const char *_name);

    virtual ~cFunctionals();
};




#endif // __CFUNCTIONALS_HPP
