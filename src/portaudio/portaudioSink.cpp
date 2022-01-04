/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

portaudio dataSink for live recording from sound device
known to work on windows, linux, and mac

*/


#include <portaudio/portaudioSink.hpp>
#define MODULE "cPortaudioSink"

#ifdef HAVE_PORTAUDIO

SMILECOMPONENT_STATICS(cPortaudioSink)

SMILECOMPONENT_REGCOMP(cPortaudioSink)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CPORTAUDIOSINK;
  sdescription = COMPONENT_DESCRIPTION_CPORTAUDIOSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("monoMixdown","mix down all channels to 1 mono channel",0);
    ct->setField("device","PortAudio device to use (device number)",-1);
    ct->setField("listDevices","(1/0=yes/no) list available portaudio devices during initialisation phase",0);
    ct->setField("sampleRate","force output sample rate (0=determine sample rate from input level)",0);
//    ct->setField("channels","number of channels to record",1);
    //ct->setField("nBits","number of bits per sample and channel to use for playback (0=pass float values to portaudio)  [ NOT IMPLEMENTED YET ]",0);
    ct->setField("audioBuffersize","size of port audio recording buffer in samples (overwrites audioBuffersize_sec, if set)",1000);
    ct->setField("audioBuffersize_sec","size of port audio recording buffer in seconds",0.05);
  )
  SMILECOMPONENT_MAKEINFO(cPortaudioSink);
}

SMILECOMPONENT_CREATE(cPortaudioSink)

//-----

cPortaudioSink::cPortaudioSink(const char *_name) :
  cDataSink(_name),
  audioBuffersize(-1),
  audioBuffersize_sec(-1.0),
  eof(0),  abort(0),
  monoMixdown(0),
  deviceId(0),
  listDevices(0),
  sampleRate(0),
  channels(1),
  dataFlag(1),
  nBits(16),
  isPaInit(0),
  callbackMatrix(NULL),
  lastDataCount(0),
  stream(NULL),
  streamStatus(PA_STREAM_STOPPED)
{
  //smileMutexCreate(dataFlagMtx);
  smileMutexCreate(callbackMtx);
  smileCondCreate(callbackCond);
}

void cPortaudioSink::myFetchConfig()
{
  cDataSink::myFetchConfig();
  
  monoMixdown = getInt("monoMixdown");
  if (monoMixdown) { SMILE_IDBG(2,"monoMixdown enabled!"); }

  if (isSet("audioBuffersize")) {
    audioBuffersize = getInt("audioBuffersize");
    if (audioBuffersize < 1) audioBuffersize=1;
    SMILE_IDBG(2,"audioBuffersize = %i",audioBuffersize);
  }
  if (isSet("audioBuffersize_sec")) {
    audioBuffersize_sec = getInt("audioBuffersize_sec");
    if (audioBuffersize_sec < 1) audioBuffersize_sec = 1;
    SMILE_IDBG(2,"audioBuffersize_sec = %i",audioBuffersize_sec);
  } 

  listDevices = getInt("listDevices");
  if (listDevices) { SMILE_IDBG(3,"listDevices enabled!"); }

  deviceId = getInt("device");
  SMILE_IDBG(2,"using portAudio device # %i",deviceId);

  sampleRate = getInt("sampleRate");
  if (sampleRate != 0) { SMILE_IDBG(2,"user-defined playback sample rate = %i",sampleRate); }

//  channels = getInt("channels");
//  if (channels < 1) channels=1;
//  SMILE_IDBG(2,"No. of recording channels = %i",channels);

  /*nBits = getInt("nBits");
  switch(nBits) {
    case 8: nBPS=1; break;
    case 16: nBPS=2; break;
    case 24: nBPS=4; break;
    case 32: nBPS=4; break;
    case 33: nBPS=4; break;
    default:
      SMILE_IERR(1,"invalid number of bits requested: %i (allowed: 8, 16, 24, 32, 33(for 23-bit float))\n   Setting number of bits to default (16)",nBits);
      nBits=16;
  }
  SMILE_IDBG(2,"No. of bits per sample = %i",nBits);
*/
  nBits = 33;
  nBPS=4;

//  // TODO: buffersize in seconds.. AND unlink buffersize = buffersize in datamemory!
}

