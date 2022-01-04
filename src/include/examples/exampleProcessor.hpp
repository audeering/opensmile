/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example for dataProcessor descendant

*/


#ifndef __EXAMPLE_PROCESSOR_HPP
#define __EXAMPLE_PROCESSOR_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>

#define COMPONENT_DESCRIPTION_CEXAMPLEPROCESSOR "This is an example of a cDataProcessor descendant. It has no meaningful function, this component is intended as a template for developers."
#define COMPONENT_NAME_CEXAMPLEPROCESSOR "cExampleProcessor"

class cExampleProcessor : public cDataProcessor {
  private:
    double offset;
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    // virtual int dataProcessorCustomFinalise() override;
    // virtual int setupNewNames(long nEl) override;
    // virtual int setupNamesForField() override;
    virtual int configureWriter(sDmLevelConfig &c) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cExampleProcessor(const char *_name);

    virtual ~cExampleProcessor();
};




#endif // __EXAMPLE_PROCESSOR_HPP
