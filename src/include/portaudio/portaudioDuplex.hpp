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


#ifndef __CPORTAUDIODUPLEX_HPP
#define __CPORTAUDIODUPLEX_HPP

#include <core/smileCommon.hpp>
#include <core/smileThread.hpp>
#include <core/dataProcessor.hpp>

#ifdef HAVE_PORTAUDIO

#include <portaudio.h>

#define COMPONENT_DESCRIPTION_CPORTAUDIODUPLEX "dataProcessor for full-duplex playback and recording of live audio using PortAudio library"
#define COMPONENT_NAME_CPORTAUDIODUPLEX "cPortaudioDuplex"


#define PA_STREAM_STOPPED 0
#define PA_STREAM_STARTED 1

class cPortaudioDuplex : public cDataProcessor {
  private:
    PaStream *stream;
    long paFrames;
    int deviceId;
    int streamStatus;
    int listDevices;
    int numDevices;
    int lastDataCount;
    int isPaInit;
    
    long audioBuffersize;
    double audioBuffersize_sec;
    //long mBuffersize;    

    int monoMixdownPB, monoMixdownREC;    // if set to 1, multi-channel files will be mixed down to 1 channel

    int eof, abort; // NOTE : when setting abort, first lock the callbackMtx!!!
    int channels, sampleRate, nBits, nBPS;
    
    //int setupDevice();

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;


    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNewNames(long nEl) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    // variables for the callback method:
    //smileMutex dataFlagMtx;
    smileMutex callbackMtx;
    smileCond  callbackCond;
    int dataFlagR, dataFlagW;
    cMatrix *callbackMatrix;
    
    cPortaudioDuplex(const char *_name);

    void printDeviceList();

    int startDuplex();
    int stopDuplex();
    int stopDuplexWait();
    
    cDataReader * getReader() { return reader_; }
    cDataWriter * getWriter() { return writer_; }

    /*void setReadDataFlag() {
      smileMutexLock(dataFlagMtx);
      dataFlag = 1;
      //lastDataCount=0;
      smileMutexUnlock(dataFlagMtx);
    }*/

    int getNBPS() { return nBPS; }
    int getNBits() { return nBits; }
    int getChannels() { return channels; }
    int getSampleRate() { return sampleRate; }
    
    int isAbort() { return abort; }
    int isMonoMixdownPB() { return monoMixdownPB; }
    int isMonoMixdownREC() { return monoMixdownREC; }
    
    virtual ~cPortaudioDuplex();
};


#endif // HAVE_PORTAUDIO


#endif // __CPORTAUDIODUPLEX_HPP
