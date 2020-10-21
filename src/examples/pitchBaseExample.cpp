/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Base class for pitch components, provides standard output values and configuration

*/



#include <examples/pitchBaseExample.hpp>

#define MODULE "cPitchBaseExample"


SMILECOMPONENT_STATICS(cPitchBaseExample)

SMILECOMPONENT_REGCOMP(cPitchBaseExample)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CPITCHBASEEXAMPLE;
  sdescription = COMPONENT_DESCRIPTION_CPITCHBASEEXAMPLE;

  // we inherit cPitchBase configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cPitchBase")

  // if the inherited config type was found, we register our configuration variables
  SMILECOMPONENT_IFNOTREGAGAIN( {} // <- this is only to avoid compiler warnings...
    // name append has a special role: it is defined in cDataProcessor, and can be overwritten here:
	  // if you set description to NULL, the existing description will be used, thus the following call can
  	// be used to update the default value:
    //ct->setField("nameAppend",NULL,"processed");

    // add custom config here...

  )

  // The configType gets automatically registered with the config manger by the SMILECOMPONENT_IFNOTREGAGAIN macro
  
  // we now create out sComponentInfo, including name, description, success status, etc. and return that
  SMILECOMPONENT_MAKEINFO(cPitchBaseExample);
}

SMILECOMPONENT_CREATE(cPitchBaseExample)

//-----

cPitchBaseExample::cPitchBaseExample(const char *_name) :
  cPitchBase(_name)
{

}

void cPitchBaseExample::myFetchConfig()
{
  cPitchBase::myFetchConfig();

  // fetch custom config here...

}

/* override, if you want to set custom names!
int cPitchBaseExample::setupNewNames(long nEl)
{
  int n = cPitchBase::setupNewNames(nEl);
  // set up custom names here:

  return n;
}
*/

int cPitchBaseExample::pitchDetect(FLOAT_DMEM * _inData, long _N, double _fsSec, double _baseT, FLOAT_DMEM *_f0cand, FLOAT_DMEM *_candVoice, FLOAT_DMEM *_candScore, long _nCandidates)
{
  // to be implemented by child class

  // return actual number of candidates on success (0 on failure...)
  return 0;
}

int cPitchBaseExample::addCustomOutputs(FLOAT_DMEM *dstCur, long NdstLeft)
{
  // to be implemented by child class

  // return the number of custom outputs that were added..
  return 0;
}


cPitchBaseExample::~cPitchBaseExample()
{

}

