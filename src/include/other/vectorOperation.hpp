/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

do elementary operations on vectors 
(i.e. basically everything that does not require history or context,
 everything that can be performed on single vectors w/o external data (except for constant parameters, etc.))

*/


#ifndef __CVECTOROPERATION_HPP
#define __CVECTOROPERATION_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CVECTOROPERATION "This component performs elementary operations on vectors (i.e. basically everything that does not require history or context, everything that can be performed on single vectors w/o external data (except for constant parameters, etc.))"
#define COMPONENT_NAME_CVECTOROPERATION "cVectorOperation"

// n->n mapping operations
#define VOP_NORMALISE 0
#define VOP_ADD       1
#define VOP_MUL       2
#define VOP_LOG       3
#define VOP_NORMALISE_L1 4
#define VOP_SQRT      5
#define VOP_LOGA      6
#define VOP_POW       7
#define VOP_EXP       8
#define VOP_E         9
#define VOP_ABS      10
#define VOP_AGN      11
#define VOP_FLATTEN  12
#define VOP_NORM_RANGE0  13
#define VOP_NORM_RANGE1  14
#define VOP_MIN      15
#define VOP_MAX      16
#define VOP_DB_POW   17
#define VOP_DB_MAG   18
#define VOP_FCONV    20
#define VOP_NORM_MAX_ABS  21

// begin of n->1 mapping operations
#define VOP_X          1000 
// n->1 mapping operations
#define VOP_X_SUM      1001
#define VOP_X_SUMSQ    1002
#define VOP_X_L1       1003
#define VOP_X_L2       1004


class cVectorOperation : public cVectorProcessor {
  private:
    int powOnlyPos;
    double gnGenerator();
    char * operationString_;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    int operation;
    int fscaleA, fscaleB;
    const char * nameBase;
    const char * targFreqScaleStr;

    FLOAT_DMEM param1,param2,logfloor;

    virtual void myFetchConfig() override;
  
    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    //virtual int setupNewNames(long ni) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cVectorOperation(const char *_name);

    virtual ~cVectorOperation();
};




#endif // __CVECTOROPERATION_HPP
