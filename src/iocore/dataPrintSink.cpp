/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: prints data as text to stdout or log */


#include <iocore/dataPrintSink.hpp>

#define MODULE "cDataPrintSink"


SMILECOMPONENT_STATICS(cDataPrintSink)

SMILECOMPONENT_REGCOMP(cDataPrintSink)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CDATAPRINTSINK;
  sdescription = COMPONENT_DESCRIPTION_CDATAPRINTSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("parseable", "1 = Print parseable output to stdout. 0 = Print human readable output to stdout or log (depending on the value of useLog).", 0);
    ct->setField("useLog", "1 = Use the log for human readable output. 0 = use stdout.", 0);
    ct->setField("printTimeMeta", "1 = Include TimeMetaInfo (tmeta) in the output. 0 = do not include TimeMetaInfo.", 0);
  )

  SMILECOMPONENT_MAKEINFO(cDataPrintSink);
}

SMILECOMPONENT_CREATE(cDataPrintSink)

cDataPrintSink::cDataPrintSink(const char *_name) :
  cDataSink(_name)
{
}

void cDataPrintSink::myFetchConfig()
{
  cDataSink::myFetchConfig();
  parseable_ = getInt("parseable");
  useLog_ = getInt("useLog");
  printTimeMeta_ = getInt("printTimeMeta");

  if (parseable_) {
    if (useLog_) {
      SMILE_IERR(2, "Option useLog is not supported for parseable output");
    }
    if (printTimeMeta_) {
      SMILE_IERR(2, "Option printTimeMeta is not supported for parseable output");
    }
  }
}


int cDataPrintSink::myFinaliseInstance()
{
  int ret = cDataSink::myFinaliseInstance();
  return ret;
}


eTickResult cDataPrintSink::myTick(long long t)
{
  cVector *vec= reader_->getNextFrame();
  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;
  long vi = vec->tmeta->vIdx;
  double tm = vec->tmeta->time;
  int i;
  if (parseable_) {
    for (i=0; i<vec->N; i++) {
      printf("SMILE-RESULT::ORIGIN=dataprint::TYPE=value::COMPONENT=%s::VIDX=%ld::TIME=%f::NAME=%s::VALUE=%e\n",
        getInstName(), vi, tm, vec->name(i).c_str(), vec->data[i]);
    }
  } else {
    for (i=0; i<vec->N; i++) {
      print("  %s.%s = %f\n", reader_->getLevelName().c_str(), vec->name(i).c_str(), vec->data[i]);
    }
    if (printTimeMeta_) {
      print("  tmeta:\n");      
      print("    filled = %d\n", vec->tmeta->filled);                    
      print("    vIdx = %ld\n", vec->tmeta->vIdx);           
      print("    period = %f\n", vec->tmeta->period);       
      print("    time = %f\n", vec->tmeta->time);         
      print("    lengthSec = %f\n", vec->tmeta->lengthSec);    
      print("    framePeriod = %d\n", vec->tmeta->framePeriod);  
      print("    smileTime = %f\n", vec->tmeta->smileTime);   
    }    
  }    
  
  nWritten_++;
  
  return TICK_SUCCESS;
}


cDataPrintSink::~cDataPrintSink()
{
}

