/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

portAudio dataSource for live recording from sound device
known to work on windows, linux, and mac

*/


#ifndef __CPORTAUDIOSOURCE_HPP
#define __CPORTAUDIOSOURCE_HPP

#include <core/smileCommon.hpp>
#include <core/smileThread.hpp>
#include <core/dataSource.hpp>

#ifdef HAVE_PORTAUDIO

#define COMPONENT_DESCRIPTION_CPORTAUDIOSOURCE "This component handles live audio recording from the soundcard via the PortAudio library"
#define COMPONENT_NAME_CPORTAUDIOSOURCE "cPortaudioSource"


#define PA_STREAM_STOPPED 0
#define PA_STREAM_STARTED 1

/*
typedef struct {
  long sampleRate;
  int sampleType;
  int nChan;
  int blockSize;
  int nBPS;       // actual bytes per sample
  int nBits;       // bits per sample (precision)
  int byteOrder;  // BYTEORDER_LE or BYTEORDER_BE
  int memOrga;    // MEMORGA_INTERLV  or MEMORGA_SEPCH
  long nBlocks;  // nBlocks in buffer
} sWaveParameters;
*/

typedef void PaStream;

class cPortaudioSource : public cDataSource {
  private:
    PaStream *stream;
    int byteSwap;
    long paFrames;
    int deviceId;
    int streamStatus;
    int listDevices;
    int numDevices;
    int lastDataCount;
    int isPaInit;
    int warned;

    long audioBuffersize;
    double audioBuffersize_sec;
    //long mBuffersize;
    
    //sWaveParameters pcmParam;

    int monoMixdown;    // if set to 1, multi-channel files will be mixed down to 1 channel

    long curReadPos;   // in samples
    int eof, abort; // NOTE : when setting abort, first lock the callbackMtx!!!
    int channels, sampleRate, nBits, nBPS, selectChannel;
    
    int setupDevice();

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int pauseEvent() override;
    virtual void resumeEvent() override; 

    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNewNames(long nEl) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    // variables for the callback method:
    smileMutex dataFlagMtx;
    smileMutex callbackMtx;
    smileCond  callbackCond;
    int dataFlag;
    cMatrix *callbackMatrix;
    
    cPortaudioSource(const char *_name);

    void printDeviceList();
    int startRecording();
    int stopRecording();
    int stopRecordingWait();
    
    virtual bool notifyEmptyTickloop() override;

    cDataWriter * getWriter() { return writer_; }

    void setNewDataFlag() {
      smileMutexLock(dataFlagMtx);
      dataFlag = 1;
      lastDataCount=0;
      smileMutexUnlock(dataFlagMtx);
    }
    int getNBPS() { return nBPS; }
    int getNBits() { return nBits; }
    int getChannels() { return channels; }
    int getSelectedChannel() { return selectChannel; }
    int getSampleRate() { return sampleRate; }
    int isAbort() { return abort; }
    int isMonoMixdown() { return monoMixdown; }
    
    virtual ~cPortaudioSource();
};


#endif // HAVE_PORTAUDIO


#endif // __CPORTAUDIOSOURCE_HPP
