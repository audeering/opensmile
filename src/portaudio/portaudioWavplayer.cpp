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


#include <portaudio/portaudioWavplayer.hpp>
#define MODULE "cPortaudioWavplayer"

#ifdef HAVE_PORTAUDIO




SMILECOMPONENT_STATICS(cPortaudioWavplayer)

SMILECOMPONENT_REGCOMP(cPortaudioWavplayer)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CPORTAUDIOWAVPLAYER;
  sdescription = COMPONENT_DESCRIPTION_CPORTAUDIOWAVPLAYER;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_CREATE_CONFIGTYPE
    //SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("monoMixdown","mix down all channels to 1 mono channel",0);
    ct->setField("device","PortAudio device to use (device number)",-1);
    ct->setField("listDevices","(1/0=yes/no) list available portaudio devices during initialisation phase",0);
    ct->setField("sampleRate","force output sample rate (0=determine sample rate from input level)",0);

    ct->setField("wavefile","The filenames of the wavesample(s) to play (triggered via smile message or numbers read from the input level).",(const char*)NULL,ARRAY_TYPE);
    ct->setField("keyword","List of keywords or classnames (in same order as wave files)",(const char*)NULL,ARRAY_TYPE);
    ct->setField("indices","List of class indicies for classification results (same order as wave files)",-1.0,ARRAY_TYPE);

    ct->setField("semaineCallbackRcpt","Reciepient of SEMAINE callback message for echo feedback supression (start speaking / stop speaking messages, i.e. when player starts playing/stops playing).",(const char*)NULL);
    ct->setField("audioBuffersize","size of port audio playback buffer in samples (overwrites audioBuffersize_sec, if set)",1000);
  )
  SMILECOMPONENT_MAKEINFO(cPortaudioWavplayer);
}

SMILECOMPONENT_CREATE(cPortaudioWavplayer)

//-----

cPortaudioWavplayer::cPortaudioWavplayer(const char *_name) :
  cSmileComponent(_name),
  audioBuffersize(-1),
  audioBuffersize_sec(-1.0),
  eof(0),  abort(0),
  monoMixdown(0),
  deviceId(0),
  listDevices(0),
  sampleRate(0),
  channels(1),
  inputChannels(0),
  playback(0),
  playPtr(0),
  playIndex(0),
  dataFlag(1),
  wavedata(NULL),
  wavelength(NULL),
  numKw(0),
  keywords(NULL),
  nWavs(0),
  wavsLoaded(0),
  numInd(0),
  indices(NULL),
  nBits(16),
  isFirst(1),
  isPaInit(0),
  stream(NULL),
  streamStatus(PA_STREAM_STOPPED)
{
  //smileMutexCreate(dataFlagMtx);
  smileMutexCreate(callbackMtx);
  smileCondCreate(callbackCond);
}

void cPortaudioWavplayer::myFetchConfig()
{
  //cDataSink::myFetchConfig();
  
  monoMixdown = getInt("monoMixdown");
  if (monoMixdown) { 
    SMILE_IDBG(2,"monoMixdown enabled!"); 
    channels = 1;
  } else {
    channels = 2;
  }

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

  numKw = getArraySize("keyword");
  keywords = (const char **)calloc(1,sizeof(const char*)*numKw);
  int i;
  for (i=0; i<numKw; i++) {
    keywords[i] = getStr_f(myvprint("keyword[%i]",i));
  }

  numInd = getArraySize("indices");
  indices = (int *)calloc(1,sizeof(int*)*numInd);
  for (i=0; i<numInd; i++) {
    indices[i] = getInt_f(myvprint("indices[%i]",i));
  }

  semaineCallbackRcpt = getStr("semaineCallbackRcpt");
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


int cPortaudioWavplayer::myConfigureInstance()
{
  // pre-load wave files snippets..
  if (!wavsLoaded) {
    nWavs = getArraySize("wavefile");
    wavedata = (float **)calloc(1,sizeof(float *)*nWavs);
    wavelength = (long *)calloc(1,sizeof(long)*nWavs);
    int i;
    for (i=0; i<nWavs; i++) {
      // open wave file
      const char * filename = getStr_f(myvprint("wavefile[%i]",i));
      FILE * filehandle;
      filehandle = fopen(filename, "rb");
      if (filehandle == NULL) COMP_ERR("failed to open wave file '%s'",filename);
  
      // read size from header
      sWaveParameters pcmParam;
      smilePcm_readWaveHeader(filehandle, &pcmParam, filename);

      // check whether files have same sampling rate and channels
      if (sampleRate == 0) sampleRate = pcmParam.sampleRate;
      else if (sampleRate != pcmParam.sampleRate) {
        SMILE_IERR(1,"Error loading wave files. The file '%s' has a sample rate (%i) which differs from the other (previously loaded) files (%i).",filename,pcmParam.sampleRate,sampleRate);
        COMP_ERR("aborting");
      }

      if (inputChannels==0) inputChannels = pcmParam.nChan;
      else if (inputChannels != pcmParam.nChan) {
        SMILE_IERR(1,"mismatch in number of channels while loading wave files!");
        COMP_ERR("aborting");
      }

      // allocate memory
      wavelength[i] = pcmParam.blockSize*pcmParam.nBlocks;
      wavedata[i] = (float*)calloc(1,sizeof(float)*wavelength[i]);

      // load data
      smilePcm_readSamples(&filehandle, &pcmParam, wavedata[i], pcmParam.nChan, wavelength[i], monoMixdown);

      if (filehandle != NULL) fclose(filehandle);
    }
    wavsLoaded = 1;
  }


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
      SMILE_IMSG(1,"Device list was printed, cPortaudioWavplayer::myConfigureInstance will now return 0 in order to stop openSMILE initialisation!");
    }
    return 0;
  } else {
    if (numDevices <= 0) {
      SMILE_IERR(1,"NO PortAudio devices found! Cannot play live audio!");
    }
  }
  
  return cSmileComponent::myConfigureInstance();
}

