/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

waveSink:
writes data to an uncompressed PCM WAVE file

*/


#ifndef __CWAVESINKCUT_HPP
#define __CWAVESINKCUT_HPP

#include <core/smileCommon.hpp>
#include <core/dataSink.hpp>
#include <iocore/waveSource.hpp>

#define COMPONENT_DESCRIPTION_CWAVESINKCUT "This component writes data to uncompressed PCM WAVE files. Only chunks, based on timings received via smile messages are written to files. The files may have consecutive numbers appended to the file name."
#define COMPONENT_NAME_CWAVESINKCUT "cWaveSinkCut"

class cWaveSinkCut : public cDataSink {
  private:
    const char *fileExtension;
    const char *filebase;
    const char *fileNameFormatString;
    int multiOut;
    long curFileNr;
    
    long forceSampleRate;
    long preSil, postSil;
    int turnStart, turnEnd, nPre, nPost, vIdxStart, vIdxEnd, curVidx;
    float startSec0_, startSec_, endSec_;
    long curStart, curEnd;
    int isTurn, endWait;

    FILE * fHandle;
    void *sampleBuffer; long sampleBufferLen;

  	int nBitsPerSample;
	  int nBytesPerSample;
	  int sampleFormat;
    int nChannels;
    long fieldSize;
	
    long nOvl;

  	long curWritePos;   // in samples??
	  long nBlocks;
	  int showSegmentTimes_;
	  const char * saveSegmentTimes_;

    int writeWaveHeader();
    int writeDataFrame(cVector *m=NULL);
    char * getCurFileName();
    void saveAndPrintSegmentData(long n);

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;
    virtual int processComponentMessage(cComponentMessage *_msg) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cWaveSinkCut(const char *_name);

    virtual ~cWaveSinkCut();
};




#endif // __CWAVESINKCUT_HPP
