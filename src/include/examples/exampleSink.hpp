/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example dataSink:
reads data from data memory and outputs it to console/logfile (via smileLogger)
this component is also useful for debugging

*/


#ifndef __EXAMPLE_SINK_HPP
#define __EXAMPLE_SINK_HPP

#include <core/smileCommon.hpp>
#include <core/dataSink.hpp>

#define COMPONENT_DESCRIPTION_CEXAMPLESINK "This is an example of a cDataSink descendant. It reads data from the data memory and prints it to the console. This component is intended as a template for developers."
#define COMPONENT_NAME_CEXAMPLESINK "cExampleSink"
#define BUILD_COMPONENT_ExampleSink

class cExampleSink : public cDataSink {
  private:
    const char *filename;
    FILE * fHandle;
    int lag;
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cExampleSink(const char *_name);

    virtual ~cExampleSink();
};




#endif // __EXAMPLE_SINK_HPP
