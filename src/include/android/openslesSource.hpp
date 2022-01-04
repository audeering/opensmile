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
***by Bernd Huber, 2014
***   berndbhuber@gmail.com

reads openSL audio buffer input into datamemory

*/

#ifndef __COPENSLESSOURCE_HPP
#define __COPENSLESSOURCE_HPP

#include <core/smileCommon.hpp>
#include <core/smileThread.hpp>
#include <core/dataSource.hpp>

#ifdef __ANDROID__
#ifndef __STATIC_LINK
#ifdef HAVE_OPENSLES

#define BUILD_COMPONENT_OpenslesSource

#define COMPONENT_DESCRIPTION_COPENSLESSOURCE "This component reads opensl audio buffer input into datamemory."
#define COMPONENT_NAME_COPENSLESSOURCE "cOpenslesSource"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
//#include <pthread.h>   // check if this is really required, as we use smileThread pthread layer!

/*typedef struct threadLock_{
  pthread_mutex_t m;
  pthread_cond_t  c;
  unsigned char   s;
} threadLock;
*/

using namespace std;
#define BUFFERFRAMES 1024
#define VECSAMPS_MONO 64
#define VECSAMPS_STEREO 128
#define SR 44100
#define CONV16BIT 32768
#define CONVMYFLT (1./32768.)

#define NUM_RECORDING_DBL_BUFFERS  16    // internal lock-free double buffering buffers
#define NUM_RECORDING_BUFFERS  2   // android device (openSL ES) buffers, max. 2 supported!
#define NUM_PLAYBACK_BUFFERS   2

typedef struct opensl_stream22 {
  // engine interfaces
  SLObjectItf engineObject;
  SLEngineItf engineEngine;
  // output mix interfaces
  SLObjectItf outputMixObject;
  // buffer queue player interfaces
  SLObjectItf bqPlayerObject;
  SLPlayItf bqPlayerPlay;
  SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
  SLEffectSendItf bqPlayerEffectSend;
  // recorder interfaces
  SLObjectItf recorderObject;
  SLRecordItf recorderRecord;
  SLAndroidSimpleBufferQueueItf recorderBufferQueue;
  // buffer indexes
  //int currentInputIndex;
  int currentOutputIndex;
  // current buffer half (0, 1) - the buffer that we next read from
  int currentDeviceOutputBuffer;
  int currentDeviceInputBuffer;
  int internalBufferCurW;
  int internalBufferCurR;
  // buffers
  int16_t * deviceOutputBuffers[NUM_PLAYBACK_BUFFERS];
  int16_t * deviceInputBuffers[NUM_RECORDING_BUFFERS];
  int16_t * internalInputBuffers[NUM_RECORDING_DBL_BUFFERS];
  // size of buffers
  int outBufSamples;
  int inBufSamples;
  double inBufMSeconds;
  // locks
  smileCond  recorderCondition_;
  smileCond  playerCondition_;
  smileMutex newDataMutex_;
  // number of waiting device buffers
  int hasNewInputData_;
  // number of waiting internal cache buffers
  int hasNewInternalData_;
  double time;
  int inchannels;
  int outchannels;
  int sr;  // sampling rate
  SLuint32 microphoneSource_;
} opensl_stream2;


class cOpenslesSource : public cDataSource {
private:

  opensl_stream2 * audioDevice_;

      // if set to 1, multi-channel files will be mixed down to 1 channel
  //long curReadPos;   // in samples
  //int eof, abort; // NOTE : when setting abort, first lock the callbackMtx!!!

  long blockSizeReader_;
  long minBlockSizeReader_;

  // audio settings
  SLuint32 microphoneSource_;
  int monoMixdown_;
  int nChannels_;
  int nChannelsEffective_;   // nChannels or 1 if monoMixdown
  int sampleRate_;
  int nBits_;
  int nBPS_;
  int selectChannel_;
  long audioBuffersize_;
  double audioBuffersize_sec_;

  bool agcDebug_;
  int agcDebugCounter_;
  FLOAT_DMEM agcTarget_;
  FLOAT_DMEM agcGain_;
  FLOAT_DMEM agcPeakLp_;
  FLOAT_DMEM agcGainMin_;
  FLOAT_DMEM agcGainMax_;
  FLOAT_DMEM agcErrorIntegrated_;

  // variables for the background thread sync:
  smileMutex dataFlagMutex_;
  smileMutex threadMutex_;
  smileCond  threadCondition_;
  cMatrix *threadMatrix_;
  bool dataFlag_;
  bool threadActive_;
  bool threadStarted_;
  //int isInTickLoop_;
  bool agcEnabled_;

  // other
  bool warned_;
  const char *outFieldName_;

  // Audio recording queue thread
  smileThread recordingThread_;
  static SMILE_THREAD_RETVAL recordingThreadEntry(void *param);
  void recordingThreadLoop();

protected:
  SMILECOMPONENT_STATIC_DECL_PR

  virtual void myFetchConfig() override;
  virtual int myConfigureInstance() override;
  virtual eTickResult myTick(long long t) override;

  virtual int configureWriter(sDmLevelConfig &c) override;
  virtual int setupNewNames(long nEl) override;
  virtual int myFinaliseInstance() override;

public:
  SMILECOMPONENT_STATIC_DECL

  cOpenslesSource(const char *_name);
  void performAgc(cMatrix * samples);
  /*
  void setNewDataFlag() {
    smileMutexLock(dataFlagMtx);
    dataFlag = 1;
    lastDataCount=0;
    smileMutexUnlock(dataFlagMtx);
  }
*/

  virtual bool notifyEmptyTickloop() override;

  bool setupAudioDevice();
  bool startRecordingThread();
  void terminateAudioRecordingThread();

  //socket flags
  //int isReaderThreadStarted,isComponentConfigured,isReaderConfigured;
  //int isStillReading;
  //bool isReaderSet;

  //datamemory parameters
  //int frameN,frameSizeSec,fieldN,nChannels;
  //double blockSizeR,frameT;






  /*
	Open the audio device with a given sampling rate (sr), input and output channels and IO buffer size
	in frames. Returns a handle to the OpenSL stream
   */
  opensl_stream2 * android_OpenAudioDevice(int sr,
      int inchannels, int outchannels, int bufferframes,
      bool openPlayer, bool openRecording);
  /*
	Close the audio device
   */
  void android_CloseAudioDevice(opensl_stream2 *p);
  /* New synchronized and double buffered reading function
   * Polls buffers and writes them directly to the data memory
   * */
  void android_AudioIn2(opensl_stream2 *p, const sWaveParameters * pcmParam);
  /*
	Read a buffer from the OpenSL stream *p, of size samples. Returns the number of samples read.
   */
  int android_AudioIn(opensl_stream2 *p, int16_t * buffer, int size);
  /*
	Write a buffer to the OpenSL stream *p, of size samples. Returns the number of samples written.
   */
  int android_AudioOut(opensl_stream2 *p, float *buffer,int size);
  /*
	Get the current IO block time in seconds
   */
  double android_GetTimestamp(opensl_stream2 *p);

  virtual ~cOpenslesSource();
};

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);

SLresult openSLCreateEngine(opensl_stream2 *p);
SLresult openSLPlayOpen(opensl_stream2 *p);
SLresult openSLRecOpen(opensl_stream2 *p);
SLresult openSLstartRecording(opensl_stream2 *p);
SLresult openSLstopRecording(opensl_stream2 *p);
void openSLDestroyEngine(opensl_stream2 *p);

#endif // HAVE_OPENSLES
#endif // !__STATIC_LINK
#endif // __ANDROID__

#endif // __COPENSLESSOURCE_HPP
