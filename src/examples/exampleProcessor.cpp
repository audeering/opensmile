/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example dataProcessor
writes data to data memory...

** you can use this as a template for custom dataProcessor components **

*/


#include <examples/exampleProcessor.hpp>

#define MODULE "cExampleProcessor"

SMILECOMPONENT_STATICS(cExampleProcessor)

SMILECOMPONENT_REGCOMP(cExampleProcessor)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CEXAMPLEPROCESSOR;
  sdescription = COMPONENT_DESCRIPTION_CEXAMPLEPROCESSOR;

  // we inherit cDataProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("offset","The offset that this dummy component adds to the input values.",1.0);
  )
  
  SMILECOMPONENT_MAKEINFO(cExampleProcessor);
}


SMILECOMPONENT_CREATE(cExampleProcessor)

//-----

cExampleProcessor::cExampleProcessor(const char *_name) :
  cDataProcessor(_name)
{
}

void cExampleProcessor::myFetchConfig()
{
  cDataProcessor::myFetchConfig();

  // load all configuration parameters you will later require fast and easy access to here:

  offset = getDouble("offset");
  // note, that it is "polite" to output the loaded parameters at debug level 2:
  SMILE_IDBG(2,"offset = %f",offset);
}

/*  This method is rarely used. It is only there to improve readability of component code.
    It is called from cDataProcessor::myFinaliseInstance just before the call to configureWriter.
    I.e. you can do everything that you would do here, also in configureWriter()
    However, if you implement the method, it must return 1 in order for the configure process to continue!
*/
/*
int cExampleProcessor::configureReader() 
{
  return 1;
}
*/

int cExampleProcessor::configureWriter(sDmLevelConfig &c) 
{
  // if you would like to change the write level parameters... do so HERE:

  c.T = 0.01; /* don't forget to set the write level sample/frame period */
  c.nT = 100; /* e.g. 100 frames buffersize for ringbuffer */

  return 1; /* success */
}

/* You shouldn't need to touch this....
int cExampleProcessor::myConfigureInstance()
{
  int ret = cDataProcessor::myConfigureInstance();
  return ret;
}
*/

/*
  Do what you like here... this is called after the input names and number of input elements have become available, 
  so you may use them here.
*/
/*
int cExampleProcessor::dataProcessorCustomFinalise()
{
  
  return 1;
}
*/


/* 
  Use setupNewNames() to freely set the data elements and their names in the output level
  The input names are available at this point, you can get them via reader->getFrameMeta()
  Please set "namesAreSet" to 1, when you do set names
*/
/*
int cExampleProcessor::setupNewNames(long nEl) 
{
  // namesAreSet = 1;
  return 1;
}
*/

/*
  If you don't use setupNewNames() you may set the names for each input field by overwriting the following method:
*/
/*
int cExampleProcessor::setupNamesForField( TODO )
{
  // DOC TODO...
}
*/

int cExampleProcessor::myFinaliseInstance()
{
  int ret = cDataProcessor::myFinaliseInstance();
  if (ret) {
    // do all custom init stuff here... 
    // e.g. allocating and initialising memory (which is not used before, e.g. in setupNames, etc.),
    // loading external data, 
    // etc.

    // ...

  }
  return ret;
}


eTickResult cExampleProcessor::myTick(long long t)
{
  /* actually process data... */

  SMILE_IDBG(4,"tick # %i, processing value vector",t);

  // check if the destination memory level has enough space to 
  // write our result
  if (!writer_->checkWrite(1)) {
    return TICK_DEST_NO_SPACE;
  }

  // get next frame from dataMemory
  cVector *vec = reader_->getNextFrame();

  // return if there is no frame in the source level available right now
  if (vec == NULL) {
    return TICK_SOURCE_NOT_AVAIL;
  }

  // add offset
  int i;
  for (i=0; i<vec->N; i++) {
    vec->data[i] += (FLOAT_DMEM)offset;
  }

  // if you create a new vector here and pass it to setNextFrame(),
  // then be sure to assign a valid tmeta info for correct timing info:
  // e.g.:
  //   myVec->setTimeMeta(vec->tmeta);


  // save to dataMemory
  writer_->setNextFrame(vec);

  return TICK_SUCCESS;
}


cExampleProcessor::~cExampleProcessor()
{
  // cleanup...

}

