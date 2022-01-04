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


#ifndef __CWAVESINK_HPP
#define __CWAVESINK_HPP

#include <core/smileCommon.hpp>
#include <core/dataSink.hpp>
#include <iocore/waveSource.hpp>

#define COMPONENT_DESCRIPTION_CWAVESINK "This component saves data to an uncompressed PCM WAVE file"
#define COMPONENT_NAME_CWAVESINK "cWaveSink"

class cWaveSink : public cDataSink {
private:
  const char *filename;
  FILE * fHandle;
  //int lag;
  int frameRead;
  int buffersize;
  int flushData;
  void *sampleBuffer; long sampleBufferLen;

  int nBitsPerSample;
  int nBytesPerSample;
  int sampleFormat;
  int nChannels;

  //double start, end, endrel;
  //long startSamples, endSamples, endrelSamples;

  long curWritePos;   // in samples??
  long nBlocks;
  //int eof;

  int writeWaveHeader();
  int writeData(cMatrix *m=NULL);

protected:
  SMILECOMPONENT_STATIC_DECL_PR

  virtual void myFetchConfig() override;
  //virtual int myConfigureInstance() override;
  virtual int myFinaliseInstance() override;
  virtual eTickResult myTick(long long t) override;

  virtual int configureReader() override;

public:
  SMILECOMPONENT_STATIC_DECL

    cWaveSink(const char *_name);

  virtual ~cWaveSink();
};




#endif // __CWAVESINK_HPP
