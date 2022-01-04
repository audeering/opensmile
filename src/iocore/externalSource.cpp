/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Source for external data injection. Creates
a dataMemory level, and provides an externally
callable function which can write data matrices
to the datamemory asynchronously.

*/


#include <iocore/externalSource.hpp>
#define MODULE "cExternalSource"

SMILECOMPONENT_STATICS(cExternalSource)

SMILECOMPONENT_REGCOMP(cExternalSource)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CEXTERNALSOURCE;
  sdescription = COMPONENT_DESCRIPTION_CEXTERNALSOURCE;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("period", (const char*)NULL, 0, 0, 0);
    ct->setField("fieldNames", "Name of dataMemory fields, array.", (const char *)NULL, ARRAY_TYPE);
    ct->setField("fieldSizes", "dataMemory field sizes, array. Order must match fieldNames. Default is 1 if this array is not given. If you specify a field size for one field, you must specify it for all.", 1, ARRAY_TYPE);
    ct->setField("blocksize","The maximum size of data buffers that can be passed to this component at once in samples (overwrites blocksize_sec, if set)", 0, 0, 0);
    ct->setField("blocksize_sec","The maximum size of data buffers that can be passed to this component at once in seconds.", 0.05);
  )
  SMILECOMPONENT_MAKEINFO(cExternalSource);
}

SMILECOMPONENT_CREATE(cExternalSource)

//-----

cExternalSource::cExternalSource(const char *_name):
  cDataSource(_name),
  vectorSize_(0),
  writtenDataBuffer_(NULL),
  externalEOI_(false),
  eoiProcessed_(false)
{
  smileMutexCreate(writeDataMtx_);
}

int cExternalSource::configureWriter(sDmLevelConfig &c) 
{
  c.noTimeMeta = true;
  return 1;
}

int cExternalSource::setupNewNames(long nEl)
{
  vectorSize_ = 0;
  int nFields = getArraySize("fieldNames");
  if (nFields > 0) {
    for (int i = 0; i < nFields; i++) {
      const char * name = getStr_f(myvprint("fieldNames[%i]", i));
      int size = getInt_f(myvprint("fieldSizes[%i]", i));
      if (size < 1) {
        size = 1;
      }
      writer_->addField(name, size);
      vectorSize_ += size;
    }
  }
  namesAreSet_ = 1;
  return vectorSize_;
}

bool cExternalSource::checkWrite(int nFrames) const {
  if (!smileMutexLock(writeDataMtx_))
    return false;
  bool canWrite = !isAbort() && !isPaused() && !isEOI() && !externalEOI_ && writer_->checkWrite(nFrames) != 0;
  smileMutexUnlock(writeDataMtx_);
  return canWrite;
}

bool cExternalSource::writeData(const FLOAT_DMEM *dataMatrix, int nFrames) {
  if (!smileMutexLock(writeDataMtx_))
    return false;
  if (isAbort() || isPaused() || isEOI() || externalEOI_) {
    smileMutexUnlock(writeDataMtx_);
    return false;
  }
  // this component needs to be finalised at this point as otherwise vectorSize_ would not be initialized yet
  if (!isFinalised()) {
    SMILE_IERR(1, "cExternalSource::writeData called before component was finalised.");
    smileMutexUnlock(writeDataMtx_);
    return false;
  }
  // if the writer level has growDyn=1 set, nFrames is allowed to be larger than the blocksize
  // thus this check is disabled for now
  /*if (nFrames > blocksizeW_) {
    SMILE_IERR(1, "externalSource::writeData N (%i) > max blocksize (%i)",
        nFrames, writtenDataBuffer_->nT);
    smileMutexUnlock(writeDataMtx_);
    return false;
  }*/
  if (!writer_->checkWrite(nFrames)) {
    smileMutexUnlock(writeDataMtx_);
    return false;
  }

  // allocate/resize data buffer
  if (writtenDataBuffer_ == NULL) {
    writtenDataBuffer_ = new cMatrix(vectorSize_, nFrames, true);
  } else if (writtenDataBuffer_->nT < nFrames) {
    delete writtenDataBuffer_;
    writtenDataBuffer_ = new cMatrix(vectorSize_, nFrames, true);
  }

  memcpy(writtenDataBuffer_->data, dataMatrix, sizeof(FLOAT_DMEM) * vectorSize_ * nFrames);

  // temporarily set the size of writtenDataBuffer_ to the current amount of data to write
  long realBufferSize = writtenDataBuffer_->nT;
  writtenDataBuffer_->nT = nFrames;
  // save to data memory
  int ret = writer_->setNextMatrix(writtenDataBuffer_);
  // restore size of writtenDataBuffer_
  writtenDataBuffer_->nT = realBufferSize;

  if (!ret) {
    smileMutexUnlock(writeDataMtx_);
    return false;
  }

  signalDataAvailable();

  smileMutexUnlock(writeDataMtx_);
  return true;
}

eTickResult cExternalSource::myTick(long long t)
{
  if (isAbort() || isPaused() || isEOI()) {
    return TICK_INACTIVE;
  }

  smileMutexLock(writeDataMtx_);
  bool eoi = externalEOI_;
  smileMutexUnlock(writeDataMtx_);

  if (!eoi) {
    return TICK_EXT_SOURCE_NOT_AVAIL;
  } else if (!eoiProcessed_) {
    // After the EOI has been set, we need to return TICK_SUCCESS once
    // to account for the following corner case:
    // * The cExternalSource component is not registered at the beginning of the components list,
    //   i.e. components that depend on the input run in the tick loop before the cExternalSource component.
    // * Part of the external data is written and all components have fully processed it as far as possible.
    //   Thus, all components return TICK_SOURCE_NOT_AVAIL/TICK_INACTIVE.
    // * Right before the tick of cExternalSource is executed,
    //   more external data is written and the external EOI is set,
    //   then the tick function of cExternalSource runs.
    // If cExternalSource immediately returned TICK_INACTIVE at this point, 
    // the component manager would see all components as inactive and set the EOI condition even though 
    // there is more data available that has not been processed yet.
    // Premature entering of the EOI mode leads in general to wrong behavior of components.
    //
    // We need to return TICK_SUCCESS here, not TICK(_EXT)_SOURCE_NOT_AVAIL, because:
    // * TICK_EXT_SOURCE_NOT_AVAIL would lead to suspending of the tick loop if no other components performed more work,
    //   which is not what we want.
    // * TICK_SOURCE_NOT_AVAIL would not fix the above corner case because it would not prevent the component manager 
    //   from entering EOI mode early.
    eoiProcessed_ = true;
    return TICK_SUCCESS;
  } else {
    return TICK_INACTIVE;
  }
}

void cExternalSource::setExternalEOI()
{
  smileMutexLock(writeDataMtx_);
  externalEOI_ = true;
  // we have to call signalDataAvailable to make sure the component manager wakes up the tick loop
  // if it is currently waiting. Component manager will then call our myTick where we return 0.
  signalDataAvailable();
  smileMutexUnlock(writeDataMtx_);
}

cExternalSource::~cExternalSource()
{
  if (writtenDataBuffer_ != NULL)
    delete writtenDataBuffer_;
  smileMutexDestroy(writeDataMtx_);
}
