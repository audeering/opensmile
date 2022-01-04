/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:
 ***
 ***(c) 2014-2015 audEERING UG. All rights reserved.
 ***
 ***Fixed and updated by Florian Eyben, fe@audeering.com
 ***
 * written by Bernd Huber, 2014
 * berndbhuber@gmail.com
---
reads openSL audio buffer input into datamemory
Still under development, data can be corrupt depending on platform!!!
 */


#include <android/openslesSource.hpp>
#define MODULE "cOpenslesSource"

#ifdef __ANDROID__
#ifndef __STATIC_LINK
#ifdef HAVE_OPENSLES
SMILECOMPONENT_STATICS(cOpenslesSource)

SMILECOMPONENT_REGCOMP(cOpenslesSource)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_COPENSLESSOURCE;
  sdescription = COMPONENT_DESCRIPTION_COPENSLESSOURCE;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")

  SMILECOMPONENT_IFNOTREGAGAIN(
      ct->setField("monoMixdown", "Mix down all channels to 1 mono channel (1=on, 0=off)", 1);
  ct->setField("sampleRate","Set/force the sampling rate that is assigned to the input data.",16000,0,0);
  ct->setField("channels","Set/force the number of input (recording) channels",1,0,0);
  ct->setField("outFieldName", "Set the name of the output field, containing the pcm data", "pcm");
  //ct->setField("selectChannel","Select only the specified channel from 'channels' that are recorded. Set to -1 to grab all channels.",-1);
  ct->setField("nBits","The number of bits per sample and channel",16);
  ct->setField("nBPS","The number of bytes per sample and channel (0=determine automatically from nBits)",0,0,0);
  ct->setField("audioBuffersize","The size of the portaudio recording buffer in samples (overwrites audioBuffersize_sec, if set)",0,0,0);
  ct->setField("audioBuffersize_sec","size of the portaudio recording buffer in seconds. This value has influence on the system latency. Setting it too high might introduce a high latency. A too low value might lead to dropped samples and reduced performance.",0.05);
  ct->setField("agcEnabled", "1/0 = enable automatic gain control for input audio", 0);
  ct->setField("agcTarget", "Target peak level for AGC, if enabled.", 0.5);
  ct->setField("agcDebug", "1/0 = enable agc debugging output to log.", 0);
  ct->setField("microphoneSource", "Android microphone source. One of: \n"
      "    DEFAULT (typically with AGC and noise reduction, device specific),\n"
      "    CAMCORDER,\n"
      "    VOICE_RECOGNITION (unprocessed)", "DEFAULT");
  )

  SMILECOMPONENT_MAKEINFO(cOpenslesSource);
}

SMILECOMPONENT_CREATE(cOpenslesSource)

//-----

cOpenslesSource::cOpenslesSource(const char *_name) :
cDataSource(_name),
audioBuffersize_(0),
audioBuffersize_sec_(0),
audioDevice_(NULL),
sampleRate_(0),
warned_(false),
threadStarted_(false),
threadActive_(false),
threadMatrix_(NULL),
outFieldName_(NULL),
blockSizeReader_(0), minBlockSizeReader_(1),
agcGainMin_(0.02), agcGainMax_(20.0),
agcGain_(1.0), agcPeakLp_(0.0),
agcErrorIntegrated_(0.0),
agcTarget_(0.5),
agcDebugCounter_(0),
agcDebug_(false)
{
  smileCondCreate(threadCondition_);
  smileMutexCreate(dataFlagMutex_);
  smileMutexCreate(threadMutex_);
}

void cOpenslesSource::myFetchConfig()
{	
  cDataSource::myFetchConfig();
  agcTarget_ = getDouble("agcTarget");
  agcDebug_ = (getInt("agcDebug") == 1);
  monoMixdown_ = getInt("monoMixdown");
  if (isSet("audioBuffersize")) {
    audioBuffersize_ = getInt("audioBuffersize");
  }
  if (isSet("audioBuffersize_sec")) {
    audioBuffersize_sec_ = getDouble("audioBuffersize_sec");
  }
  sampleRate_ = getInt("sampleRate");
  if (sampleRate_ <= 0) {
    SMILE_IERR(1, "sampleRate (%i) must be > 0! Setting to 16000.", sampleRate_);
    sampleRate_ = 16000;
  }
  nChannels_ = getInt("channels");
  if (nChannels_ < 1) {
    SMILE_IERR(1, "channels (%i) must be >= 1. Setting to 1.", nChannels_);
    nChannels_ = 1;
  }
  agcEnabled_ = (getInt("agcEnabled") == 1);
  const char * microphoneSource = getStr("microphoneSource");
  microphoneSource_ = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
  if (!strncmp(microphoneSource, "VOICE_RECOGNITION", 17)) {
    microphoneSource_ = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
  } else if (!strncmp(microphoneSource, "CAMCORDER", 9)) {
    microphoneSource_ = SL_ANDROID_RECORDING_PRESET_CAMCORDER;
  } else if (!strncmp(microphoneSource, "DEFAULT", 7)) {
    microphoneSource_ = SL_ANDROID_RECORDING_PRESET_GENERIC;
  }
  /*
  selectChannel_ = getInt("selectChannel");
  if (selectChannel_ >= 0) {
    SMILE_IMSG(2, "only selected %i. channel.", selectChannel_);
  }*/
  nBits_ = getInt("nBits");
  nBPS_ = getInt("nBPS");
  if (nBPS_ == 0) {
    switch (nBits_) {
    case 8: nBPS_=1; break;
    case 16: nBPS_=2; break;
    case 24: nBPS_=4; break;
    case 32: nBPS_=4; break;
    case 33: nBPS_=4; break;
    case 0:  nBPS_=4;
    nBits_=32;
    break;
    default:
      SMILE_IERR(1, "invalid number of bits requested: %i (allowed: 8, 16, 24, 32, 33(for 23-bit float))\n   Setting number of bits to default (16)", nBits_);
      nBits_ = 16;
    }
  }
  if (nBPS_ != 2) {
    SMILE_IERR(1, "This openSLES interface currently only supports 16-bit (2-byte) per sample PCM formats!");
    COMP_ERR("aborting");
  }
  outFieldName_ = getStr("outFieldName");
}

