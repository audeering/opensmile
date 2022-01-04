/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

waveSource : reads PCM WAVE files (RIFF format)

Support for a negative start index was added by Benedikt Gollan (TUM).

*/


#ifndef __WAVE_SOURCE_HPP
#define __WAVE_SOURCE_HPP

#include <core/smileCommon.hpp>
#include <core/dataSource.hpp>

#define COMPONENT_DESCRIPTION_CWAVESOURCE "This component reads an uncompressed RIFF (PCM-WAVE) file and saves it as a stream to the data memory."
#define COMPONENT_NAME_CWAVESOURCE "cWaveSource"

/*
#define BYTEORDER_LE    0
#define BYTEORDER_BE    1
#define MEMORGA_INTERLV 0
#define MEMORGA_SEPCH   1

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

class cWaveSource : public cDataSource {
  private:
    const char *filename;
    //long buffersize;
    FILE *filehandle;
    sWaveParameters pcmParam;

    int properTimestamps_;
    int negativestart;
    long negstartoffset;
    double start, end, endrel;
    long startSamples, endSamples, endrelSamples;
    
    int monoMixdown;    // if set to 1, multi-channel files will be mixed down to 1 channel
    long pcmDataBegin;  // in bytes
    long curReadPos;   // in samples
    int eof;
    const char *outFieldName;

    int readWaveHeader();
    int readData(cMatrix *m=NULL);
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNewNames(long nEl) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cWaveSource(const char *_name);

    virtual ~cWaveSource();
};




#endif // __EXAMPLE_SOURCE_HPP
