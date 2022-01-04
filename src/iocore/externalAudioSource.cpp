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

Provides another function which converts audio data
from several PCM formats to float -1 to +1.

*/


#include <iocore/externalAudioSource.hpp>
#define MODULE "cExternalAudioSource"

SMILECOMPONENT_STATICS(cExternalAudioSource)

SMILECOMPONENT_REGCOMP(cExternalAudioSource)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CEXTERNALAUDIOSOURCE;
  sdescription = COMPONENT_DESCRIPTION_CEXTERNALAUDIOSOURCE;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("period",(const char*)NULL,0,0,0);
    ct->setField("sampleRate","The sampling rate of the external audio input",16000);
    ct->setField("channels","The number of channels of the external audio input",1);
    ct->setField("nBits","The number of bits per sample and channel of the external audio input",16);
    ct->setField("nBPS","The number of bytes per sample and channel of the external audio input (0=determine automatically from nBits)",0,0,0);
    ct->setField("blocksize","The maximum size of audio sample buffers that can be passed to this component at once in samples (per channel, overwrites blocksize_sec, if set)", 0, 0, 0);
    ct->setField("blocksize_sec","The maximum size of sample buffers that can be passed to this component at once in seconds.", 0.05);
    ct->setField("fieldName", "Name of dataMemory field data is written to.", "pcm");
  )
  SMILECOMPONENT_MAKEINFO(cExternalAudioSource);
}

SMILECOMPONENT_CREATE(cExternalAudioSource)

//-----

cExternalAudioSource::cExternalAudioSource(const char *_name) :
  cDataSource(_name),
  sampleRate_(0),
  channels_(1),
  nBits_(16),
  nBPS_(0),
  fieldName_(NULL),
  writtenDataBuffer_(NULL),
  externalEOI_(false),
  eoiProcessed_(false)
{
  smileMutexCreate(writeDataMtx_);
}

void cExternalAudioSource::myFetchConfig()
{
  cDataSource::myFetchConfig();
  sampleRate_ = getInt("sampleRate");
  channels_ = getInt("channels");
  if (channels_ < 1)
    channels_ = 1;
  nBits_ = getInt("nBits");
  nBPS_ = getInt("nBPS");
  if (nBPS_ == 0) {
    switch(nBits_) {
      case 8: nBPS_=1; break;
      case 16: nBPS_=2; break;
      case 24: nBPS_=4; break;
      case 32: nBPS_=4; break;
      case 33: nBPS_=4; break;
      case 0:  nBPS_=4; nBits_=32; break;
      default:
        SMILE_IERR(1,"invalid number of bits requested: %i (allowed: 8, 16, 24, 32, 33 (for 32-bit float))\n   Setting number of bits to default (16)",nBits_);
        nBits_=16;
        nBPS_=2;
    }
  }
  fieldName_ = getStr("fieldName");

  memset(&pcmParam_, 0, sizeof(sWaveParameters));
  pcmParam_.sampleRate = sampleRate_;
  pcmParam_.nChan = channels_;
  pcmParam_.nBPS = nBPS_;
  pcmParam_.nBits = nBits_ == 33 ? 32 : nBits_;
  pcmParam_.byteOrder = BYTEORDER_LE;
  pcmParam_.memOrga = MEMORGA_INTERLV;
  pcmParam_.sampleType = nBits_ == 33 ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
  // remaining fields of sWaveParameters are not needed
}

int cExternalAudioSource::configureWriter(sDmLevelConfig &c)
{
  c.T = 1.0 / sampleRate_;
  c.noTimeMeta = true;
  return 1;
}

int cExternalAudioSource::setupNewNames(long nEl)
{
  writer_->addField(fieldName_, channels_);
  namesAreSet_ = 1;
  return 1;
}