bool cOpenslesSource::setupAudioDevice() {
  // Opensl initialization for recording
  SMILE_IMSG(4, "calling setup audio sr %i, nCh %i, abfs %i", sampleRate_, nChannels_, audioBuffersize_);
  if (audioDevice_ == NULL) {
    audioDevice_ = android_OpenAudioDevice(sampleRate_,
        nChannels_, 2, audioBuffersize_,
        false /* playback */, true /* record */);
    if (audioDevice_ != NULL) {
      SMILE_IMSG(3, "Opened OpenSLES audio device.");
      return true;
    } else {
      SMILE_IERR(1, "Failed to open OpenSLES audio device! Check your audio settings, sample rate etc.");
      return false;
    }
  } else {
    return true;
  }
}

bool cOpenslesSource::startRecordingThread() {
  if (!threadStarted_) {
    // thread should only be created once, configureWrite could be called multiple times
    threadStarted_ = true;
    smileThreadCreate(recordingThread_, recordingThreadEntry, this);
  }
  return true;
}

int cOpenslesSource::configureWriter(sDmLevelConfig &c)
{
  double TT = 1.0;
  c.T = 1.0/(double)sampleRate_;
  if (c.T != 0.0)
    TT = c.T;
  if (audioBuffersize_ > 0) {
    c.blocksizeWriter = audioBuffersize_;
    audioBuffersize_sec_ = (double)audioBuffersize_ * TT;
  } else if (audioBuffersize_sec_ > 0.0) {
    c.blocksizeWriter = blocksizeW_ = audioBuffersize_ = (long)ceil(audioBuffersize_sec_ / TT);
  } else {
    if (audioBuffersize_ != 1024) {
      SMILE_IMSG(3,"using default audioBuffersize (1024 samples) since no option was given in config file");
      audioBuffersize_ = 1024;
    }
    c.blocksizeWriter = audioBuffersize_;
    audioBuffersize_sec_ = (double)audioBuffersize_ * TT;
  }
  c.noTimeMeta = true;
  blocksizeW_ = audioBuffersize_;
  blocksizeW_sec = audioBuffersize_sec_;
  if (setupAudioDevice()) {
    nChannelsEffective_ = (monoMixdown_ || selectChannel_ ? 1 : nChannels_);
    c.N = nChannelsEffective_;
    SMILE_IMSG(3, "nChannelsEffective = %i", nChannelsEffective_);
    return 1;
  } else {
    return 0;
  }
}

int cOpenslesSource::myConfigureInstance()
{
  return cDataSource::myConfigureInstance();
}

int cOpenslesSource::setupNewNames(long nEl)
{
  SMILE_IMSG(4, "calling setup new names: name = %s ; matrix rows: %i", outFieldName_, nChannelsEffective_);
  writer_->addField(outFieldName_, nChannelsEffective_);
  namesAreSet_ = 1;
  return nChannelsEffective_;
}

int cOpenslesSource::myFinaliseInstance() {
  int ret = cDataSource::myFinaliseInstance();
  if (ret) {
    if (threadMatrix_ == NULL)
      threadMatrix_ = new cMatrix(nChannelsEffective_, audioBuffersize_ / nChannelsEffective_, true);
  }
  return ret;
}

/*
int cOpenslesSource::pauseEvent()
{
  stopRecording();
  return 1;
}

void cOpenslesSource::resumeEvent()
{
  startRecording();
}
 */

bool cOpenslesSource::notifyEmptyTickloop() {
  smileMutexLock(threadMutex_);
  bool threadStarted = threadStarted_;
  smileMutexUnlock(threadMutex_);
  // we must check threadStarted and NOT threadActive here,
  // to avoid exiting when thread is just about starting
  if (!threadStarted) {
    return false;
  }
  smileCondTimedWait(threadCondition_, 1.000);
  smileMutexLock(dataFlagMutex_);
  if (dataFlag_) {
    dataFlag_ = false;
    smileMutexUnlock(dataFlagMutex_);
    // return 1 (true) when data was received from audio device since last tick
    return true;
  }
  smileMutexUnlock(dataFlagMutex_);
  smileMutexLock(threadMutex_);
  bool threadActive = threadActive_;
  smileMutexUnlock(threadMutex_);
  if (threadActive) {
    return true;
  } else {
    // return false, when no data received for 1 second
    return false;
  }
}

