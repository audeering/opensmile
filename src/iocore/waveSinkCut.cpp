/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

waveSink: writes data to an uncompressed PCM WAVE file, thereby being able to split the output to multiple files, based on messages

*/

/*
 read wave frame by frame
 get turnStart / turnEnd messages (pre/post normalised)
 read from initial pcm frames level
 if isTurn: write data to current file frame by frame
 @turnEnd: close file
 @turnStart: open new file, increase file number counter
*/




#include <iocore/waveSinkCut.hpp>

#define MODULE "cWaveSinkCut"


#define SMILE_SFSTR_8BIT        "8bit"     // 8-bit signed
#define SMILE_SFSTR_16BIT       "16bit"    // 16-bit signed
#define SMILE_SFSTR_24BIT       "24bit"    // 24-bit signed sample in 4byte dword
#define SMILE_SFSTR_24BITp      "24bitp"   // 3-byte packed 24-bit signed value
#define SMILE_SFSTR_32BIT       "32bit"    // 32-bit signed integer
#define SMILE_SFSTR_32BIT_FLOAT "float"    // 32-bit float

#define SMILE_SF_8BIT        0    // 8-bit signed
#define SMILE_SF_16BIT       1    // 16-bit signed
#define SMILE_SF_24BIT       2    // 24-bit signed sample in 4byte dword
#define SMILE_SF_24BITp      3    // 3-byte packed 24-bit signed value
#define SMILE_SF_32BIT       4    // 32-bit signed integer
#define SMILE_SF_32BIT_FLOAT 5    // 32-bit float

SMILECOMPONENT_STATICS(cWaveSinkCut)

SMILECOMPONENT_REGCOMP(cWaveSinkCut)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CWAVESINKCUT;
  sdescription = COMPONENT_DESCRIPTION_CWAVESINKCUT;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("fileBase","The base of the wave file name, if writing multiple output files (multiOut=1), or else the filename of the wave file to write to","output_segment_");
    ct->setField("fileExtension","The file extension to use when writing multiple output files (multiOut=1), else this option is ignored (the extension is set via 'fileBase' then which specifies the full file name)",".wav");
    ct->setField("fileNameFormatString","Specifies how the filename will be formatted (printf compatible syntax, three parameters are available in the given order: fileBase (string), current index (integer), fileExtension (string)), the default should be reasonable, it generates filenames such as 'output_segment_XXXX.wav'.","%s%04d%s");
    ct->setField("startIndex","The index of the first file for consecutive numbering of output files (if multiOut=1)",1);
    ct->setField("preSil","Specifies the amount of silence at the turn beginning in seconds, i.e. the lag of the turn detector. This is the length of the data that will be added to the current segment prior to the turn start time received in the message from the turn detector component.",0.2);
    ct->setField("postSil","Specifies the amount of silence at the turn end in seconds. This is the length of the data that will be added to the current segment after to the turn end time received in the message from the turn detector component.",0.3);
    ct->setField("multiOut","1 = enable multiple file mode, i.e. multiple files segmented by turnStart/turnEnd messages ; 0 = write all frames (only between turnStart/turnEnd messages) concatenated to one file, i.e. effectively filtering out non-turn audio.",1);
    ct->setField("forceSampleRate","force sample rate to given value (if not set, it is determined from the reader's frame size in bytes, which may be inaccurate)",16000);
    char * sfdesc = myvprint("openSMILE uses float for all data internally. Thus you must specify your desired sample format for the wave files here. Available formats:\n   '%s' : 8-bit signed \n   '%s' : 16-bit signed\n   '%s' : 24-bit signed\n   '%s' : 24-bit signed packed in 3 bytes\n   '%s' : 32-bit signed integer\n   '%s' : 32-bit float",SMILE_SFSTR_8BIT,SMILE_SFSTR_16BIT,SMILE_SFSTR_24BIT,SMILE_SFSTR_24BITp,SMILE_SFSTR_32BIT,SMILE_SFSTR_32BIT_FLOAT);
    ct->setField("sampleFormat",sfdesc,SMILE_SFSTR_16BIT);
    ct->setField("showSegmentTimes", "1 = show start and end times relative to input of segments that are saved by this sink.", 0);
    ct->setField("saveSegmentTimes", "1 = save turn times (start, end, and filename) to text file.", (const char*)NULL);
    //ct->setField("sampleFormat","openSMILE uses float for all data internally. Thus you must specify your desired sample format for the wave files here. Available formats:\n   '8bit' : 8-bit signed \n   '16bit' : 16-bit signed\n   '24bit' : 24-bit signed\n   '24bitp' : 24-bit signed packed in 3 bytes\n   '32bit' : 32-bit signed integer\n   'float' : 32-bit float",SMILE_SFSTR_16BIT);
	  free(sfdesc);
  )

  SMILECOMPONENT_MAKEINFO(cWaveSinkCut);
}

