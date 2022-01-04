/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example dataSink:
reads data from data memory and outputs it to console/logfile (via smileLogger)
this component is also useful for debugging

*/


#include <examples/exampleSink.hpp>

#define MODULE "cExampleSink"


SMILECOMPONENT_STATICS(cExampleSink)

SMILECOMPONENT_REGCOMP(cExampleSink)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CEXAMPLESINK;
  sdescription = COMPONENT_DESCRIPTION_CEXAMPLESINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("filename","The name of a text file to dump values to (this file will be overwritten, if it exists)",(const char *)NULL);
    ct->setField("lag","Output data <lag> frames behind",0,0,0);
  )

  SMILECOMPONENT_MAKEINFO(cExampleSink);
}

SMILECOMPONENT_CREATE(cExampleSink)

//-----

cExampleSink::cExampleSink(const char *_name) :
  cDataSink(_name),
  fHandle(NULL)
{
  // ...
}

void cExampleSink::myFetchConfig()
{
  cDataSink::myFetchConfig();
  
  filename = getStr("filename");
  SMILE_IDBG(2,"filename = '%s'",filename);
  lag = getInt("lag");
  SMILE_IDBG(2,"lag = %i",lag);
}


/*
int cExampleSink::myConfigureInstance()
{
  return  cDataSink::myConfigureInstance();
  
}
*/


int cExampleSink::myFinaliseInstance()
{
  // FIRST call cDataSink myFinaliseInstance, this will finalise our dataWriter...
  int ret = cDataSink::myFinaliseInstance();

  /* if that was SUCCESSFUL (i.e. ret == 1), then do some stuff like
     loading models or opening output files: */

  if ((ret)&&(filename != NULL)) {
    fHandle = fopen(filename,"w");
    if (fHandle == NULL) {
      SMILE_IERR(1,"failed to open file '%s' for writing!",filename);
      COMP_ERR("aborting");
	    //return 0;
    }
  }

  return ret;
}


eTickResult cExampleSink::myTick(long long t)
{
  SMILE_IDBG(4,"tick # %i, reading value vector:",t);
  cVector *vec= reader_->getFrameRel(lag);  //new cVector(nValues+1);
  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;
  //else reader->nextFrame();

  long vi = vec->tmeta->vIdx;
  double tm = vec->tmeta->time;
  
  // now print the vector:
  int i;
  for (i=0; i<vec->N; i++) {
    //SMILE_PRINT("  (a=%i vi=%i, tm=%fs) %s.%s = %f",reader->getCurR(),vi,tm,reader->getLevelName().c_str(),vec->name(i).c_str(),vec->data[i]);
    printf("  %s.%s = %f\n",reader_->getLevelName().c_str(),vec->name(i).c_str(),vec->data[i]);
  }
  
  if (fHandle != NULL) {
    for (i=0; i<vec->N; i++) {
      fprintf(fHandle, "%s = %f\n",vec->name(i).c_str(),vec->data[i]);
    }
  }
  
  nWritten_++;

  return TICK_SUCCESS;
}


cExampleSink::~cExampleSink()
{
  if (fHandle != NULL) {
    fclose(fHandle);
  }
}