eTickResult cOpenslesSource::myTick(long long t)
{
  if (isPaused() || isEOI())
    return TICK_INACTIVE;
  if (!startRecordingThread())
    return TICK_INACTIVE;

  if (blockSizeReader_ <= 0) {
    const sDmLevelConfig *c = writer_->getLevelConfig();
    if (c != NULL) {
      blockSizeReader_ = c->blocksizeReader;
      minBlockSizeReader_ = c->minBlocksizeReader;
      SMILE_IMSG(4, "blockSizeReader = %i, blockSizeWriter = %i",
          c->blocksizeReader, c->blocksizeWriter);
    }
  }
  //SMILE_IMSG(4, "minBlockSizeReader = %i", minBlockSizeReader_);
  //if ((writer_->checkWrite(audioBuffersize_))
  //    && (writer_->getNAvail() < minBlockSizeReader_)) {
    //SMILE_IMSG(4, "tick wait()");
    //smileCondTimedWait(threadCondition_, 1.000);  // TODO: set timeout to 2 * audioBuffersize
    // CHECK: The logic of timeout depends on the buffersize....
    //  if one reader only reads "1" sample in each tick but a second one reads 500 samples (blockSizeReader = 500)
    //   and we receive 1000 samples when threadCond is signalled,
    //   then something will eventually block!
    //  in the example config this is due to the waveSink component (500 samples vs. 1024 samples)!
    // ******** DONE: we would need a minBlockSizeReader variable!!! ***********
    // This part could still have problems. This component should know how many ticks are needed for all
    // the subsequent components. Being in sleep mode here will affect them by blocking the whole process
    // while data is not arrived
    //return TICK_ACTIVE;
  //} else {
  if (!writer_->checkWrite(audioBuffersize_ * 2)) {
    if (!warned_) {
      SMILE_IWRN(3, "Output buffer FULL! Processing seems to be too slow. Audio data possibly lost!\n NOTE: this warning will appear only once!");
      warned_ = true;
    }
    return TICK_DEST_NO_SPACE;
  } else {
    if (warned_) {
      SMILE_IWRN(3, "Output buffer has space again. Processing back to normal.\n");
    }
    warned_ = false;
  }
  // TODO: CHECK:  processing in multi-thread hangs after a while?
  //}
  
  smileMutexLock(dataFlagMutex_);
  if (dataFlag_) {
    dataFlag_ = false;
    smileMutexUnlock(dataFlagMutex_);
    // return TICK_SUCCESS when data was received from audio device since last tick
    return TICK_SUCCESS;
  }
  smileMutexUnlock(dataFlagMutex_);
  // return TICK_INACTIVE if no data was received.
  // Here actually since it is android, we expect to get data all the time.
  // But this should be fixed.
  return TICK_INACTIVE;
}

// TODO: must be called if terminated via component manager! on = 0.. ?
void cOpenslesSource::terminateAudioRecordingThread() {
  //   1. set threadActive_ = 0 to terminate the thread (while having mutex threadMutex locked, but unlock after)
  smileMutexLock(threadMutex_);
  bool threadActive = threadActive_;
  threadActive_ = false;
  smileMutexUnlock(threadMutex_);
  if (threadActive) {  // old state. Do not signal and wait if thread was not active
    //   2. notify the audioDevice_->recorderCondition_ to unlock the recorder thread waiting for new audio
    smileCondSignal(audioDevice_->recorderCondition_);
    // join thread: wait until it terminates
    smileThreadJoin(recordingThread_);
  }
}

cOpenslesSource::~cOpenslesSource() {
  SMILE_IMSG(3, "Waiting for audio recording thread to finish.");
  terminateAudioRecordingThread();
  if (threadMatrix_ != NULL)
    delete threadMatrix_;
  android_CloseAudioDevice(audioDevice_);
  smileCondDestroy(threadCondition_);
  smileMutexDestroy(dataFlagMutex_);
  smileMutexDestroy(threadMutex_);
  SMILE_IMSG(3, "cleanup successful");
}

//--------------------------------------------------------------------------------openSLES code from here

SMILE_THREAD_RETVAL cOpenslesSource::recordingThreadEntry(void *param)
{
  cOpenslesSource *comp = (cOpenslesSource*)param;  
  // ensures log messages created in this thread are sent to the global logger
  comp->getLogger()->useForCurrentThread();
  //SMILE_IMSG(4,"Starting Audio Source Thread in OpenSL");
  comp->recordingThreadLoop();
  SMILE_THREAD_RET;
}

// FLOAT_DMEM peakLp = 0;
// FLOAT_DMEM agcGain = 1;
// FLOAT_DMEM agcGainMin = 0.02;
// FLOAT_DMEM agcGainMax = 20.0;

void cOpenslesSource::performAgc(cMatrix * samples)
{
  if (samples == NULL)
    return;
  // apply gain to audio buffer
  for (int i = 0; i < samples->nT; i++) {
    samples->data[i] *= agcGain_;
  }
  // get frame peak
  FLOAT_DMEM peak = 0.0;
  for (int i = 0; i < samples->nT; i++) {
    if (fabs(samples->data[i]) > peak) {
      peak = fabs(samples->data[i]);
    }
  }
  // lowpass of peak and delta
  FLOAT_DMEM peakLpLast = agcPeakLp_;
  //agcPeakLp_ = 0.995 * agcPeakLp_ + 0.005 * peak;
  if (peak > agcPeakLp_) {
    agcPeakLp_ = 0.4 * agcPeakLp_ + 0.6 * peak;
  } else {
    agcPeakLp_ = 0.995 * agcPeakLp_ + 0.005 * peak;
  }
  // compute current error and integrate error if not outside gain bounds
  FLOAT_DMEM error = agcTarget_ - agcPeakLp_ ;
  if (agcGain_ < agcGainMax_ && agcGain_ > agcGainMin_)
    agcErrorIntegrated_ += error;
  FLOAT_DMEM errorDelta = error - (agcTarget_ - peakLpLast);
  // compute PID output
  FLOAT_DMEM Kp = 0.1;
  FLOAT_DMEM Ki = 0.0;
  FLOAT_DMEM Kd = 0.1;
  FLOAT_DMEM update = Kp * error + Ki * agcErrorIntegrated_ + Kd * errorDelta;
  // peakTarget = update + peakCurrent;
  // == >  peakTarget = peakCurrent * (1 + update/peakCurrent);
  //       peakTarget = peakCurrent * gain
  agcGain_ = 1.0 + update/agcPeakLp_;
  // limit gain
  if (agcGain_ > agcGainMax_)
    agcGain_ = agcGainMax_;
  if (agcGain_ < agcGainMin_)
    agcGain_ = agcGainMin_;
  if (agcDebug_) {
    agcDebugCounter_++;
    if (agcDebugCounter_ == 100) {
      SMILE_IMSG(1, "agcGain: %.4f, agcPeakLp: %.4f, update: %.4f, error: %.4f, errorIntegrated: %.4f",
          agcGain_, agcPeakLp_, update, error, agcErrorIntegrated_);
      agcDebugCounter_ = 0;
    }
  }
}