SMILECOMPONENT_CREATE(cWaveSinkCut)

//-----

cWaveSinkCut::cWaveSinkCut(const char *_name) :
  cDataSink(_name),
  fHandle(NULL),
  filebase(NULL),
  fileExtension(NULL),
  multiOut(0),
  isTurn(0), curStart(0), curEnd(0),
  turnEnd(0), turnStart(0),
  curFileNr(0), fieldSize(0),
  curVidx(0), vIdxStart(0), vIdxEnd(0), endWait(-1),
  sampleBuffer(NULL), sampleBufferLen(0),
  nOvl(0), preSil(0), postSil(0),
  forceSampleRate(0),
  showSegmentTimes_(0)
{
  // ...
}

void cWaveSinkCut::myFetchConfig()
{
  cDataSink::myFetchConfig();

  filebase = getStr("fileBase");
  SMILE_IDBG(2,"fileBase = '%s'",filebase);
  if (filebase == NULL) COMP_ERR("myFetchConfig: getStr(filebase) returned NULL! missing option in config file?");

  fileExtension = getStr("fileExtension");
  SMILE_IDBG(2,"fileExtension = '%s'",fileExtension);

  fileNameFormatString = getStr("fileNameFormatString");
  SMILE_IDBG(2,"fileNameFormatString = '%s'",fileNameFormatString);

  multiOut = getInt("multiOut");
  if (multiOut) { SMILE_IDBG(2,"outputting multiple files"); }

  curFileNr = getInt("startIndex");
  SMILE_IDBG(2,"startIndex = %i",curFileNr);

  const char * sampleFormatStr = getStr("sampleFormat");
  if (sampleFormatStr != NULL) {
    SMILE_IDBG(2,"sampleFormat = '%s'",sampleFormatStr);
    if (!strcasecmp(sampleFormatStr,SMILE_SFSTR_8BIT)) {
      nBitsPerSample = 8;
      nBytesPerSample = 1;
      sampleFormat = SMILE_SF_8BIT;
    }
    else if (!strcasecmp(sampleFormatStr,SMILE_SFSTR_16BIT)) {
      nBitsPerSample = 16;
      nBytesPerSample = 2;
      sampleFormat = SMILE_SF_16BIT;
    }
    else if (!strcasecmp(sampleFormatStr,SMILE_SFSTR_24BIT)) {
      nBitsPerSample = 24;
      nBytesPerSample = 4;
      sampleFormat = SMILE_SF_24BIT;
    }
    else if (!strcasecmp(sampleFormatStr,SMILE_SFSTR_24BITp)) {
      nBitsPerSample = 24;
      nBytesPerSample = 3;
      sampleFormat = SMILE_SF_24BITp;
    }
    else if (!strcasecmp(sampleFormatStr,SMILE_SFSTR_32BIT)) {
      nBitsPerSample = 32;
      nBytesPerSample = 4;
      sampleFormat = SMILE_SF_32BIT;
    }
    else if (!strcasecmp(sampleFormatStr,SMILE_SFSTR_32BIT_FLOAT)) {
      nBitsPerSample = 32;
      nBytesPerSample = 4;
      sampleFormat = SMILE_SF_32BIT_FLOAT;
    }
    else
    {
      SMILE_IERR(1,"unknown sampleFormat '%s'!",sampleFormatStr);
      COMP_ERR("aborting");
    }
  }

  if (isSet("forceSampleRate")) {
    forceSampleRate = getInt("forceSampleRate");
  } else {
    forceSampleRate = 0;
  }

  showSegmentTimes_ = getInt("showSegmentTimes");
  saveSegmentTimes_ = getStr("saveSegmentTimes");
}