int cPortaudioWavplayer::myFinaliseInstance()
{
  int ret;

  ret = cSmileComponent::myFinaliseInstance();

  if (ret) {
    SMILE_IDBG(2,"setting playback sampleRate: %i Hz",sampleRate);
    //    printf("XXXXXXXXXXXXXXx computed sampleRate as: %i Hz (per = %f)\n",sampleRate,reader->getLevelT()*1000);
  }

  return ret;
}

#ifdef HAVE_JULIUSLIB
// return value = -1 , if no keyword was found in current result string
// TODO: set a flag if multiple keywords are found in string, currently we return after finding the first keyword
int cPortaudioWavplayer::getPlayIdxFromKeyword(juliusResult *k) 
{
  int i,j;

  // for each word
  for (i=0; i<k->numW; i++) {
    int found = 0;
    // find in keyword list, and get the playback index
    for (j=0; j<numKw; j++) {

      // NOTE: keyword comparison is case INsensitive ! is that ok?
      if (!strcasecmp( k->word[i], keywords[j] )) {
        return j;
      }

    }
  }

  return -1;
}
#endif

int cPortaudioWavplayer::getPlayIdxFromClassname(const char *cls) 
{
  int j;

  // find in keyword list, and get the playback index
  for (j=0; j<numKw; j++) {

    // NOTE: keyword comparison is case INsensitive ! is that ok?
    if (!strcasecmp( cls, keywords[j] )) {
      return j;
    }

  }

  return -1;
}

int cPortaudioWavplayer::getPlayIdxFromClassidx(float idx) 
{
  int j;
  if (idx < 0) return -1;

  // find in keyword list, and get the playback index
  for (j=0; j<numInd; j++) {

    // NOTE: keyword comparison is case INsensitive ! is that ok?
    if ((int)idx == indices[j] ) {
      return j;
    }

  }

  return -1;
}
void cPortaudioWavplayer::sendEndMessage()
{
  cComponentMessage msg("semaineCallback");
  msg.msgname[0] = 'e';
  msg.msgname[1] = 'n';
  msg.msgname[2] = 'd';
  sendComponentMessage( semaineCallbackRcpt, &msg );
}

int cPortaudioWavplayer::triggerPlayback(int idx) 
{
  if (isFirst) {
    isFirst = 0;
    return 0;
  }

  cComponentMessage msg("semaineCallback");
  msg.msgname[0] = 's';
  msg.msgname[1] = 't';
  msg.msgname[2] = 'a';
  msg.msgname[3] = 'r';
  msg.msgname[4] = 't';
  sendComponentMessage( semaineCallbackRcpt, &msg );
  
  // mutex lock
  smileMutexLock(callbackMtx);

  if (idx < 0) idx = 0;
  if (idx >= nWavs) idx = nWavs-1;

  if (playback) {
    smileMutexUnlock(callbackMtx);
    return 0;
  }

  playback = 1;
  playIndex = idx;
  playPtr = 0;

  // mutex unlock
  smileMutexUnlock(callbackMtx);

  return 1;
}