void cOpenslesSource::recordingThreadLoop()
{
  //int16_t * inbuffer = (int16_t *)calloc(1, sizeof(int16_t) * audioBuffersize_);
  smileMutexLock(threadMutex_);
  threadActive_ = true;
  int threadActive = threadActive_;
  smileMutexUnlock(threadMutex_);
  SMILE_IMSG(3, "OpenSLES audio recording thread initialized");
  if (audioDevice_ == NULL) {
    SMILE_IERR(2, "No openSLES audio device opened, terminating openSLES audio recording thread.");
    //free(inbuffer);
    return;
  }
  if (openSLstartRecording(audioDevice_) != SL_RESULT_SUCCESS) {
    SMILE_IERR(1, "Failed to start recording, terminating openSLES audio recording thread.");
    //free(inbuffer);
    return;
  }
  sWaveParameters pcmParam;
  pcmParam.nBits = nBits_;
  pcmParam.nBPS = nBPS_;
  pcmParam.nChan = nChannels_;
  // NOTE: we open device before tick-loop to know whether the settings are supported
  // However we need to check what happens to audio that is recorded while configuration is still going
  // buffers discarded?
  // TERMINATING:
  //   1. set threadActive_ = 0 to terminate the thread (while having mutex threadMutex locked, but unlock after)
  //   2. notify the audioDevice_->recorderCondition_ to unlock the recorder thread waiting for new audio
  //   3.
  while (threadActive) {
    android_AudioIn2(audioDevice_, &pcmParam);
    //SMILE_IMSG(4, "read nSamples = %i in thread.", nSamples);
    // update the thread active status
    smileMutexLock(threadMutex_);
    threadActive = threadActive_;
    smileMutexUnlock(threadMutex_);
  }
  SMILE_IMSG(3, "OpenSLES thread was signaled to end.");
  openSLstopRecording(audioDevice_);
  SMILE_IMSG(4, "Recording stopped.");
  //free(inbuffer);
  smileMutexLock(threadMutex_);
  threadActive_ = false;
  threadStarted_ = false;
  smileMutexUnlock(threadMutex_);
}



// open the android audio device for input and/or output
opensl_stream2 * cOpenslesSource::android_OpenAudioDevice(int sr,
    int inchannels, int outchannels, int bufferSize,
    bool openPlay, bool openRecord) {
  opensl_stream2 *p;
  p = (opensl_stream2 *) calloc(sizeof(opensl_stream2), 1);
  p->hasNewInputData_ = 0;
  p->hasNewInternalData_ = 0;
  if (bufferSize <= 0) {
    SMILE_IERR(1, "bufferSize must be > 0 in android_OpenAudioDevice()");
    android_CloseAudioDevice(p);
    return NULL;
  }
  smileCondCreate(p->recorderCondition_);
  smileCondCreate(p->playerCondition_);
  smileMutexCreate(p->newDataMutex_);
  p->sr = sr;  // sampling rate assignment
  if (openRecord) {
    p->inchannels = inchannels;
    p->inBufSamples = bufferSize * inchannels;
    p->inBufMSeconds = (int)floor(audioBuffersize_sec_ * 1000.0);  // TODO: compute from bufferSize and sample period!
    SMILE_IMSG(1, "audioBuffersize_sec_ %i",p->inBufMSeconds);
    if (p->inBufSamples != 0) {
      for (int i = 0; i < NUM_RECORDING_BUFFERS; ++i) {
        p->deviceInputBuffers[i] = (int16_t *)calloc(p->inBufSamples, sizeof(int16_t));
        if (p->deviceInputBuffers[i] == NULL) {
          SMILE_IERR(1, "Failed to allocate device audio recording buffer memory # %i.", i);
          android_CloseAudioDevice(p);
          return NULL;
        }
      }
      for (int i = 0; i < NUM_RECORDING_DBL_BUFFERS; ++i) {
        p->internalInputBuffers[i] = (int16_t *)calloc(p->inBufSamples, sizeof(int16_t));
        if (p->internalInputBuffers[i] == NULL) {
          SMILE_IERR(1, "Failed to allocate internal audio recording buffer memory # %i.", i);
          android_CloseAudioDevice(p);
          return NULL;
        }
      }
    }
  }
  if (openPlay) {
    p->outchannels = outchannels;
    p->outBufSamples = bufferSize * outchannels;
    if (p->outBufSamples != 0) {
      for (int i = 0; i < NUM_PLAYBACK_BUFFERS; ++i) {
        p->deviceOutputBuffers[i] = (int16_t *)calloc(p->outBufSamples, sizeof(int16_t));
        if (p->deviceOutputBuffers[i] == NULL) {
          SMILE_IERR(1, "Failed to allocate audio playback buffer memory # %i.", i);
          android_CloseAudioDevice(p);
          return NULL;
        }
      }
    }
    p->currentDeviceOutputBuffer = 0;
  }

  if (openSLCreateEngine(p) != SL_RESULT_SUCCESS) {
    SMILE_IERR(2, "Failed to create openSL engine.");
    android_CloseAudioDevice(p);
    return NULL;
  }
  if (openRecord) {
    p->microphoneSource_ = microphoneSource_;
    if (openSLRecOpen(p) != SL_RESULT_SUCCESS) {
      SMILE_IERR(2, "Failed to open openSL audio recording.");
      android_CloseAudioDevice(p);
      return NULL;
    }
  }
  if (openPlay) {
    if (openSLPlayOpen(p) != SL_RESULT_SUCCESS) {
      SMILE_IERR(2, "Failed to open openSL audio playback.");
      android_CloseAudioDevice(p);
      return NULL;
    }
  }
  p->time = 0.0;
  return p;
}

