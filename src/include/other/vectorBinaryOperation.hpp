/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

do elementary element-wise binary operations on vectors 

*/


#ifndef __CVECTORBINARYOPERATION_HPP
#define __CVECTORBINARYOPERATION_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>

#define COMPONENT_DESCRIPTION_CVECTORBINARYOPERATION "This component performs element-wise binary operations on vectors (+, -, *, /, ^, min, max). Requires both fields to have the same dimensions."
#define COMPONENT_NAME_CVECTORBINARYOPERATION "cVectorBinaryOperation"

// operations
#define VBOP_ADD      0
#define VBOP_SUB      1
#define VBOP_MUL      2
#define VBOP_DIV      3
#define VBOP_POW      4
#define VBOP_MIN      5
#define VBOP_MAX      6

class cVectorBinaryOperation : public cDataProcessor {
  private:
    int elementMode, selectionIsRange, dummyMode;
    cVector *vecO;

    int nSel; // number of names in names array
    char **names;
    const char *newName;

    int operation;
    int powOnlyPos;
    bool divZeroOutputVal1_;
    
    int dimension;  // actual number of elements and fields selected (size of mapping array)
    long *startIdx; 

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int setupNewNames(long nEl) override;
    //virtual int configureWriter(const sDmLevelConfig *c) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cVectorBinaryOperation(const char *_name);

    virtual ~cVectorBinaryOperation();
};




#endif // __CVECTORBINARYOPERATION_HPP
