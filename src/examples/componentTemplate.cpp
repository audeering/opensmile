/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

very basic and low-level cSmileComponent template ...

*/


#include <examples/componentTemplate.hpp>

#define MODULE "myComponentTemplate"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cMyComponent::registerComponent(_confman);
}
*/

SMILECOMPONENT_STATICS(cMyComponent)
                 // vvv!!!!! change cMyComponent to ..... !
SMILECOMPONENT_REGCOMP(cMyComponent)
//sComponentInfo * cMyComponent::registerComponent(cConfigManager *_confman)
{
  SMILECOMPONENT_REGCOMP_INIT
  cname_ = COMPONENT_NAME_CMYCOMPONENT;
  description_ = COMPONENT_DESCRIPTION_CMYCOMPONENT;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
 // OR: inherit a type, e.g. cDataSink:   SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")

  SMILECOMPONENT_IFNOTREGAGAIN_BEGIN
  
  ct->setField("f1", "this is an example int", 0);
  if (ct->setField("subconf", "this is config of sub-component",
                  _confman->getTypeObj("nameOfSubCompType"), NO_ARRAY, DONT_FREE) == -1) {
     rA=1; // if subtype not yet found, request , re-register in the next iteration
  }

  SMILECOMPONENT_IFNOTREGAGAIN_END

    // vvv!!!!! change cMyComponent to ..... !
  SMILECOMPONENT_MAKEINFO(cMyComponent);
}


SMILECOMPONENT_CREATE(cMyComponent)

// NOTE: use this for abstract classes, e.g. dataSource, etc., which do NOT implement myFetchConfig()
//SMILECOMPONENT_CREATE_ABSTRACT(cMyComponent)


// ----

// constructor:
cMyComponent::cMyComponent(const char *_name) : cSmileComponent(_name);
{

}

/*
virtual int cMyComponent::configureInstance()
{
  if (isConfigured()) return 1;
  
  // ....
}

virtual int cMyComponent::finaliseInstance()
{
  if (isFinalised()) return 1;
  
  // ....
}

virtual int tick(long long t)
{
  if (!isFinalised()) return 0;
  
  // ....
}
*/