// close the android audio device
void cOpenslesSource::android_CloseAudioDevice(opensl_stream2 *p) {
  if (p == NULL)
    return;
  openSLDestroyEngine(p);  // destroys player, recorder, mix, and engine
  smileCondDestroy(p->recorderCondition_);
  smileCondDestroy(p->playerCondition_);
  smileMutexDestroy(p->newDataMutex_);
  for (int i = 0; i < NUM_RECORDING_BUFFERS; ++i) {
    if (p->deviceInputBuffers[i] != NULL) {
      free(p->deviceInputBuffers[i]);
      p->deviceInputBuffers[i] = NULL;
    }
  }
  for (int i = 0; i < NUM_RECORDING_DBL_BUFFERS; ++i) {
    if (p->internalInputBuffers[i] != NULL) {
      free(p->internalInputBuffers[i]);
      p->internalInputBuffers[i] = NULL;
    }
  }
  for (int i = 0; i < NUM_PLAYBACK_BUFFERS; ++i) {
    if (p->deviceOutputBuffers[i] != NULL) {
      free(p->deviceOutputBuffers[i]);
      p->deviceOutputBuffers[i] = NULL;
    }
  }
  free(p);
}

// returns timestamp of the processed stream
double cOpenslesSource::android_GetTimestamp(opensl_stream2 *p){
  return p->time;
}

void cOpenslesSource::android_AudioIn2(opensl_stream2 *p, const sWaveParameters * pcmParam) {
  if (p->hasNewInternalData_ > 0) {
    //SMILE_IWRN(1, "hasNewInternalData_ %d, p->internalBufferCurR %d, p->internalBufferCurW %d",
        //p->hasNewInternalData_, p->internalBufferCurR, p->internalBufferCurW);
    int16_t * currentBuffer = p->internalInputBuffers[p->internalBufferCurR];
    threadMatrix_->nT = smilePcm_convertSamples((uint8_t *)currentBuffer,
        pcmParam, threadMatrix_->data, nChannels_,
        p->inBufSamples / nChannels_, monoMixdown_);
    // indicate to the callback in an atomic operation that another
    // internal buffer is free
    __sync_fetch_and_sub(&(p->hasNewInternalData_), 1);
    // our read counter is only used in our thread,
    // so no synchronization needed
    p->internalBufferCurR = (p->internalBufferCurR + 1)
        % NUM_RECORDING_DBL_BUFFERS;
    if (agcEnabled_)
      performAgc(threadMatrix_);
    if (!writer_->checkWrite(threadMatrix_->nT)) {
      SMILE_IWRN(1, "Audio data was lost, processing might be too slow (lost %ld samples)",
          threadMatrix_->nT - writer_->getNFree())
      if (writer_->getNFree() > 0) {
        threadMatrix_->nT = writer_->getNFree();
      }
    }
    if (writer_->checkWrite(threadMatrix_->nT)) {
      writer_->setNextMatrix(threadMatrix_);
      smileMutexLock(dataFlagMutex_);
      dataFlag_ = true;
      smileMutexUnlock(dataFlagMutex_);
      // signal the threadCondition_ to show that there is new data.
      smileCondSignal(threadCondition_);
    }
    p->time += (double)p->inBufSamples / (double)(p->sr * p->inchannels);
  } else {
    smileCondTimedWait(p->recorderCondition_, (p->inBufMSeconds * 3.0));// / 2.0);
  }
}

// puts a buffer of size samples to the device
int cOpenslesSource::android_AudioOut(opensl_stream2 *p, float *buffer,int size){

  short *outBuffer;
  int i, bufsamps = p->outBufSamples, index = p->currentOutputIndex;
  if(p == NULL  || bufsamps ==  0)  return 0;
  outBuffer = p->deviceOutputBuffers[p->currentDeviceOutputBuffer];

  for(i=0; i < size; i++){
    outBuffer[index++] = (short) (buffer[i]*CONV16BIT);
    if (index >= p->outBufSamples) {
      smileCondTimedWait(p->playerCondition_, 500000);  // TODO: set timeout according to audioBuffersize_sec
      (*p->bqPlayerBufferQueue)->Enqueue(p->bqPlayerBufferQueue,
          outBuffer,bufsamps*sizeof(short));
      p->currentDeviceOutputBuffer = (p->currentDeviceOutputBuffer ?  0 : 1);
      index = 0;
      outBuffer = p->deviceOutputBuffers[p->currentDeviceOutputBuffer];
    }
  }
  p->currentOutputIndex = index;
  p->time += (double) size/(p->sr*p->outchannels);
  return i;
}