bool cExternalAudioSource::checkWrite(int nBytes) const
{
  if (!smileMutexLock(writeDataMtx_))
    return false;
  if (isAbort() || isPaused() || isEOI() || externalEOI_) {
    smileMutexUnlock(writeDataMtx_);
    return false;
  }
  int nSamples = smilePcm_numberBytesToNumberSamples(nBytes, &pcmParam_);
  bool canWrite = writer_->checkWrite(nSamples) != 0;
  smileMutexUnlock(writeDataMtx_);
  return canWrite;
}

bool cExternalAudioSource::writeData(const void *data, int nBytes)
{
  if (!smileMutexLock(writeDataMtx_))
    return false;
  if (isAbort() || isPaused() || isEOI() || externalEOI_) {
    smileMutexUnlock(writeDataMtx_);
    return false;
  }
  if (!isFinalised()) {
    SMILE_IERR(1, "cExternalAudioSource::writeData called before component was finalised.");
    smileMutexUnlock(writeDataMtx_);
    return false;
  }
  int nSamples = smilePcm_numberBytesToNumberSamples(nBytes, &pcmParam_);
  // if the writer level has growDyn=1 set, nSamples is allowed to be larger than the blocksize
  // thus this check is disabled for now
  /*if (nSamples > blocksizeW_) {
    SMILE_IERR(1, "cExternalAudioSource::writeData nSamples (%i) > max blocksize (%i). Write a smaller buffer or increase blocksize option in the configuration.",
        nSamples, blocksizeW_);
    smileMutexUnlock(writeDataMtx_);
    return false;
  }*/
  if (!writer_->checkWrite(nSamples)) {
    smileMutexUnlock(writeDataMtx_);
    return false;
  }

  // allocate/resize data buffer
  if (writtenDataBuffer_ == NULL) {
    writtenDataBuffer_ = new cMatrix(channels_, nSamples, true);
  } else if (writtenDataBuffer_->nT < nSamples) {
    delete writtenDataBuffer_;
    writtenDataBuffer_ = new cMatrix(channels_, nSamples, true);
  }

  if (nBits_ == 33) {
    if (!smilePcm_convertFloatSamples(data, &pcmParam_, writtenDataBuffer_->data, channels_, nSamples, 0)) {
      smileMutexUnlock(writeDataMtx_);
      return false;
    }
  } else {
    if (!smilePcm_convertSamples(data, &pcmParam_, writtenDataBuffer_->data, channels_, nSamples, 0)) {
      smileMutexUnlock(writeDataMtx_);
      return false;
    }
  }

  // temporarily set the size of writtenDataBuffer_ to the current amount of data to write
  long realBufferSize = writtenDataBuffer_->nT;
  writtenDataBuffer_->nT = nSamples;
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

eTickResult cExternalAudioSource::myTick(long long t)
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
    // * The cExternalAudioSource component is not registered at the beginning of the components list,
    //   i.e. components that depend on the input run in the tick loop before the cExternalAudioSource component.
    // * Part of the external data is written and all components have fully processed it as far as possible.
    //   Thus, all components return TICK_SOURCE_NOT_AVAIL/TICK_INACTIVE.
    // * Right before the tick of cExternalAudioSource is executed,
    //   more external data is written and the external EOI is set,
    //   then the tick function of cExternalAudioSource runs.
    // If cExternalAudioSource immediately returned TICK_INACTIVE at this point, 
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

void cExternalAudioSource::setExternalEOI()
{
  smileMutexLock(writeDataMtx_);
  externalEOI_ = true;
  // we have to call signalDataAvailable to make sure the component manager wakes up the tick loop
  // if it is currently waiting. Component manager will then call our myTick where we return TICK_INACTIVE.
  signalDataAvailable();
  smileMutexUnlock(writeDataMtx_);
}

cExternalAudioSource::~cExternalAudioSource()
{
  if (writtenDataBuffer_ != NULL)
    delete writtenDataBuffer_;
  smileMutexDestroy(writeDataMtx_);
}
