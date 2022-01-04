/*F***************************************************************************
 * This file is part of openSMILE.
 *
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: externalSink

Provides a callback function to external code that gets for each frame that
this sink component reads.

*/


#include <iocore/externalSink.hpp>

#define MODULE "cExternalSink"


SMILECOMPONENT_STATICS(cExternalSink)

SMILECOMPONENT_REGCOMP(cExternalSink)
{
  SMILECOMPONENT_REGCOMP_INIT;
  scname = COMPONENT_NAME_CEXTERNALSINK;
  sdescription = COMPONENT_DESCRIPTION_CEXTERNALSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink");

  SMILECOMPONENT_IFNOTREGAGAIN( {} )

  SMILECOMPONENT_MAKEINFO(cExternalSink);
}

SMILECOMPONENT_CREATE(cExternalSink)

//-----

cExternalSink::cExternalSink(const char *_name) :
  cDataSink(_name), dataCallback(NULL), callbackParam(NULL),
  dataCallbackEx(NULL), callbackParamEx(NULL),
  numElements(0), elementNames(NULL)
{
}

void cExternalSink::myFetchConfig()
{
  cDataSink::myFetchConfig();
}

int cExternalSink::configureReader()
{
  reader_->setupSequentialMatrixReading(blocksizeR_, blocksizeR_, 0);
  return 1;
}

int cExternalSink::myFinaliseInstance()
{
  int ret = cDataSink::myFinaliseInstance();

  numElements = reader_->getLevelN();
  elementNames = new char *[numElements];
  for (long i = 0; i < numElements; i++)
  {
    elementNames[i] = reader_->getElementName(i);
  }

  if (ret) {
    // we may want to provide this info to external code, as well
    //period = reader_->getLevelT();
    //vecSize = reader_->getLevelN();
  }

  return ret;
}

eTickResult cExternalSink::myTick(long long t)
{
  cMatrix *mat = reader_->getNextMatrix(0, 0, DMEM_PAD_NONE);

  if (mat == NULL)
    return TICK_SOURCE_NOT_AVAIL;

  if (dataCallback == NULL && dataCallbackEx == NULL)
    return TICK_INACTIVE;

  if (dataCallback) {
    for (long i = 0; i < mat->nT; i++) {
      dataCallback(mat->data + i * mat->N, mat->N, callbackParam);
    }
  }

  if (dataCallbackEx) {
    sExternalSinkMetaDataEx metaData;
    metaData.vIdx = mat->tmeta[0].vIdx;
    metaData.time = mat->tmeta[0].time;
    metaData.period = mat->tmeta[0].period;
    metaData.lengthSec = mat->tmeta[0].lengthSec;
    dataCallbackEx(mat->data, mat->nT, mat->N, &metaData, callbackParamEx);
  }

  nWritten_ += mat->nT;
  return TICK_SUCCESS;
}

long cExternalSink::getNumElements()
{
  return numElements;
}

const char *cExternalSink::getElementName(long idx)
{
  return elementNames[idx];
}

void cExternalSink::setDataCallback(ExternalSinkCallback callback, void *param)
{
  dataCallback = callback;
  callbackParam = param;
}

void cExternalSink::setDataCallbackEx(ExternalSinkCallbackEx callback, void *param)
{
  dataCallbackEx = callback;
  callbackParamEx = param;
}

cExternalSink::~cExternalSink()
{
  if (elementNames)
  {
    for (long i = 0; i < numElements; i++)
    {
      free(elementNames[i]);
    }
    delete[] elementNames; elementNames = NULL;
  }
}