//----------------------------------------------openSL specific code

// creates the OpenSL ES audio engine
SLresult openSLCreateEngine(opensl_stream2 *p) {
  SLresult result;
  // create engine
  result = slCreateEngine(&(p->engineObject), 0, NULL, 0, NULL, NULL);
  if (result != SL_RESULT_SUCCESS) {
    return result;
  }
  // realize the engine
  result = (*p->engineObject)->Realize(p->engineObject, SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) {
    return result;
  }
  // get the engine interface, which is needed in order to create other objects
  result = (*p->engineObject)->GetInterface(p->engineObject, SL_IID_ENGINE, &(p->engineEngine));
  return result;
}

// opens the OpenSL ES device for output
SLresult openSLPlayOpen(opensl_stream2 *p) {
  SLresult result;
  SLuint32 sr = p->sr;
  SLuint32 channels = p->outchannels;
  if (channels){
    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    switch(sr){
    case 8000:
      sr = SL_SAMPLINGRATE_8;
      break;
    case 11025:
      sr = SL_SAMPLINGRATE_11_025;
      break;
    case 16000:
      sr = SL_SAMPLINGRATE_16;
      break;
    case 22050:
      sr = SL_SAMPLINGRATE_22_05;
      break;
    case 24000:
      sr = SL_SAMPLINGRATE_24;
      break;
    case 32000:
      sr = SL_SAMPLINGRATE_32;
      break;
    case 44100:
      sr = SL_SAMPLINGRATE_44_1;
      break;
    case 48000:
      sr = SL_SAMPLINGRATE_48;
      break;
    case 64000:
      sr = SL_SAMPLINGRATE_64;
      break;
    case 88200:
      sr = SL_SAMPLINGRATE_88_2;
      break;
    case 96000:
      sr = SL_SAMPLINGRATE_96;
      break;
    case 192000:
      sr = SL_SAMPLINGRATE_192;
      break;
    default:
      return -1;
    }
    const SLInterfaceID ids[] = {SL_IID_VOLUME};
    const SLboolean req[] = {SL_BOOLEAN_FALSE};
    result = (*p->engineEngine)->CreateOutputMix(p->engineEngine,
        &(p->outputMixObject), 1, ids, req);
    // realize the output mix
    result = (*p->outputMixObject)->Realize(p->outputMixObject, SL_BOOLEAN_FALSE);
    SLuint32 speakers = SL_SPEAKER_FRONT_CENTER;
    if(channels > 1)
      speakers = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,channels, sr,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        speakers, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};
    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, p->outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};
    // create audio player
    const SLInterfaceID ids1[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req1[] = {SL_BOOLEAN_TRUE};
    result = (*p->engineEngine)->CreateAudioPlayer(p->engineEngine, &(p->bqPlayerObject),
        &audioSrc, &audioSnk, 1, ids1, req1);
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;
    // realize the player
    result = (*p->bqPlayerObject)->Realize(p->bqPlayerObject, SL_BOOLEAN_FALSE);
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;
    // get the play interface
    result = (*p->bqPlayerObject)->GetInterface(p->bqPlayerObject, SL_IID_PLAY, &(p->bqPlayerPlay));
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;
    // get the buffer queue interface
    result = (*p->bqPlayerObject)->GetInterface(p->bqPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
        &(p->bqPlayerBufferQueue));
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;
    // register callback on the buffer queue
    result = (*p->bqPlayerBufferQueue)->RegisterCallback(p->bqPlayerBufferQueue, bqPlayerCallback, p);
    if(result != SL_RESULT_SUCCESS) goto end_openaudio;
    // set the player's state to playing
    result = (*p->bqPlayerPlay)->SetPlayState(p->bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    SMILE_MSG(3, "playback: set state = playing");
    end_openaudio:
    return result;
  }
  return SL_RESULT_SUCCESS;
}