int cPortaudioSink::configureReader()
{
  if (audioBuffersize > 0) {
    
  } else if (audioBuffersize_sec > 0) {
    double TT = reader_->getLevelT();
    if (TT == 0.0) TT = 1.0;
    audioBuffersize = (long)ceil( audioBuffersize_sec / TT );
  } else {
    SMILE_IMSG(3,"using default audioBuffersize (1000 samples) since no option was given in config file");
    audioBuffersize = 1000;
  }

  reader_->setupSequentialMatrixReading(audioBuffersize, audioBuffersize);

  return 1;
}

int cPortaudioSink::myConfigureInstance()
{
  // initialise port audio
  if (!isPaInit) {
    PaError err = Pa_Initialize(); 
    if( err != paNoError ) COMP_ERR(0,"error initialising portaudio library (code %i)\n",err);
    isPaInit=1;
  }

#ifdef HAVE_PORTAUDIO_V19
  numDevices = Pa_GetDeviceCount();
#else
  numDevices = Pa_CountDevices();
#endif

  // list devices...
  if (listDevices) {
    if (listDevices != -1) {
      printDeviceList();
      listDevices=-1;
      SMILE_IMSG(1,"Device list was printed, cPortaudioSink::myConfigureInstance will now return 0 in order to stop openSMILE initialisation!");
    }
    return 0;
  } else {
    if (numDevices <= 0) {
      SMILE_IERR(1,"NO PortAudio devices found! Cannot play live audio!");
    }
  }
  
  return cDataSink::myConfigureInstance();
}

int cPortaudioSink::myFinaliseInstance()
{
  int ret;

  ret = cDataSink::myFinaliseInstance();

  if (ret) {
    channels = reader_->getLevelN();
    if (channels > 2) {
      SMILE_IWRN(1,"channels was > 2 , it was limited to 2! This might cause problems...");
      channels=2;
    }
    if (channels < 1) {
      SMILE_IWRN(1,"channels was < 1 , it was automatically adjusted to 1! This might cause problems...");
      channels=1;
    }

	  if (sampleRate == 0) 
      sampleRate = (int)(1.0/(reader_->getLevelT()));

    SMILE_IDBG(2,"setting playback sampleRate: %i Hz",sampleRate);
    //    printf("XXXXXXXXXXXXXXx computed sampleRate as: %i Hz (per = %f)\n",sampleRate,reader->getLevelT()*1000);
  }

  return ret;
}


eTickResult cPortaudioSink::myTick(long long t)
{
  if (isEOI()) return TICK_INACTIVE; //XXX ????
  
  if (streamStatus == PA_STREAM_STOPPED) {
    dataFlag = 1;
    if (!startPlayback()) return TICK_INACTIVE;
  }

  smileMutexLock(callbackMtx);

  if (dataFlag == 0) {
    smileCondTimedWaitWMtx(callbackCond,1000,callbackMtx);
  }

  smileMutexUnlock(callbackMtx);

  return dataFlag ? TICK_SUCCESS : TICK_INACTIVE;
}


cPortaudioSink::~cPortaudioSink()
{
  if (isPaInit) {
    smileMutexLock(callbackMtx);
    stopPlaybackWait();

// TODO: set a flag to inform the callback recording has stoppped!!
//   and, do not call Pa_terminate while the callback is running... i.e. introduce a callback mutex!
    isPaInit = 0;
    PaError err = Pa_Terminate();
    if( err != paNoError ) {
      SMILE_IERR(2,"PortAudio error (Pa_Terminate): %s\n", Pa_GetErrorText( err ) );
    }
    smileMutexUnlock(callbackMtx);
  }

  //smileMutexDestroy(dataFlagMtx);
  smileMutexDestroy(callbackMtx);
  smileCondDestroy(callbackCond);
  if (callbackMatrix != NULL) delete callbackMatrix;
}

