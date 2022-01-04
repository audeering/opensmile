/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

simple mixer, which adds multiple channels (elements) to a single channel (element)

*/


#include <dspcore/monoMixdown.hpp>

#define MODULE "cMonoMixdown"

SMILECOMPONENT_STATICS(cMonoMixdown)

SMILECOMPONENT_REGCOMP(cMonoMixdown)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CMONOMIXDOWN;
  sdescription = COMPONENT_DESCRIPTION_CMONOMIXDOWN;

  // we inherit cDataProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")
  
  SMILECOMPONENT_IFNOTREGAGAIN( 
    ct->setField("normalise","1/0 = yes/no : divide by the number of channels when adding samples from multiple channels.",1);
    ct->setField("bufsize","number of samples to process at once. Choose a number >> 1 for optimal performance. Too large buffer sizes may influence the latency!",1024);
  )
  
  SMILECOMPONENT_MAKEINFO(cMonoMixdown);
}


SMILECOMPONENT_CREATE(cMonoMixdown)

//-----

cMonoMixdown::cMonoMixdown(const char *_name) :
  cDataProcessor(_name), matout(NULL)
{
}

void cMonoMixdown::myFetchConfig()
{
  cDataProcessor::myFetchConfig();

  normalise = getInt("normalise");
  SMILE_IDBG(2,"normalise = %i",normalise);
  bufsize = getInt("bufsize");
  SMILE_IDBG(2,"bufsize = %i",bufsize);

}

int cMonoMixdown::configureWriter(sDmLevelConfig &c) 
{
  reader_->setupSequentialMatrixReading(bufsize,bufsize);
  return 1; /* success */
}


int cMonoMixdown::setupNamesForField(int i, const char *name, long nEl)
{
  // add a field with a single element for each input array field
  writer_->addField(name,1);
  return 1;
}


int cMonoMixdown::myFinaliseInstance()
{
  int ret = cDataProcessor::myFinaliseInstance();
  return ret;
}


eTickResult cMonoMixdown::myTick(long long t)
{
  /* actually process data... */
  if (!writer_->checkWrite(bufsize))
    return TICK_DEST_NO_SPACE;

  // get next matrix from dataMemory
  cMatrix *mat = reader_->getNextMatrix();
  if (mat == NULL) return TICK_SOURCE_NOT_AVAIL;

  if (matout == NULL) matout = new cMatrix(mat->fmeta->N, mat->nT);

  // sum up channels
  long i,j,c;
  for (i=0; i<mat->nT; i++) {
    for (j=0; j<matout->N; j++) {
      matout->data[i*matout->N+j] = 0.0; long st = mat->fmeta->field[j].Nstart;
      for (c=0; c<mat->fmeta->field[j].N; c++) 
        matout->data[i*matout->N+j] += mat->data[i*mat->N+st+c];
      if ((mat->fmeta->field[j].N > 0)&&(normalise))
        matout->data[i*matout->N+j] /= (FLOAT_DMEM)(mat->fmeta->field[j].N);
    }
  }

  // if you create a new vector here and pass it to setNextFrame(),
  // then be sure to assign a valid tmeta info for correct timing info:
  // e.g.:
  matout->setTimeMeta(mat->tmeta);


  // save to dataMemory
  writer_->setNextMatrix(matout);

  return TICK_SUCCESS;
}


cMonoMixdown::~cMonoMixdown()
{
  // cleanup...
  if (matout != NULL) delete matout;
}