// get the current filename , caller must free the string!
char * cWaveSinkCut::getCurFileName() 
{
  if (multiOut) {
    return myvprint(fileNameFormatString, filebase, curFileNr, fileExtension);
  } else {
    return strdup(filebase);
  }
}

int cWaveSinkCut::myConfigureInstance()
{
  int ret = cDataSink::myConfigureInstance();
  if (!ret) return 0;
  return ret;
}


int cWaveSinkCut::myFinaliseInstance()
{
  int ret = cDataSink::myFinaliseInstance();
  if (!ret) return 0;

 // setup pre/post silence config:
  preSil = (float)getDouble("preSil");
  postSil = (float)getDouble("postSil");

  nChannels = reader_->getLevelNf();
  fieldSize = reader_->getLevelN() / nChannels;

  // open wave file for writing
  if (multiOut == 0) {
    if (fHandle == NULL) {
      fHandle = fopen(getCurFileName(), "wb");  // TODO: support append mode
      if (fHandle == NULL) COMP_ERR("failed to open output file '%s'",getCurFileName());
    }
  }

  nBlocks = 0;
  if (fHandle != NULL) {
    // write dummy header...
    curWritePos = writeWaveHeader();
    if (curWritePos == 0) COMP_ERR("failed writing initial wave header to file '%s'! Disk full or read-only filesystem?",getCurFileName());
  }

  return ret;
}

void cWaveSinkCut::saveAndPrintSegmentData(long n)
{
  if (saveSegmentTimes_ != NULL && saveSegmentTimes_[0] != '?') {
    FILE *f = fopen(saveSegmentTimes_, "a");
    // each line: filename;startsec;endsec;numberOfFrames(Vidxes)
    fprintf(f, "%s;%f,%f;%ld\n", getCurFileName(), startSec_, endSec_, n);
    fclose(f);
  }
  if (showSegmentTimes_) {
    SMILE_IMSG(4, "Segment '%s' : %f sec. - %f sec. (%ld frames)", getCurFileName(), startSec_, endSec_, n);
  }
}

int cWaveSinkCut::processComponentMessage( cComponentMessage *msg )
{
  if (multiOut==1) {
	double _T = reader_->getLevelT();
    if (isMessageType(msg,"turnStart")) {
      // if previous message was not yet processed we update the data
      nPre = (long)(msg->floatData[0]);
      startSec0_ = (float)(msg->floatData[1] * msg->floatData[2]) - preSil;
      vIdxStart = startSec0_ / _T;
      turnStart=1;
      if (showSegmentTimes_) {
        SMILE_IDBG(2, "Start of segment received: %.2f seconds, vIdx: %d)", startSec0_, vIdxStart);
      }
      return 1;
    }
    if (isMessageType(msg,"turnEnd")) {
      if (!turnEnd) { // if previous message was not yet processed we discard this message
        nPost = (long)(msg->floatData[0]);
        endSec_ = (float)(msg->floatData[1] * msg->floatData[2]) + postSil;
        vIdxEnd = endSec_ / _T;
        turnEnd = 1;
        startSec_ = startSec0_;
        if (showSegmentTimes_) {
          SMILE_IDBG(2, "End of segment received: %.2f seconds, vIdx: %d", endSec_, vIdxEnd);
        }
        //SMILE_IDBG(2,"received turn end at vIdx %i!",vIdxEnd); 
        return 1;
      }
    }
    // TODO: message queque??
  }
  return 0;
}

