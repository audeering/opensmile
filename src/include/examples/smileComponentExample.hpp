/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example for a cSmileComponent descendant

*/


#ifndef __CSMILECOMPONENTEXAMPLE_HPP
#define __CSMILECOMPONENTEXAMPLE_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>

#define COMPONENT_DESCRIPTION_CSMILECOMPONENTEXAMPLE "example for a cSmileComponent descendant"
#define COMPONENT_NAME_CSMILECOMPONENTEXAMPLE "cSmileComponentExample"


class cSmileComponentExample : public cSmileComponent {
  private:

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    virtual void mySetEnvironment() override;
    virtual int myRegisterInstance(int *runMe=NULL) override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;
    virtual int processComponentMessage(cComponentMessage *_msg) override;

  public:
    SMILECOMPONENT_STATIC_DECL

    cSmileComponentExample(const char *_name);
    virtual ~cSmileComponentExample();
};




#endif // __CSMILECOMPONENTEXAMPLE_HPP
