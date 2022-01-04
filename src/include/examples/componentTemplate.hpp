/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:


*/


#ifndef __MY_COMPONENT_HPP
#define __MY_COMPONENT_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>

#define COMPONENT_DESCRIPTION_CMYCOMPONENT "a good template..."
#define COMPONENT_NAME_CMYCOMPONENT "cMyComponent"

class cMyComponent : public cSmileComponent {
  private:

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

	virtual void myFetchConfig() override;

  public:
    SMILECOMPONENT_STATIC_DECL

    //cMyComponent(cConfigManager *cm) : cSmileComponent("myComponentTemplate",cm) {}
    cMyComponent(const char *_name);

    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;
    virtual int manualConfig(); // custom function with arbirtrary parameters, use instead of myFetchConfig when passing NULL for cConfigManager to constructor

    virtual ~cMyComponent() {}
};




#endif // __MY_COMPONENT_HPP