eTickResult cWaveSinkCut::myTick(long long t)
{
  //if (fieldSize == 0) fieldSize = reader->getLevelN() / reader->getLevelNf(); //vec->fmeta->field[0].N;
  if (multiOut == 1) {
    lockMessageMemory();
    // handle pre/post silence and turn detector interface
    if ((turnStart)&&(!turnEnd)) { 
      turnStart = 0; 
      curVidx = vIdxStart;
      if (curVidx < 0) {
        curVidx = 0;
      }
      isTurn = 1;
      SMILE_IDBG(2,"received turn start at vIdx %i!",vIdxStart);

      // just to be sure...
      if (fHandle != NULL) fclose(fHandle);
      fHandle = fopen(getCurFileName(), "wb");  // TODO: support append mode
      if (fHandle == NULL) SMILE_IERR(1,"failed to open output file '%s', no wave output will be written",getCurFileName());

      nBlocks=0;
      curWritePos = writeWaveHeader();
      if (curWritePos == 0) {
        SMILE_IERR(1,"failed writing initial wave header to file '%s'! Disk full or read-only filesystem?",getCurFileName());
        fclose(fHandle); fHandle=NULL;
      }
    }
    if (turnEnd) { 
      if (curVidx >= vIdxEnd) { turnEnd = 0; isTurn = 0; }
      // if no frames have been written...
      if (curVidx == vIdxStart && isTurn) { 
        SMILE_IERR(1,"no frames were written for turn #%i",curFileNr);
        turnEnd=0; isTurn=0;
      }
      if (!turnEnd) {
        if (fHandle != NULL) {
          saveAndPrintSegmentData(curVidx - vIdxStart);
          SMILE_IDBG(2,"processed turn end, file '%s' was closed!", getCurFileName());
          writeWaveHeader();
          fclose(fHandle); 
          nBlocks=0;
          fHandle=NULL;
          curFileNr++;
        }
      }
    }
    //if (!isTurn) { ret=-3; isTurn = 1; }
    unlockMessageMemory();
  }

  // read next buffer from memory:
  if ((fHandle != NULL)&&(isTurn)) {
    // TODO: if vIdxstart == -1 , then call reader_->getNextFrame to get latest frame...
    cVector *vec = reader_->getFrame(curVidx);
    if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;
    curVidx++;
    // write data buffer to file:
    long written = writeDataFrame(vec);
    if (written > 0) {
      nWritten_++;
    }
    return written > 0 ? TICK_SUCCESS : TICK_INACTIVE;
  } else {
    reader_->catchupCurR();
  }
  return isTurn ? TICK_SUCCESS : TICK_INACTIVE;
}


cWaveSinkCut::~cWaveSinkCut()
{
  if (sampleBuffer!=NULL) free(sampleBuffer);
  if (fHandle != NULL) {
    // write final wave header
    writeWaveHeader();
    fclose(fHandle);

// FIXME : write last segment!

  }
}

//----------------------------------------------------------------------------------

/* WAVE Header struct, valid only for PCM Files */
typedef struct {
  uint32_t	Riff;    /* Must be little endian 0x46464952 (RIFF) */
  uint32_t	FileSize;
  uint32_t	Format;  /* Must be little endian 0x45564157 (WAVE) */

  uint32_t	Subchunk1ID;  /* Must be little endian 0x20746D66 (fmt ) */
  uint32_t	Subchunk1Size;
  uint16_t	AudioFormat;
  uint16_t	NumChannels;
  uint32_t	SampleRate;
  uint32_t	ByteRate;
  uint16_t	BlockAlign;
  uint16_t	BitsPerSample;

  uint32_t	Subchunk2ID;  /* Must be little endian 0x61746164 (data) */
  uint32_t  Subchunk2Size;
} sRiffPcmWaveHeader;

typedef struct {
  uint32_t SubchunkID;
  uint32_t SubchunkSize;
} sRiffChunkHeader;

int cWaveSinkCut::writeWaveHeader()
{
  if (fHandle == NULL) return 0;

  // use reader parameters to determine
  //   nChannels
  //   sampleRate
  long sampleRate;
  // TODO: detect overlap!!
  double fss = reader_->getFrameSizeSec();
  double ft  = reader_->getLevelT();

  nOvl = (long)ceil( (double)fieldSize * (1.0 - (ft/fss)) );

  // get sample format from config options
  if (forceSampleRate != 0) {
    sampleRate = forceSampleRate;
  } else {
    sampleRate = (long)( 1.0 / (fss / (double)fieldSize) );
  }

  sRiffPcmWaveHeader head; 
  head.Riff = 0x46464952;  // RIFF
  head.Format = 0x45564157; // WAVE
  head.Subchunk1ID = 0x20746D66; // fmt
  head.Subchunk1Size = 4*4; // size of format chunk
  head.SampleRate = sampleRate;
  head.BitsPerSample = nBitsPerSample;
  head.ByteRate = sampleRate * nChannels * nBytesPerSample;
  head.AudioFormat = sampleFormat == SMILE_SF_32BIT_FLOAT ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
  head.NumChannels = nChannels;
  head.BlockAlign = nChannels * nBytesPerSample;
  head.Subchunk2ID = 0x61746164; // data
  head.Subchunk2Size = nBlocks * nChannels * nBytesPerSample;  // size of wave data chunk
  head.FileSize = sizeof(sRiffPcmWaveHeader)  + head.Subchunk2Size;

  fseek(fHandle, 0, SEEK_SET);
  return (fwrite(&head, sizeof(sRiffPcmWaveHeader), 1, fHandle) == 1 ? sizeof(sRiffPcmWaveHeader) : 0 );
}