// Open the OpenSL ES device for input
SLresult openSLRecOpen(opensl_stream2 *p) {
  SLresult result;
  SLuint32 sr = p->sr;
  SLuint32 channels = p->inchannels;
  SLuint32 presetValue0 = p->microphoneSource_;
  // default: SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
  SLuint32 presetValue = presetValue0;
  SLuint32 presetSize = 2*sizeof(SLuint32); // intentionally too big
  SMILE_MSG(3, "openslesSource: recording: slrecopen");
  if (channels){
    switch (sr){
    case 8000:
      sr = SL_SAMPLINGRATE_8;
      break;
    case 11025:
      sr = SL_SAMPLINGRATE_11_025;
      break;
    case 16000:
      sr = SL_SAMPLINGRATE_16;
      break;
    case 22050:
      sr = SL_SAMPLINGRATE_22_05;
      break;
    case 24000:
      sr = SL_SAMPLINGRATE_24;
      break;
    case 32000:
      sr = SL_SAMPLINGRATE_32;
      break;
    case 44100:
      sr = SL_SAMPLINGRATE_44_1;
      break;
    case 48000:
      sr = SL_SAMPLINGRATE_48;
      break;
    case 64000:
      sr = SL_SAMPLINGRATE_64;
      break;
    case 88200:
      sr = SL_SAMPLINGRATE_88_2;
      break;
    case 96000:
      sr = SL_SAMPLINGRATE_96;
      break;
    case 192000:
      sr = SL_SAMPLINGRATE_192;
      break;
    default:
      return -1;
    }
    SMILE_MSG(3, "openslesSource: recording: sr constant = %i", sr);
    // configure audio source
    //SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
    //   SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataLocator_IODevice loc_dev;
    loc_dev.locatorType = SL_DATALOCATOR_IODEVICE;
    loc_dev.deviceType = SL_IODEVICE_AUDIOINPUT;
    loc_dev.deviceID = SL_DEFAULTDEVICEID_AUDIOINPUT;
    loc_dev.device = NULL;
    //SLDataSource audioSrc = {&loc_dev, NULL};
    SLDataSource audioSrc;
    audioSrc.pLocator = (void *)&loc_dev;
    audioSrc.pFormat = NULL;
    // configure audio sink (this sets the actual audio format)
    SLuint32 speakers = SL_SPEAKER_FRONT_CENTER;
    if(channels > 1)
      speakers = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, channels, sr,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        speakers, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSnk = {&loc_bq, &format_pcm};
    // create audio recorder
    // (requires the RECORD_AUDIO permission in the Manifest!)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*p->engineEngine)->CreateAudioRecorder(p->engineEngine, &(p->recorderObject),
        &audioSrc, &audioSnk, 1, id, req);
    if (SL_RESULT_SUCCESS != result) {
      SMILE_MSG(3, "openslesSource: recording: failed to create AudioRecorder");
      goto end_recopen;
    }

    // Get the Android configuration interface
  /*  SLAndroidConfigurationItf configItf;
    result = (*(p->recorderObject))->GetInterface(p->recorderObject, 
        SL_IID_ANDROIDCONFIGURATION, (void *)&configItf);
    if (SL_RESULT_SUCCESS != result) {
      SMILE_MSG(3, "openslesSource: recording: failed to get ConfigurationInterface. Cannot set microphone source.");
    } else {
      // Use the configuration interface to configure the recorder before it is realized
      // _CAMCORDER _GENERIC (MIC) _NONE _VOICE_RECOGNITION
      result = (*configItf)->SetConfiguration(configItf, SL_ANDROID_KEY_RECORDING_PRESET,
          &presetValue, sizeof(SLuint32));
      if (SL_RESULT_SUCCESS != result) {
        SMILE_MSG(3, "openslesSource: recording: failed to configure microphone source");
        goto end_recopen;
      }
      presetValue = SL_ANDROID_RECORDING_PRESET_NONE;
      result = (*configItf)->GetConfiguration(configItf, SL_ANDROID_KEY_RECORDING_PRESET,
          &presetSize, (void*)&presetValue);
      if (SL_RESULT_SUCCESS != result) {
        SMILE_MSG(3, "openslesSource: recording: failed to retrieve microphone source to verify configuration");
        goto end_recopen;
      }
      if (presetValue != presetValue0) {
        SMILE_MSG(3, "openslesSource: Error setting recording preset, retrieving preset did not return expected value\n");
        goto end_recopen;
      }
      SMILE_MSG(3, "openslesSource: AudioRecorder parametrized to source preset.\n");
    }
*/
    // realize the audio recorder
    result = (*p->recorderObject)->Realize(p->recorderObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) goto end_recopen;
    // get the record interface
    result = (*p->recorderObject)->GetInterface(p->recorderObject, SL_IID_RECORD, &(p->recorderRecord));
    if (SL_RESULT_SUCCESS != result) goto end_recopen;
    // get the buffer queue interface
    result = (*p->recorderObject)->GetInterface(p->recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
        &(p->recorderBufferQueue));
    if (SL_RESULT_SUCCESS != result) goto end_recopen;
    SMILE_MSG(3, "openslesSource: recording: got interface");
    // register callback on the buffer queue
    result = (*p->recorderBufferQueue)->RegisterCallback(p->recorderBufferQueue,
        bqRecorderCallback, p);
    if (SL_RESULT_SUCCESS != result) goto end_recopen;
    SMILE_MSG(3, "openslesSource: recording: registered callback");

    end_recopen:
    return result;
  }
  return SL_RESULT_SUCCESS;
}

SLresult openSLstopRecording(opensl_stream2 *p) {
  SLresult result;
  if (p->recorderRecord != NULL) {
    // in case already recording, stop recording and clear buffer queue
    result = (*p->recorderRecord)->SetRecordState(p->recorderRecord, SL_RECORDSTATE_STOPPED);
    if (result != SL_RESULT_SUCCESS) {
      SMILE_ERR(3, "openSL: failed to set recorder state to stopped.");
      return result;
    }
    //  (void)result;
    result = (*p->recorderBufferQueue)->Clear(p->recorderBufferQueue);
    if (result != SL_RESULT_SUCCESS) {
      SMILE_ERR(3, "openSL: failed to clear recorder buffer queue.");
      return result;
    }
    SMILE_MSG(3, "openSL: recording: set state = stopped, success");
  } else {
    SMILE_ERR(1, "openSL: no recorder present, cannot stop recording");
    return -1;
  }
  return SL_RESULT_SUCCESS;
}