int cPortaudioWavplayer::processComponentMessage( cComponentMessage *_msg )
{
  int playIdx = -1;
  // get a keyword message, trigger playback
  if (isMessageType(_msg,"asrKeywordOutput")) {  
#ifdef HAVE_JULIUSLIB
    if (_msg->intData[0] == 1) { // pass 1
      SMILE_IDBG(3,"received 'asrKeywordOutput' message");
      juliusResult *k = (juliusResult *)(_msg->custData);
      playIdx = getPlayIdxFromKeyword(k);
      if (playIdx >= 0) triggerPlayback(playIdx);
    }
    return 1;  // message was processed
#else
    SMILE_IWRN(2, "asrKeywordOutput message received, but portaudio components were compiled without julius support (HAVE_JULIUSLIB not defined).");
    return 0;
#endif
  } else if (isMessageType(_msg,"classificationResult")) {
    SMILE_IDBG(3,"received 'classificationResult' message");
    // get play index from class string....
    playIdx = getPlayIdxFromClassname(_msg->msgtext);
    if (playIdx == -1) {
      playIdx = getPlayIdxFromClassidx((float)(_msg->floatData[0]));
    }
    if (playIdx >= 0) triggerPlayback(playIdx);
  }
  return 0;
}

eTickResult cPortaudioWavplayer::myTick(long long t)
{
  if (streamStatus == PA_STREAM_STOPPED) {
    if (!startPlayback()) return TICK_INACTIVE;
  }

  return playback ? TICK_SUCCESS : TICK_INACTIVE;
}


cPortaudioWavplayer::~cPortaudioWavplayer()
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

  if (wavedata != NULL) {
    int i;
    for (i=0; i<nWavs; i++) {
      free(wavedata[i]);
    }
    free(wavedata);
  }
  if (wavelength != NULL) { free(wavelength); }

  if (keywords != NULL) free(keywords);
  if (indices != NULL) free(indices);
}

//--------------------------------------------------  portaudio specific


// channels = number of output channels (can be 1 or 2)
int pcmFloatChannelConvert(void *outputBuffer, long __N, float *in, int iChan, int oChan, int mixdown=0) 
{
  int i,c;

  float *out = (float*)outputBuffer;
  
  for (i=0; i<__N; i++) {
    if (mixdown) {
      out[i] = 0.0;
      for (c=0; c<iChan; c++) {
        out[i] += in[i*iChan+c];
      }
    } else {
      int minc = oChan;
      if (iChan < minc) minc = iChan;
      for (c=0; c<oChan; c++) {
        out[i*oChan+c] = in[i*iChan+c];
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
  cPortaudioWavplayer * obj = (cPortaudioWavplayer *)userData;

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

  if (obj->playback) {

    if (obj->playPtr+(long)framesPerBuffer > obj->wavelength[obj->playIndex]) {
      // EOF
      // copy last bits of buffer
      pcmFloatChannelConvert(outputBuffer, obj->wavelength[obj->playIndex] - obj->playPtr, obj->wavedata[obj->playIndex]+obj->playPtr, obj->getInputChannels(), obj->getChannels(), obj->isMonoMixdown());
      // fill remaining buffer with zeros
      int i;
      for (i=obj->wavelength[obj->playIndex] - obj->playPtr; i<(long)framesPerBuffer; i++) { ((float*)outputBuffer)[i] = 0.0; }

      obj->playback=0;
      obj->playIndex=0;
      
      obj->sendEndMessage();

      /* continuous loop playback for debugging this component
      
      if (obj->playIndex < obj->nWavs-1) {
      obj->playIndex++;
      obj->playback=1;
      }*/

      obj->playPtr=0;
    
    } else {

      // playback of current wave file snippet
      pcmFloatChannelConvert(outputBuffer, framesPerBuffer, obj->wavedata[obj->playIndex]+obj->playPtr, obj->getInputChannels(), obj->getChannels(), obj->isMonoMixdown());
      obj->playPtr += framesPerBuffer;

    }
    

  } else {
    // output silence

    memset(outputBuffer, 0, sizeof(float)*obj->getChannels()*framesPerBuffer);
  }

  smileMutexUnlock(obj->callbackMtx);
  return paContinue;
}


int cPortaudioWavplayer::startPlayback()
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

int cPortaudioWavplayer::stopPlayback()
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

int cPortaudioWavplayer::stopPlaybackWait()
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

void cPortaudioWavplayer::printDeviceList(void)
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
