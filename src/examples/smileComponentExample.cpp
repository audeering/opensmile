/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example for a direct descendant of cSmileComponent

no dataMemory interface functionality is used (thus, the tick method may be ignored)

the component may only process messages, if that is desired

*/


#include <examples/smileComponentExample.hpp>

#define MODULE "cSmileComponentExample"

SMILECOMPONENT_STATICS(cSmileComponentExample)

SMILECOMPONENT_REGCOMP(cSmileComponentExample)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CSMILECOMPONENTEXAMPLE;
  sdescription = COMPONENT_DESCRIPTION_CSMILECOMPONENTEXAMPLE;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  //ct->setField("reader", "dataMemory interface configuration (i.e. what slot to read from)", _confman->getTypeObj("cDataReader"), NO_ARRAY, DONT_FREE);
  SMILECOMPONENT_IFNOTREGAGAIN( {} )

  SMILECOMPONENT_MAKEINFO(cSmileComponentExample);
}

SMILECOMPONENT_CREATE(cSmileComponentExample)



//-----

cSmileComponentExample::cSmileComponentExample(const char *_name) :
  cSmileComponent(_name)
{
  // initialisation code...

}

void cSmileComponentExample::myFetchConfig() 
{

}

void cSmileComponentExample::mySetEnvironment()
{

}

int cSmileComponentExample::myRegisterInstance(int *runMe)
{

  return 1;
}

int cSmileComponentExample::myConfigureInstance()
{

  return 1;
}

int cSmileComponentExample::myFinaliseInstance()
{

  return 1;
}

int cSmileComponentExample::processComponentMessage( cComponentMessage *_msg ) 
{ 
  if (isMessageType(_msg,"myXYZmessage")) {  // see doc/messages.txt for documentation of currently available messages
   
    // return 1;  // if message was processed
  }
  return 0; // if message was not processed
}  

eTickResult cSmileComponentExample::myTick(long long t) 
{
  // see the definition of eTickResult in smileComponent.hpp for the different
  // result codes the tick method may return
  
  return TICK_INACTIVE;  // component did not perform any work
}

cSmileComponentExample::~cSmileComponentExample()
{
  // cleanup code...

}

