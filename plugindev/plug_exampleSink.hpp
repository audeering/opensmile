/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example plugin: data sink example

*/


#ifndef __EXAMPLE_SINK_HPP
#define __EXAMPLE_SINK_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <core/dataSink.hpp>

#define COMPONENT_DESCRIPTION_CEXAMPLESINK "dataSink example, read data and prints it to screen"
#define COMPONENT_NAME_CEXAMPLESINK "cExampleSinkPlugin"

// add this for plugins to compile properly in MSVC++
#ifdef SMILEPLUGIN
#ifdef class
#endif
#endif

class cExampleSinkPlugin : public cDataSink {
  private:
/*  static const char *cname;  // name of component object type
    static const char *description;*/

    const char *filename;
    int lag;
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;


  public:
    //static sComponentInfo * registerComponent(cConfigManager *_confman);
    //static cSmileComponent * create(const char *_instname);
    SMILECOMPONENT_STATIC_DECL
    
    cExampleSinkPlugin(const char *_name);

    virtual ~cExampleSinkPlugin();
};




#endif // __EXAMPLE_SINK_HPP
