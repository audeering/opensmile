/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

portAudio dataSink for live audio playback
(?? known to work on windows, linux, and mac)

*/


#ifndef __CPORTAUDIOWAVPLAYER_HPP
#define __CPORTAUDIOWAVPLAYER_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <core/smileThread.hpp>

#ifdef HAVE_PORTAUDIO

#include <portaudio.h>

#define COMPONENT_DESCRIPTION_CPORTAUDIOWAVPLAYER "This component can play wavefiles directly to a portaudio output device. Multiple wave files can be specified as inputs and they can be played via smile messages containing the index of the sample to play. Concurrent play requests will be discarded (TODO: mix the output). All wave files must have the same sampling rate."
#define COMPONENT_NAME_CPORTAUDIOWAVPLAYER "cPortaudioWavplayer"


#define PA_STREAM_STOPPED 0
#define PA_STREAM_STARTED 1

class cPortaudioWavplayer : public cSmileComponent {
  private:
    PaStream *stream;
    long paFrames;
    int deviceId;
    int streamStatus;
    int listDevices;
    int numDevices;
    int isPaInit;
    
    long audioBuffersize;
    double audioBuffersize_sec;
    
    int monoMixdown;    // if set to 1, multi-channel files will be mixed down to 1 channel

    int eof, abort; // NOTE : when setting abort, first lock the callbackMtx!!!
    int inputChannels, channels, sampleRate, nBits, nBPS;
    
    const char *semaineCallbackRcpt;
    int isFirst;

    int numKw;
    const char ** keywords;
    int numInd;
    int * indices;


    int setupDevice();

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    //virtual int configureReader() override;

    virtual int processComponentMessage(cComponentMessage *_msg) override;

#ifdef HAVE_JULIUSLIB
    int getPlayIdxFromKeyword(juliusResult *k);
#endif
    int getPlayIdxFromClassname(const char *cls);
    int getPlayIdxFromClassidx(float idx);
    int triggerPlayback(int idx);

  public:
    SMILECOMPONENT_STATIC_DECL
    
    // variables for the callback method:
    int wavsLoaded;
    int nWavs;
    float **wavedata;
    long *wavelength;

    int playback, playIndex;
    long playPtr;

    smileMutex dataFlagMtx;
    smileMutex callbackMtx;
    smileCond  callbackCond;
    int dataFlag;
        
    cPortaudioWavplayer(const char *_name);

    void printDeviceList();
    
    void sendEndMessage();

	  int startPlayback();
    int stopPlayback();
    int stopPlaybackWait();

    int getNBPS() { return nBPS; }
    int getNBits() { return nBits; }
    int getChannels() { return channels; }
    int getInputChannels() { return inputChannels; }
    int getSampleRate() { return sampleRate; }

    int isAbort() { return abort; }
    int isMonoMixdown() { return monoMixdown; }

    virtual ~cPortaudioWavplayer();
};


#endif // HAVE_PORTAUDIO


#endif // __CPORTAUDIOWAVPLAYER_HPP