//--------------------------------------------------  portaudio specific


// channels = number of output channels (can be 1 or 2)
int matrixToPcmDataFloat(void *outputBuffer, long __N, cMatrix *_mat, int channels, int mixdown=0) 
{
  int i,c,n;

  float *out = (float*)outputBuffer;
  
  for (i=0; i<MIN(_mat->nT,__N); i++) {
	if (mixdown) {
      out[i*channels] = 0.0;
      for (c=0; c<channels; c++) {
	    for (n=0; n<_mat->N; n++) 
          out[i*channels+c] += (float)(_mat->data)[i*(_mat->N)+n];
      }
	} else {
	  int minc = channels;
	  if (_mat->N < minc) minc = _mat->N;
      for (c=0; c<channels; c++) {
        out[i*channels+c] = (float)(_mat->data)[i*(_mat->N)+c];
	  }
	}
  }
  return 1;
}

/* This routine will be called by the PortAudio engine when audio is available.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int paSink_playbackCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
#ifdef HAVE_PORTAUDIO_V19
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
#else // V18 (old API)
                           PaTimestamp outTime,
#endif
                           void *userData )
{
  cPortaudioSink * obj = (cPortaudioSink *)userData;

  if (obj == NULL) { return paAbort; }

  if (!smileMutexLock(obj->callbackMtx)) return paAbort;

  if (obj->isAbort()) {
    smileMutexUnlock(obj->callbackMtx);
    return paAbort;
  }
  if (outputBuffer==NULL) {
    smileMutexUnlock(obj->callbackMtx);
	  return paContinue;
  }

  cDataReader * reader = obj->getReader();

  cMatrix * _mat = reader->getNextMatrix();  // TODO::: if problems on some architectures are encountered... get rid of the inherent malloc in getNextMatrix...?
  
  if (_mat != NULL) {
    matrixToPcmDataFloat(outputBuffer, framesPerBuffer, _mat, obj->getChannels(), obj->isMonoMixdown());
    smileCondSignal(obj->callbackCond);
	  obj->dataFlag=1;
  } else {
    // we missed data....
	  // concealment strategy... pad with zeros
    memset(outputBuffer, 0, sizeof(float)*obj->getChannels()*framesPerBuffer);
	
	if (obj->dataFlag == 1) 
	  SMILE_MSG(3,"dropped (>=%i) frames during playback! (maybe increase audioBuffersize ?) [instance: '%s']",framesPerBuffer,obj->getInstName());
      // OR: repeat last frame...?    

	obj->dataFlag=0;
  }

  smileMutexUnlock(obj->callbackMtx);
  return paContinue;
}


int cPortaudioSink::startPlayback()
{
    PaStreamParameters  outputParameters;
    PaError             err = paNoError;

#ifdef HAVE_PORTAUDIO_V19
    if (Pa_IsStreamActive( stream ) == 1) {
#else
    if (Pa_StreamActive( stream ) == 1) {
#endif
      SMILE_IWRN(2,"portAudio stream is already active (in startPlayback).");
      return 1;
    }

    if (deviceId < 0) {
      outputParameters.device = Pa_GetDefaultOutputDevice(); /* default input device */
    } else {
      outputParameters.device = deviceId;
    }
//

    SMILE_IMSG(2,"playing on portAudio device with index %i",outputParameters.device);
    outputParameters.channelCount = channels;
	outputParameters.sampleFormat = paFloat32;

/*	switch (nBits) {
      case 8: outputParameters.sampleFormat = paInt8; break;
      case 16: outputParameters.sampleFormat = paInt16; break;
      case 24: outputParameters.sampleFormat = paInt24; break;
      case 32: outputParameters.sampleFormat = paInt32; break;
      case 33: outputParameters.sampleFormat = paFloat32; break;
      default:
        COMP_ERR("invalid number of bits requested: %i (allowed: 8, 16, 24, 32, 33(for 23-bit float))\n",nBits);
    }*/

