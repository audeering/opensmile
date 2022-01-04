/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example dataSource
writes data to data memory...

*/


#ifndef __EXAMPLE_SOURCE_HPP
#define __EXAMPLE_SOURCE_HPP

#include <core/smileCommon.hpp>
#include <core/dataSource.hpp>

#define COMPONENT_DESCRIPTION_CEXAMPLESOURCE "This is an example of a cDataSource descendant. It writes random data to the data memory. This component is intended as a template for developers."
#define COMPONENT_NAME_CEXAMPLESOURCE "cExampleSource"
#define BUILD_COMPONENT_ExampleSource

class cExampleSource : public cDataSource {
  private:
    int nValues;
    double randSeed;
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNewNames(long nEl) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cExampleSource(const char *_name);

    virtual ~cExampleSource();
};




#endif // __EXAMPLE_SOURCE_HPP
