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


#include <examples/exampleSource.hpp>
#define MODULE "cExampleSource"

SMILECOMPONENT_STATICS(cExampleSource)

SMILECOMPONENT_REGCOMP(cExampleSource)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CEXAMPLESOURCE;
  sdescription = COMPONENT_DESCRIPTION_CEXAMPLESOURCE;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("nValues","The number of random values to generate",1);
    ct->setField("randSeed","The random seed",1.0);
  )

  SMILECOMPONENT_MAKEINFO(cExampleSource);
}

SMILECOMPONENT_CREATE(cExampleSource)

//-----

cExampleSource::cExampleSource(const char *_name) :
  cDataSource(_name)
{
  // ...
}

void cExampleSource::myFetchConfig()
{
  cDataSource::myFetchConfig();
  
  nValues = getInt("nValues");
  SMILE_IDBG(2,"nValues = %i",nValues);
  randSeed = getDouble("randSeed");
  SMILE_IDBG(2,"nValues = %f",randSeed);
}

/*
int cExampleSource::myConfigureInstance()
{
  int ret = cDataSource::myConfigureInstance();
  // ...

  return ret;
}
*/

int cExampleSource::configureWriter(sDmLevelConfig &c)
{
  // configure your writer by setting values in &c

  return 1;
}

// NOTE: nEl is always 0 for dataSources....
int cExampleSource::setupNewNames(long nEl)
{
  // configure dateMemory level, names, etc.
  writer_->addField("randVal",nValues);
  writer_->addField("const");
  // ...

  allocVec(nValues+1);
  return 1;
}

/*
int cExampleSource::myFinaliseInstance()
{
  return cDataSource::myFinaliseInstance();
}
*/

eTickResult cExampleSource::myTick(long long t)
{
  SMILE_IDBG(4,"tick # %i, writing value vector",t);

  // ensure there is enough space in the destination level
  if (!writer_->checkWrite(1))
    return TICK_DEST_NO_SPACE;

  // todo: fill with random values...
  vec_->data[0] = (FLOAT_DMEM)t+(FLOAT_DMEM)3.3;
  writer_->setNextFrame(vec_);
  
  return TICK_SUCCESS;
}


cExampleSource::~cExampleSource()
{
  // ..
}
