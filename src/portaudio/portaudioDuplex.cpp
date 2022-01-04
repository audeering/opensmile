/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

portaudio dataPorcessor for full-duplex simultaneous recording and playback from the same audio device
NOTE: this does not work on all sound-card devices and it does not work at all sample-rates (often only 48kHz are supported on cheap devices)
(?? known to work on windows, linux, and mac)

*/


#include <portaudio/portaudioDuplex.hpp>
#define MODULE "cPortaudioDuplex"

#ifdef HAVE_PORTAUDIO

SMILECOMPONENT_STATICS(cPortaudioDuplex)

SMILECOMPONENT_REGCOMP(cPortaudioDuplex)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CPORTAUDIODUPLEX;
  sdescription = COMPONENT_DESCRIPTION_CPORTAUDIODUPLEX;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("monoMixdownPB","mix down all channels to 1 mono channel (for playback)",0);
    ct->setField("monoMixdownREC","mix down all channels to 1 mono channel (for recording)",0);
    ct->setField("device","PortAudio device to use (device number)",-1);
    ct->setField("listDevices","(1/0=yes/no) list available portaudio devices during initialisation phase",0);
    ct->setField("sampleRate","recording AND playback sample rate (WARNING: no sample rate conversion of input level data will be performed prior to playback!)",44100);
    ct->setField("nBits","number of bits per sample and channel to use for playback (0=pass float values to portaudio)  [ NOT IMPLEMENTED YET ]",0);
    ct->setField("channels","number of channels to record",1);
    ct->setField("audioBuffersize","size of port audio playback&recording buffer in samples",1000);
    ct->setField("audioBuffersize_sec","size of port audio playback&recording buffer in seconds",0.05);
  )
  SMILECOMPONENT_MAKEINFO(cPortaudioDuplex);
}

SMILECOMPONENT_CREATE(cPortaudioDuplex)

//-----

cPortaudioDuplex::cPortaudioDuplex(const char *_name) :
cDataProcessor(_name),
//mBuffersize(2000),
audioBuffersize(-1),
audioBuffersize_sec(-1.0),
//  curReadPos(0),
eof(0),  abort(0),
monoMixdownPB(0),
monoMixdownREC(0),
deviceId(0),
listDevices(0),
sampleRate(0),
channels(1),
dataFlagR(1),
dataFlagW(1),
nBits(16),
isPaInit(0),
callbackMatrix(NULL),
lastDataCount(0),
stream(NULL),
streamStatus(PA_STREAM_STOPPED)
{
  smileMutexCreate(callbackMtx);
  smileCondCreate(callbackCond);
}

void cPortaudioDuplex::myFetchConfig()
{
  cDataProcessor::myFetchConfig();

  monoMixdownPB = getInt("monoMixdownPB");
  if (monoMixdownPB) { SMILE_IDBG(2,"monoMixdown for playback enabled!"); }
  monoMixdownREC = getInt("monoMixdownREC");
  if (monoMixdownREC) { SMILE_IDBG(2,"monoMixdown for recording enabled!"); }

  /*
  mBuffersize = getInt("buffersize");
  if (mBuffersize < 1) mBuffersize=1;
  SMILE_IDBG(2,"buffersize = %i",mBuffersize);
*/


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

  channels = getInt("channels");
  if (channels < 1) channels=1;
  SMILE_IDBG(2,"No. of recording channels = %i",channels);

  nBits = getInt("nBits");
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

  //  // TODO: buffersize in seconds.. AND unlink buffersize = buffersize in datamemory!
}

int cPortaudioDuplex::configureWriter(sDmLevelConfig &c)
{
  if (audioBuffersize > 0) {
    c.blocksizeWriter = audioBuffersize;
  } else if (audioBuffersize_sec > 0) {
    double TT = 1.0;
    if (c.T != 0.0) TT = c.T;
    c.blocksizeWriter = audioBuffersize = (long)ceil( audioBuffersize_sec / TT );
  } else {
    SMILE_IMSG(3,"using default audioBuffersize (1000 samples) since no option was given in config file");
    c.blocksizeWriter = audioBuffersize = 1000;
  }

  c.T = 1.0 / (double)(sampleRate);

  reader_->setupSequentialMatrixReading(audioBuffersize, audioBuffersize);
  
  return 1;
}

