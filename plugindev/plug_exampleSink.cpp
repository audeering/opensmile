/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example dataSink
writes data to data memory...

*/


#include <plug_exampleSink.hpp>

#define MODULE "cExampleSinkPlugin"


SMILECOMPONENT_STATICS(cExampleSinkPlugin)

SMILECOMPONENT_REGCOMP(cExampleSinkPlugin)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CEXAMPLESINK;
  sdescription = COMPONENT_DESCRIPTION_CEXAMPLESINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("filename","file to dump values to","dump.txt");
    ct->setField("lag","output data <lag> frames behind",0);
  )
  SMILECOMPONENT_MAKEINFO(cExampleSinkPlugin);
}

SMILECOMPONENT_CREATE(cExampleSinkPlugin)

//-----

cExampleSinkPlugin::cExampleSinkPlugin(const char *_name) :
  cDataSink(_name)
{
  // ...
printf("I'm here...\n");
}

void cExampleSinkPlugin::myFetchConfig()
{
  cDataSink::myFetchConfig();
printf("I'm here in myFetchConfig...\n");
  
  filename = getStr("filename");
printf("filename=%s\n",filename);
printf("log=%ld\n",(long)&smileLog);
smileLog.message(FMT("test"), 2, "cYEAH");

  SMILE_IDBG(2,"filename = '%s'",filename);
  lag = getInt("lag");
  SMILE_IDBG(2,"lag = %i",lag);
}

/*
int cExampleSink::myConfigureInstance()
{
  int ret=1;
  ret *= cDataSink::myConfigureInstance();
  // ....
  //return ret;
}
*/

/*
int cExampleSink::myFinaliseInstance()
{
  int ret=1;
  ret *= cDataSink::myFinaliseInstance();
  // ....
  //return ret;
}
*/

eTickResult cExampleSinkPlugin::myTick(long long t)
{
  SMILE_IDBG(4,"tick # %i, reading value vector:",t);
  cVector *vec= reader_->getFrameRel(lag);  //new cVector(nValues+1);
  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;
  else reader_->nextFrame();

  long vi = vec->tmeta->vIdx;
  double tm = vec->tmeta->time;
  
  // now print the vector:
  SMILE_IMSG(2,"FUCKING AWSOME PLUGIN OUTPUT:");
  int i;
  for (i=0; i<vec->N; i++) {
    printf("  (a=%ld vi=%ld, tm=%fs) %s.%s = %f\n",reader_->getCurR(),vi,tm,reader_->getLevelName().c_str(),vec->name(i).c_str(),vec->data[i]);
  }

// SMILE_PRINT("%i",var1,)

  return TICK_SUCCESS;
}


cExampleSinkPlugin::~cExampleSinkPlugin()
{
  // ...
}

