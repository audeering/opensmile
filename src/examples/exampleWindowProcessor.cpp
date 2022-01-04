/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: 

an example of a windowProcessor component

*/



#include <examples/exampleWindowProcessor.hpp>
//#include <math.h>

#define MODULE "cExampleWindowProcessor"


SMILECOMPONENT_STATICS(cExampleWindowProcessor)

SMILECOMPONENT_REGCOMP(cExampleWindowProcessor)
{
  if (_confman == NULL) return NULL;
  int rA = 0;

  scname = COMPONENT_NAME_CEXAMPLEWINDOWPROCESSOR;
  sdescription = COMPONENT_DESCRIPTION_CEXAMPLEWINDOWPROCESSOR;

  // we inherit cWindowProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cWindowProcessor")

  // if the inherited config type was found, we register our configuration variables
  SMILECOMPONENT_IFNOTREGAGAIN( {} // <- this is only to avoid compiler warnings...
    // name append has a special role: it is defined in cDataProcessor, and can be overwritten here:
	// if you set description to NULL, the existing description will be used, thus the following call can
	// be used to update the default value:
    //ct->setField("nameAppend",NULL,"processed");
    // this is an example for adding an integer option:
	//ct->setField("inverse","1 = perform inverse FFT",0);
  )

  // The configType gets automatically registered with the config manger by the SMILECOMPONENT_IFNOTREGAGAIN macro
  
  // we now create out sComponentInfo, including name, description, success status, etc. and return that
  SMILECOMPONENT_MAKEINFO(cExampleWindowProcessor);
}

SMILECOMPONENT_CREATE(cExampleWindowProcessor)

//-----

cExampleWindowProcessor::cExampleWindowProcessor(const char *_name) :
  cWindowProcessor(_name,1,0)
{
}


void cExampleWindowProcessor::myFetchConfig()
{
  cWindowProcessor::myFetchConfig();
  
  k = (FLOAT_DMEM)getDouble("k");
  SMILE_IDBG(2,"k = %f",k);
  if ((k<0.0)||(k>1.0)) {
    SMILE_IERR(1,"k must be in the range [0;1]! Setting k=0.0 !");
    k=0.0;
  }
}

// order is the amount of memory (overlap) that will be present in _in
// buf will have nT timesteps, however also order negative indicies (i.e. you may access a negative array index up to -order!)
int cExampleWindowProcessor::processBuffer(cMatrix *_in, cMatrix *_out, int _pre, int _post )
{
  long n;

  FLOAT_DMEM *x=_in->data;
  FLOAT_DMEM *y=_out->data;
  for (n=0; n<_out->nT; n++) {
    *(y++) = *(x) - k * *(x-1);
    x++;
  }
  return 1;
}


cExampleWindowProcessor::~cExampleWindowProcessor()
{
}