#ifdef HAVE_PORTAUDIO_V19
    const PaDeviceInfo * info = Pa_GetDeviceInfo( outputParameters.device );
    if (info != NULL) {
      outputParameters.suggestedLatency =
        info->defaultLowInputLatency;
    } else {
      outputParameters.suggestedLatency = 0;
    }
    outputParameters.hostApiSpecificStreamInfo = NULL;
#endif

    /* Setup recording stream -------------------------------------------- */
    err = Pa_OpenStream(
              &(stream),
#ifdef HAVE_PORTAUDIO_V19
              NULL,
              &outputParameters,
#else // V18 (old API)
              paNoDevice,
              0,
              0,
              NULL,
              outputParameters.device, 
			  outputParameters.channelCount, 
			  outputParameters.sampleFormat, 
			  NULL,
#endif
              sampleRate,
              audioBuffersize,
#ifndef HAVE_PORTAUDIO_V19
              1,
#endif
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              paSink_playbackCallback,
              (void*) this );

    if( err != paNoError ) {
        COMP_ERR("error opening portaudio playback stream (code %i) \n  check samplerate(-> %i , maybe it is not supported?) \n  maybe incorrect device? (\"listDevices=1\" in config file displays a list of devices)\n",err,sampleRate);
    }

	smileMutexLock(callbackMtx);
    err = Pa_StartStream( stream );
    if( err != paNoError ) COMP_ERR("cannot start portaudio stream (code %i)\n",err);
    streamStatus = PA_STREAM_STARTED;
    smileMutexUnlock(callbackMtx);

    return 1;
}

int cPortaudioSink::stopPlayback()
{
  PaError             err = paNoError;
  if (streamStatus == PA_STREAM_STOPPED) return 0;

  streamStatus =  PA_STREAM_STOPPED;

  abort = 1;

  err = Pa_CloseStream( stream );
  if( err != paNoError ) {
    SMILE_IERR(1,"cannot close portaudio stream (code %i)\n",err);
    return 0;
  }

  return 1;
}

int cPortaudioSink::stopPlaybackWait()
{
  PaError             err = paNoError;
  if (streamStatus == PA_STREAM_STOPPED) return 0;

  streamStatus =  PA_STREAM_STOPPED;

  abort = 1;

  smileMutexUnlock(callbackMtx);
  err = Pa_StopStream( stream );
  smileMutexLock(callbackMtx);

  if( err != paNoError ) {
    SMILE_IERR(1,"cannot close portaudio stream (code %i)\n",err);
    return 0;
  }

  return 1;
}

void cPortaudioSink::printDeviceList(void)
{
  // query devices:
  SMILE_PRINT("== cPortAudioSink ==  There are %i audio devices:", numDevices );
  if( numDevices < 0 ) {
     SMILE_IERR(1, "Pa_CountDevices returned 0x%x\n", numDevices );
     return;
  }
  if( numDevices == 0 ) {
     SMILE_IERR(1, "No PortAudio audio devices were found! (Pa_CountDevices()=0)\n", numDevices );
     return;
  }

  const PaDeviceInfo *deviceInfo;

  int i;
  for( i=0; i<numDevices; i++ )
  {
     deviceInfo = Pa_GetDeviceInfo( i );
     SMILE_PRINT("  -> Device #%i: '%s'\n       #inputChan=%i #outputChan=%i\n",i,deviceInfo->name, deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels);
  }

#ifdef HAVE_PORTAUDIO_V19
  SMILE_PRINT("  The default device is #%i\n",Pa_GetDefaultOutputDevice());
#else
  SMILE_PRINT("  The default device is #%i\n",Pa_GetDefaultOutputDeviceID());
#endif

}

#endif // HAVE_PORTAUDIO
