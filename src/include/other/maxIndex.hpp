/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

computes the index of the feature with the maximum index

*/


#ifndef __CMAXINDEX_HPP
#define __CMAXINDEX_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>
#include <math.h>

#define COMPONENT_DESCRIPTION_CMAXINDEX "This component computes the indices of the features with the maximum absolute values per frame."
#define COMPONENT_NAME_CMAXINDEX "cMaxIndex"

class cMaxIndex : public cVectorProcessor {
  private:
    int nIndices, minFeature, maxFeature;
    FLOAT_DMEM noise;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    //virtual eTickResult myTick(long long t) override;

    //virtual int configureWriter(const sDmLevelConfig *c) override;
    //virtual int dataProcessorCustomFinalise() override;
    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cMaxIndex(const char *_name);

    virtual ~cMaxIndex();
};




#endif // __CMAXINDEX_HPP