SLresult openSLstartRecording(opensl_stream2 *p) {
  SLresult result;
  if (p->recorderRecord != NULL) {
    // in case already recording, stop recording and clear buffer queue
    result = (*p->recorderRecord)->SetRecordState(p->recorderRecord, SL_RECORDSTATE_STOPPED);
    if (result != SL_RESULT_SUCCESS) {
      SMILE_ERR(3, "openSL: failed to set recorder state to stopped.");
      return result;
    }
    result = (*p->recorderBufferQueue)->Clear(p->recorderBufferQueue);
    if (result != SL_RESULT_SUCCESS) {
      SMILE_ERR(3, "openSL: failed to clear recorder buffer queue.");
      return result;
    }
    // the current one will be the first to be enqueued, before we wait for the first buffer to be filled
    // in audioIn. This is why we don't enqueue it here (NUM_RECORDING_BUFFERS - 1 in above loop).
    //    p->currentDeviceInputBuffer = NUM_RECORDING_BUFFERS - 1;
   
    // start recording
    result = (*p->recorderRecord)->SetRecordState(p->recorderRecord, SL_RECORDSTATE_RECORDING);
    if (result != SL_RESULT_SUCCESS) {
      SMILE_ERR(3, "openSL: failed to set recorder state to started.");
      return result;
    }
    SMILE_MSG(3, "openSL: recording: set state = recording, success");
    // for streaming recording, we enqueue at least 2 empty buffers to start things off)
    p->currentDeviceInputBuffer = 0;
    p->internalBufferCurW = 0;
    p->internalBufferCurR = 0;
    p->hasNewInternalData_ = 0;
    p->hasNewInputData_ = 0;
    for (int i = 0; i < NUM_RECORDING_BUFFERS /* - 1*/; ++i) {
      result = (*p->recorderBufferQueue)->Enqueue(p->recorderBufferQueue,
          p->deviceInputBuffers[i], p->inBufSamples * sizeof(int16_t));
      // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
      // which for this code example would indicate a programming error
      if (result != SL_RESULT_SUCCESS) {
        SMILE_ERR(3, "openSL: failed to enqueue audio buffer # %i for streaming recording.", i);
        return result;
      }
    }
  } else {
    SMILE_ERR(1, "openSL: no recorder present, cannot start recording");
    return -1;
  }
  return SL_RESULT_SUCCESS;
}

// close the OpenSL IO and destroy the audio engine
void openSLDestroyEngine(opensl_stream2 *p) {
  // destroy buffer queue audio player object, and invalidate all associated interfaces
  if (p->bqPlayerObject != NULL) {
    (*p->bqPlayerObject)->Destroy(p->bqPlayerObject);
    p->bqPlayerObject = NULL;
    p->bqPlayerPlay = NULL;
    p->bqPlayerBufferQueue = NULL;
    p->bqPlayerEffectSend = NULL;
  }
  // destroy audio recorder object, and invalidate all associated interfaces
  if (p->recorderObject != NULL) {
    (*p->recorderObject)->Destroy(p->recorderObject);
    p->recorderObject = NULL;
    p->recorderRecord = NULL;
    p->recorderBufferQueue = NULL;
  }
  // destroy output mix object, and invalidate all associated interfaces
  if (p->outputMixObject != NULL) {
    (*p->outputMixObject)->Destroy(p->outputMixObject);
    p->outputMixObject = NULL;
  }
  // destroy engine object, and invalidate all associated interfaces
  if (p->engineObject != NULL) {
    (*p->engineObject)->Destroy(p->engineObject);
    p->engineObject = NULL;
    p->engineEngine = NULL;
  }
}

// this callback handler is called every time a buffer finishes recording
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
  opensl_stream2 *p = (opensl_stream2 *) context;
  //SMILE_MSG(3, "opensles: recorder callback triggered");

  // GCC Atomic increment: p->hasNewInputData_++
  // We copy the device buffer to an internal buffer asap here...
  // Check for enough space in internal buffer
  if (p->hasNewInternalData_ < NUM_RECORDING_DBL_BUFFERS) {
    memcpy(p->internalInputBuffers[p->internalBufferCurW],
        p->deviceInputBuffers[p->currentDeviceInputBuffer],
        p->inBufSamples * sizeof(int16_t));
    // we don't need synchronisation for this, as the callback
    // is the only thread accessing this, and callbacks are not
    // called in parallel
    p->internalBufferCurW = (p->internalBufferCurW + 1)
            % NUM_RECORDING_DBL_BUFFERS;
    __sync_fetch_and_add(&(p->hasNewInternalData_), 1);    
  } else {
    // we lost data!! Can we print a warning here, or will this make things worse?
    // p->lostAudioBuffers++;
    //__sync_fetch_and_add(&(p->lostAudioBuffers), 1);
    SMILE_WRN(1, "openSL-ES: SERIOUS: Audio data lost in callback! Internal audio collector thread is too slow. (%i)",
        p->hasNewInternalData_);
  }
  // enqueue this buffer again:
  (*p->recorderBufferQueue)->Enqueue(p->recorderBufferQueue,
      p->deviceInputBuffers[p->currentDeviceInputBuffer],
      p->inBufSamples * sizeof(int16_t));
  // increase current buffer, again the callback
  // is the only thread accessing this, no sync needed
  p->currentDeviceInputBuffer = (p->currentDeviceInputBuffer + 1)
           % NUM_RECORDING_BUFFERS;

  // Signal the condition variable without mutex
  // Potentially we could signal just before the main thread goes to wait
  // but the wait has a timeout of one audio buffer, so we will be fine when we
  // have sufficiently many internal buffers...
  smileCondSignalRaw(p->recorderCondition_);

  //smileMutexLock(p->newDataMutex_);
  //smileCondSignal(p->recorderCondition_);
  //smileMutexUnlock(p->newDataMutex_);

}

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
  opensl_stream2 *p = (opensl_stream2 *) context;
  SMILE_MSG(3, "opensles: player callback triggered");
  smileCondSignal(p->playerCondition_);
}


#endif // HAVE_OPENSLES
#endif // !__STATIC_LINK
#endif // __ANDROID__
