/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example vectorProcessor descendant

*/


#ifndef __EXAMPLEVECTORPROCESSOR_HPP
#define __EXAMPLEVECTORPROCESSOR_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CEXAMPLEVECTORPROCESSOR "This is an example of a cVectorProcessor descendant. It has no meaningful function, this component is intended as a template for developers."
#define COMPONENT_NAME_CEXAMPLEVECTORPROCESSOR "cExampleVectorProcessor"

class cExampleVectorProcessor : public cVectorProcessor {
  private:

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    //virtual eTickResult myTick(long long t) override;

    //virtual int configureWriter(const sDmLevelConfig *c) override;

    virtual void configureField(int idxi, long __N, long nOut) override;
    //virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cExampleVectorProcessor(const char *_name);

    virtual ~cExampleVectorProcessor();
};




#endif // __EXAMPLEVECTORPROCESSOR_HPP