int cPortaudioDuplex::myConfigureInstance()
{
  int ret = 1;

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
      SMILE_IMSG(1,"Device list was printed, cPortaudioDuplex::myConfigureInstance will now return 0 in order to stop openSMILE initialisation!");
    }
    return 0;
  } else {
    if (numDevices <= 0) {
      SMILE_IERR(1,"NO PortAudio devices found! Cannot play live audio!");
    }
  }


  return cDataProcessor::myConfigureInstance();
}

int cPortaudioDuplex::setupNewNames(long nEl)
{
  writer_->addField("pcm",channels);
  namesAreSet_ = 1;
  return 1;
}

int cPortaudioDuplex::myFinaliseInstance()
{
  int ret = cDataProcessor::myFinaliseInstance();

  if (ret) {
    callbackMatrix = new cMatrix( getChannels(), audioBuffersize );

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


eTickResult cPortaudioDuplex::myTick(long long t)
{
  if (isEOI()) return TICK_INACTIVE; //XXX ????

  if (streamStatus == PA_STREAM_STOPPED) {
    if (!startDuplex()) return TICK_INACTIVE;
  }
/*
  smileMutexLock(callbackMtx);
  int ret = (dataFlagR&dataFlagW);
  if ((dataFlagR == 0)&&(dataFlagW == 0)) {
    smileCondTimedWaitWMtx(callbackCond,1000,callbackMtx);
    //dataFlagR = dataFlagW = 0;
  } else {
    
  }
  
  smileMutexUnlock(callbackMtx);
  
  return ret ? TICK_SUCCESS : TICK_INVALID;
  */
  smileMutexLock(callbackMtx);
  if ((dataFlagW == 0)&&(dataFlagR==0)) {
    smileCondTimedWaitWMtx(callbackCond,1000,callbackMtx);
  }
  int ret = dataFlagW|dataFlagR;
  dataFlagW=0; dataFlagR=0;
  
  // TODO: if dataFlagW == 0, then increase counter, after timeout set end of playback
  //       add option, terminate at end of playback....

  smileMutexUnlock(callbackMtx);


  return TICK_SUCCESS; // ret ? TICK_SUCCESS : TICK_INVALID;
}


cPortaudioDuplex::~cPortaudioDuplex()
{
  if (isPaInit) {
    smileMutexLock(callbackMtx);
    stopDuplex();

    // TODO: set a flag to inform the callback recording has stoppped!!
    //   and, do not call Pa_terminate while the callback is running... i.e. introduce a callback mutex!
    isPaInit = 0;
    PaError err = Pa_Terminate();
    if( err != paNoError ) {
      SMILE_IERR(2,"PortAudio error (Pa_Terminate): %s\n", Pa_GetErrorText( err ) );
    }
    smileMutexUnlock(callbackMtx);
  }

  smileMutexDestroy(callbackMtx);
  smileCondDestroy(callbackCond);
  if (callbackMatrix != NULL) delete callbackMatrix;
}

//--------------------------------------------------  portaudio specific


// channels = number of output channels (can be 1 or 2)
int matrixToPcmDataFloat_d(void *outputBuffer, long __N, cMatrix *_mat, int channels, int mixdown=0) 
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

// channels = number of output channels (can be 1 or 2)
int pcmDataFloatToMatrix_d(const void *inputBuffer, long __N, cMatrix *_mat, int channels, int mixdown=0) 
{
  int i,c,n;

  const float *in = (const float*)inputBuffer;
  FLOAT_DMEM *out = _mat->data;

  for (i=0; i<MIN(_mat->nT,__N); i++) {
    if (mixdown) {
      out[i*_mat->N] = (FLOAT_DMEM)0.0;
      for (c=0; c<channels; c++) {
        for (n=0; n<_mat->N; n++) 
          out[i*_mat->N+n] += (FLOAT_DMEM)(in[i*channels+c]);
      }
    } else {
      int minc = channels;
      if (_mat->N < minc) minc = _mat->N;
      for (c=0; c<channels; c++) {
        if (c>minc) {
          out[i*_mat->N+c] = (FLOAT_DMEM)(0.0);
        } else {
          out[i*_mat->N+c] = (float)(in[i*channels+c]);
        }
      }
    }
  }
  return 1;
}



/* This routine will be called by the PortAudio engine when audio is available.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int paDuplex_duplexCallback( const void *inputBuffer, void *outputBuffer,
                                 unsigned long framesPerBuffer,
#ifdef HAVE_PORTAUDIO_V19
                                 const PaStreamCallbackTimeInfo* timeInfo,
                                 PaStreamCallbackFlags statusFlags,
#else // V18 (old API)
                                 PaTimestamp outTime,
#endif
                                 void *userData )
{
  cPortaudioDuplex * obj = (cPortaudioDuplex *)userData;
  
  if (obj == NULL) { 
    memset(outputBuffer, 0, sizeof(float)*obj->getChannels()*framesPerBuffer);
    return paAbort; 
  }

  if (!smileMutexLock(obj->callbackMtx)) {
    memset(outputBuffer, 0, sizeof(float)*obj->getChannels()*framesPerBuffer);
    return paAbort;
  }
  if (obj->isAbort()) {
    memset(outputBuffer, 0, sizeof(float)*obj->getChannels()*framesPerBuffer);
    smileMutexUnlock(obj->callbackMtx);
    return paAbort;
  }

  #ifdef HAVE_PORTAUDIO_V19
  if (statusFlags & paInputOverflow) { SMILE_DBG(3,"paInputOverflow"); }
  if (statusFlags & paInputUnderflow) { SMILE_DBG(3,"paInputUnderflow"); }
  if (statusFlags & paOutputOverflow) { SMILE_DBG(3,"paOutputOverflow"); }
  if (statusFlags & paOutputUnderflow) { SMILE_DBG(3,"paOutputUnderflow"); }
  #endif

  cDataReader * reader = obj->getReader();
  cDataWriter * writer = obj->getWriter();

  //printf("CBM I:%i  O:%i \n",(long)inputBuffer,(long)outputBuffer);

  if (inputBuffer!=NULL) {

  
  if (writer->checkWrite(framesPerBuffer)) {
    // make matrix.. // TODO... move this "new" to the init code...!
    cMatrix * mat = obj->callbackMatrix; //new cMatrix( obj->getChannels(), framesPerBuffer );
    if (mat==NULL) {
      memset(outputBuffer, 0, sizeof(float)*obj->getChannels()*framesPerBuffer);
      smileMutexUnlock(obj->callbackMtx);
      return paAbort;
    }

    pcmDataFloatToMatrix_d(inputBuffer, framesPerBuffer, mat, obj->getChannels(), obj->isMonoMixdownREC());

    obj->dataFlagW=1;
    smileCondSignal(obj->callbackCond);

    // save to datamemory
    writer->setNextMatrix(mat);
  } else {
    obj->dataFlagW=0; // ??
    SMILE_WRN(1, "Audio data was lost, processing might be too slow (lost %lu samples). Increase data memory buffersize of cPortaudioSource components", framesPerBuffer);
  }

  } 

  if (outputBuffer != NULL) {

  cMatrix * _mat = reader->getNextMatrix();  // TODO::: if problems on some architectures are encountered... get rid of the inherent malloc in getNextMatrix...?

  if (_mat != NULL) {
    matrixToPcmDataFloat_d(outputBuffer, framesPerBuffer, _mat, obj->getChannels(), obj->isMonoMixdownPB());
    obj->dataFlagR=1;
    smileCondSignal(obj->callbackCond);
  } else {
    // we missed data....
    // concealment strategy... pad with zeros
    memset(outputBuffer, 0, sizeof(float)*obj->getChannels()*framesPerBuffer);

    if (obj->dataFlagR == 1) 
      SMILE_MSG(3,"dropped (>=%i) frames during playback! (maybe increase audioBuffersize ?) [instance: '%s']",framesPerBuffer,obj->getInstName());
    // OR: repeat last frame...?    

    obj->dataFlagR=0;
  }

  }

  smileMutexUnlock(obj->callbackMtx);

  return paContinue;
}


int cPortaudioDuplex::startDuplex()
{
  PaStreamParameters  inputParameters;
  PaStreamParameters  outputParameters;
  PaError             err = paNoError;

#ifdef HAVE_PORTAUDIO_V19
  if (Pa_IsStreamActive( stream ) == 1) {
#else
  if (Pa_StreamActive( stream ) == 1) {
#endif
    SMILE_IWRN(2,"portAudio stream is already active (in startDuplex).");
    return 1;
  }

  if (deviceId < 0) {
    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default input device */
  } else {
    inputParameters.device = deviceId;
    outputParameters.device = deviceId;
  }
  //

  SMILE_IMSG(2,"playing on portAudio device with index %i",outputParameters.device);
  SMILE_IMSG(2,"recording on portAudio device with index %i",inputParameters.device);
  inputParameters.channelCount = channels;
  inputParameters.sampleFormat = paFloat32;
  outputParameters.channelCount = channels;
  outputParameters.sampleFormat = paFloat32;

#ifdef HAVE_PORTAUDIO_V19
  const PaDeviceInfo * info = Pa_GetDeviceInfo( inputParameters.device );
  if (info != NULL) {
    inputParameters.suggestedLatency =
      info->defaultLowInputLatency;
  } else {
    inputParameters.suggestedLatency = 0;
  }
  inputParameters.hostApiSpecificStreamInfo = NULL;

  info = Pa_GetDeviceInfo( outputParameters.device );
  if (info != NULL) {
    outputParameters.suggestedLatency =
      info->defaultLowOutputLatency;
  } else {
    outputParameters.suggestedLatency = 0;
  }
  outputParameters.hostApiSpecificStreamInfo = NULL;
#endif

  /* Setup recording stream -------------------------------------------- */
  err = Pa_OpenStream(
    &stream,
#ifdef HAVE_PORTAUDIO_V19
    &inputParameters,
    &outputParameters,
#else // V18 (old API)
    inputParameters.device, 
    inputParameters.channelCount, 
    inputParameters.sampleFormat, 
    NULL,
    outputParameters.device, 
    outputParameters.channelCount, 
    outputParameters.sampleFormat, 
    NULL,
#endif
    sampleRate,
    audioBuffersize,
/*
#ifndef HAVE_PORTAUDIO_V19
    1,
#endif
    */
    paClipOff,      /* we won't output out of range samples so don't bother clipping them */
    paDuplex_duplexCallback,
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

int cPortaudioDuplex::stopDuplex()
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

int cPortaudioDuplex::stopDuplexWait()
{
  PaError             err = paNoError;
  if (streamStatus == PA_STREAM_STOPPED) return 0;

  streamStatus =  PA_STREAM_STOPPED;

  abort = 1;

  err = Pa_StopStream( stream );
  if( err != paNoError ) {
    SMILE_IERR(1,"cannot close portaudio stream (code %i)\n",err);
    return 0;
  }

  return 1;
}

void cPortaudioDuplex::printDeviceList(void)
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
  SMILE_PRINT("  The default device is #%i\n",Pa_GetDefaultInputDevice());
#else
  SMILE_PRINT("  The default device is #%i\n",Pa_GetDefaultInputDeviceID());
#endif

}

#endif // HAVE_PORTAUDIO