int cWaveSinkCut::writeDataFrame(cVector *m) 
{
  if (m->fmeta->N != nChannels) { SMILE_IERR(1,"number of chanels is inconsistent! %i <-> %i",m->fmeta->N,nChannels); return 0; }
  else {
    // convert data
    long i,j;
    if (( (m->fmeta->field[0].N - nOvl) > sampleBufferLen)&&(sampleBuffer!=NULL)) { free(sampleBuffer); sampleBuffer = NULL; }
    sampleBufferLen = m->fmeta->field[0].N - nOvl;
    if (sampleBufferLen<=0) {
      SMILE_IERR(1,"sampleBufferLen<=0! (%i), something went wrong with computing frame size and overlap!",sampleBufferLen);
      return 0;
    }

    if (sampleBuffer == NULL) sampleBuffer = malloc(nBytesPerSample*nChannels*sampleBufferLen);

    int8_t *b8;
    int16_t *b16;
    int32_t *b32;
    float *b32f;

    switch(sampleFormat) {
      case SMILE_SF_8BIT: 
        b8 = (int8_t*)sampleBuffer;
        for (i=0; i<sampleBufferLen; i++) { 
          for (j=0; j<nChannels; j++) { // conversion from separate channels to interleaved channels
            b8[i*nChannels+j] = (int8_t)round(m->data[i+sampleBufferLen*j] * 127.0);
          }
        }
        break;
      case SMILE_SF_16BIT: 
        b16 = (int16_t*)sampleBuffer;
        for (i=0; i<sampleBufferLen; i++) { 
          for (j=0; j<nChannels; j++) { // conversion from separate channels to interleaved channels
            b16[i*nChannels+j] = (int16_t)round(m->data[i+sampleBufferLen*j] * 32767.0); 
          }
        }
        break;
      case SMILE_SF_24BIT: 
        b32 = (int32_t*)sampleBuffer;
        for (i=0; i<sampleBufferLen; i++) { 
          for (j=0; j<nChannels; j++) { // conversion from separate channels to interleaved channels
            b32[i*nChannels+j] = (int32_t)round(m->data[i+sampleBufferLen*j] * 32767.0 * 256.0);
          }
        }
        break;
      case SMILE_SF_24BITp: 
        COMP_ERR("24-bit wave file with 3 bytes per sample encoding not yet supported!");
        //int16_t *b16 = buf;
        //for (i=0; i<m->nT*nChannels; i++) { b16[i] = (int16_t)round(m->data[i] * 32767.0); }
        //break;
      case SMILE_SF_32BIT: 
        b32 = (int32_t*)sampleBuffer;
        for (i=0; i<sampleBufferLen; i++) { 
          for (j=0; j<nChannels; j++) { // conversion from separate channels to interleaved channels
            b32[i*nChannels+j] = (int32_t)round(m->data[i+sampleBufferLen*j] * 32767.0 * 32767.0 * 2.0); 
          }
        }
        break;
      case SMILE_SF_32BIT_FLOAT: 
        b32f = (float*)sampleBuffer;
        for (i=0; i<sampleBufferLen; i++) { 
          for (j=0; j<nChannels; j++) { // conversion from separate channels to interleaved channels
            b32f[i*nChannels+j] = (float)(m->data[i+sampleBufferLen*j]); 
          }
        }
        break;
      default:
        SMILE_IERR(1,"unknown sampleFormat encountered in writeData(): %i",sampleFormat);
    }

    long written = (long)fwrite(sampleBuffer, nBytesPerSample*nChannels, sampleBufferLen, fHandle);
    //printf("written: %i of %i\n",written,m->nT);
    nBlocks += written;
    curWritePos += nBytesPerSample*nChannels * written;
    return written;
  }  
}
