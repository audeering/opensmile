/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#ifndef __DATA_MEMORY_HPP
#define __DATA_MEMORY_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <algorithm>
#include <initializer_list>
#include <memory>
#include <new>
#include <utility>
#include <vector>

// temporal frame ID
#define DMEM_IDX_ABS    -1    // no special index
#define DMEM_IDX_CURR   -11   // current read index
#define DMEM_IDX_CURW   -12   // current write index
#define DMEM_IDXR_CURR  -13   // start is relative to current read index
#define DMEM_IDXR_CURW  -14   // start is relative to current write index
#define DMEM_PAD_ZERO   -101  // pad with zeros
#define DMEM_PAD_FIRST  -102  // pad with first/last available frame (default)
#define DMEM_PAD_NONE   -103  // truncate frame (matrix) to actual available length

#define DMEM_FLOAT   0
#define DMEM_INT     1

#define LOOKAHEAD_ALLOC  20

/***** frame field dataType definitions *********/

#define DATATYPE_UNKNOWN                    0
#define DATATYPE_SIGNAL                     0x0100  // raw signal (pcm, etc.)
#define DATATYPE_SIGNAL_PCM                 0x0101
#define DATATYPE_SIGNAL_IMG                 0x0102
#define DATATYPE_SPECTRUM                   0x0200
#define DATATYPE_SPECTRUM_BINS_FIRST        0x0201
#define DATATYPE_SPECTRUM_BINS_COMPLEX      0x0201
#define DATATYPE_SPECTRUM_BINS_MAG          0x0202  // magnitude spectrum
#define DATATYPE_SPECTRUM_BINS_MAGPHASE     0x0242  // magnitude spectrum and phase in one array (mag first half, phase second half)
#define DATATYPE_SPECTRUM_BINS_PHASE        0x0203  // spectral phase 
#define DATATYPE_SPECTRUM_BINS_DBPSD        0x0204  // power spectral density in dB SPL
#define DATATYPE_SPECTRUM_BINS_POWSPECDENS  0x0205  // power spectral density 
#define DATATYPE_SPECTRUM_BINS_POWSPEC      0x0206  // power spectrum 
#define DATATYPE_SPECTRUM_BINS_SPECDENS     0x0207  // magnitude spectral density
#define DATATYPE_SPECTRUM_BINS_LAST         0x0207
#define DATATYPE_SPECTRUM_BANDS_MAG         0x0220
#define DATATYPE_COEFFICIENTS               0x0300
#define DATATYPE_ACF                        0x0380
#define DATATYPE_CEPSTRAL                   0x0400
#define DATATYPE_MIXED_LLD                  0x1000  // mixed low-level descriptors (e.g. pitch, etc.)
#define DATATYPE_FUNCTIONALS                0x2000  // functionals of lld, etc.

#ifndef MODULE
#define MODULE "dataMemory"
#define UNDEFMOD
#endif

#undef class

/**** frame meta information *********
   names, indicies (for each row)
 **************************************/

// meta information for one field...
class DLLEXPORT FieldMetaInfo { public:
  char *name;         // name of field
  int Nstart;         // start index (position in FrameMetaInfo::field)
  int N;              // number of elements (>1 ==> array)
  int dataType;       // dataType flag indicating type of data in this field (see dataType definitions in dataMemory.hpp)
  long infoSize;      // size of info struct in BYTES!
  int infoSet;        // flag that indicates whether the info has been set or not (dataType, *info, infoSize)
  void *info;         // custom info data (e.g. spectral scaling for spectra) 
  int arrNameOffset;

  FieldMetaInfo() : name(NULL), info(NULL), infoSize(0), dataType(DATATYPE_UNKNOWN), N(0), arrNameOffset(0) {}

  void copyFrom(FieldMetaInfo *f) {
    if (f!=NULL) {
      Nstart=f->Nstart;
      N = f->N;
      arrNameOffset = f->arrNameOffset;
      dataType = f->dataType;
      infoSize = f->infoSize;
      if (info != NULL) free(info);
      info = malloc(infoSize);
      memcpy(info, f->info, infoSize);
      if (name != NULL) free(name);
      if (f->name != NULL) name = strdup(f->name);
      else name = NULL;
    }
  }

  ~FieldMetaInfo() { 
    if (name!=NULL) free(name); 
    if (info!=NULL) free(info);
  }
};

class DLLEXPORT cVectorMeta { public:
  int ID;               // ID identifying the meta data type
  int iData[8];         // freely usable ints
  FLOAT_DMEM fData[8];  // freely usable floats
  char * text;          // custom meta-data text, will be auto-freed using free() if not NULL
  void * custom;        // custom meta data, will be auto-freed using free() if not NULL
  long customLength;    // length of data in custom (in bytes!), this is required for cloning a cVectorMeta data object! Set this to <= 0 to only copy the pointer and not free the pointer when the cVectorMeta object is freed!
  
  cVectorMeta() : ID(0), text(NULL), custom(NULL), customLength(0) {
    std::fill(iData, iData + 8, 0);
    std::fill(fData, fData + 8, 0.0);
  }

  cVectorMeta(const cVectorMeta &v) {
    ID = v.ID;
    std::copy(v.iData, v.iData + 8, iData);
    std::copy(v.fData, v.fData + 8, fData);

    if (v.text != NULL) {
      text = strdup(v.text);
    } else {
      text = NULL;
    }

    if ((v.customLength > 0)&&(v.custom != NULL)) {
      custom = malloc(v.customLength);
      memcpy(custom, v.custom, v.customLength);
      customLength = v.customLength;
    } else {
      // customLength <= 0 has a special meaning, see definition above
      custom = v.custom;
      customLength = v.customLength;
    }
  }

  cVectorMeta(cVectorMeta &&v) {
    ID = v.ID;
    std::move(v.iData, v.iData + 8, iData);
    std::move(v.fData, v.fData + 8, fData);
    text = v.text;
    v.text = NULL;
    custom = v.custom;
    v.custom = NULL;
    customLength = v.customLength;
    v.customLength = 0;
  }

  cVectorMeta &operator=(const cVectorMeta &v) {
    ID = v.ID;
    std::copy(v.iData, v.iData + 8, iData);
    std::copy(v.fData, v.fData + 8, fData);

    if (text != NULL) free(text);
    if (v.text != NULL) {      
      text = strdup(v.text);
    } else {
      text = NULL;
    }

    // if customLength <= 0, we are not supposed to free custom
    if ((customLength>0)&&(custom != NULL)) free(custom); 
    if ((v.customLength > 0)&&(v.custom != NULL)) {
      custom = malloc(v.customLength);
      memcpy(custom, v.custom, v.customLength);
      customLength = v.customLength;
    } else {
      // customLength <= 0 has a special meaning, see definition above
      custom = v.custom;
      customLength = v.customLength;
    }

    return *this;
  }

  cVectorMeta &operator=(cVectorMeta &&v) {
    ID = v.ID;
    std::move(v.iData, v.iData + 8, iData);
    std::move(v.fData, v.fData + 8, fData);

    if (text != NULL) free(text);
    text = v.text;
    v.text = NULL;

    // if customLength <= 0, we are not supposed to free custom
    if ((customLength>0)&&(custom != NULL)) free(custom); 
    custom = v.custom;
    v.custom = NULL;
    customLength = v.customLength;
    v.customLength = 0;
    
    return *this;
  }

  ~cVectorMeta() { 
    if (text != NULL) free(text); 
    // if customLength <= 0, we are not supposed to free custom
    if ((customLength>0)&&(custom != NULL)) free(custom); 
  }
};


// meta information for a complete vector 
class DLLEXPORT FrameMetaInfo {
  public:
    mutable smileMutex myMtx;
    long N, Ne;                // number of fields..
    FieldMetaInfo *field;      // field metadata for each of the fields
    cVectorMeta metaData;      // global metadata for the whole level (use tmeta->metadata for per frame meta data)

    FrameMetaInfo() : N(0), Ne(0), field(NULL) 
    {
      smileMutexCreate(myMtx);
    }
    
    // find a field by its full name (optionally including array index
    // returned *arrIdx will be the real index in the data structure, i.e. named index - arrNameOffset
    // if more != NULL, then *more (if > 0) will be used as a start field index for the search (i>=*more)
    //                  and *more will be filled with the number of additional matches found (0 if the returned field is the only match)
    int findField(const char*_fieldName, int *arrIdx=NULL, int *more=NULL) const;

    // find a field by a part of its name (optionally including array index) */
    // returned *arrIdx will be the real index in the data structure, i.e. named index - arrNameOffset
    // NOTE: if multiple names contain the same part, only the first one found will be returned! Solution: use the 'more' parameter:
    //   if more != NULL, then *more (if > 0) will be used as a start field index for the search (i>=*more)
    //                    and *more will be filled with the number of additional matches found (0 if the returned field is the only match)
    int findFieldByPartialName(const char*_fieldNamePart, int *arrIdx=NULL, int *more=NULL) const;

    long fieldToElementIdx(long _field, long _arrIdx=0) const;
    long elementToFieldIdx(long _element, long *_arrIdx=NULL) const;

    void printFieldNames() const {
      SMILE_PRINT("  Field name & dimension:");
      for (int i = 0; i < N; i++) {
        SMILE_PRINT("    %s %i", field[i].name, field[i].N);
      }
    }

    // n is element(!) index
    const char *getName(int n, int *arrIdx=NULL) const;

    ~FrameMetaInfo() {
      if (field != NULL) {
        for (int i=0; i<N; i++) { 
          if (field[i].name != NULL) free(field[i].name); 
          if (field[i].info!=NULL) free(field[i].info);
        }
        free(field);
      }
      smileMutexDestroy(myMtx);
    }
};

/**** temporal meta information *********
   length, times, etc. (for each column)
 **************************************/

class DLLEXPORT TimeMetaInfo { public:
  int filled;                             // whether info in this struct was already completed by setTimeMeta or the calling code
  long vIdx;                              // index of this frame in data memory level ?
  double period;                          // frame period of this level
  double time;                            // real start time in seconds
  double lengthSec;                       // real length in seconds
  double framePeriod;                     // frame period of previous level
  double smileTime;                       // smile time at which the frame (or a parent) was initially written to the data memory by a source (time measured in seconds since creation of component manager object)
  std::unique_ptr<cVectorMeta> metadata;  // optional custom meta-data for each vector/matrix, may be NULL if not present

  TimeMetaInfo() :
    filled(0), vIdx(0), period(0.0), time(0.0),
    lengthSec(0.0), framePeriod(0.0), smileTime(-1.0)
  {}

  TimeMetaInfo(const TimeMetaInfo &tm) :
    filled(tm.filled), vIdx(tm.vIdx), period(tm.period), time(tm.time),
    lengthSec(tm.lengthSec), framePeriod(tm.framePeriod), smileTime(tm.smileTime) {
    if (tm.metadata != NULL) {
      metadata = std::unique_ptr<cVectorMeta>(new cVectorMeta(*tm.metadata));
    }
  }

  TimeMetaInfo(TimeMetaInfo &&tm) : 
    filled(tm.filled), vIdx(tm.vIdx), period(tm.period), time(tm.time),
    lengthSec(tm.lengthSec), framePeriod(tm.framePeriod), smileTime(tm.smileTime),
    metadata(std::move(tm.metadata))
  {}

  TimeMetaInfo &operator=(const TimeMetaInfo &tm) {
    filled = tm.filled;
    vIdx = tm.vIdx;
    period = tm.period;
    time = tm.time;
    lengthSec = tm.lengthSec;
    framePeriod = tm.framePeriod;
    smileTime = tm.smileTime;
    if (tm.metadata != NULL) {
      metadata = std::unique_ptr<cVectorMeta>(new cVectorMeta(*tm.metadata));
    } else {
      metadata = NULL;
    }
    return *this;
  }

  TimeMetaInfo &operator=(TimeMetaInfo &&tm) {
    filled = tm.filled;
    vIdx = tm.vIdx;
    period = tm.period;
    time = tm.time;
    lengthSec = tm.lengthSec;
    framePeriod = tm.framePeriod;
    smileTime = tm.smileTime;
    metadata = std::move(tm.metadata);
    return *this;
  }
};



/**** dataMemory datatypes *********
 **************************************/

class DLLEXPORT cVector { 
  private:
    mutable char *ntmp;                  // temporary name for name(n) function

  protected:
    int tmetaAlien;                      // indicates that tmeta is a pointer to a struct allocated elsewhere
    int tmetaArr;                        // 1=tmeta is array, 0=tmeta is single object
  
  public:
    long N;                              // number of elements
    int type;                            // data type of this vector, DMEM_FLOAT or DMEM_INT
    TimeMetaInfo *tmeta;                 // usually tmeta is allocated individually for each vector, except if tmetaAlient is set
    FrameMetaInfo *fmeta;                // fmeta is always a pointer to memory in the dataMemory
    FLOAT_DMEM *dataF;                   // contains vector data if type == DMEM_FLOAT
    INT_DMEM *dataI;                     // contains vector data if type == DMEM_INT
    
    cVector(int lN, int ltype=DMEM_FLOAT, bool noTimeMeta=false);
    cVector(std::initializer_list<FLOAT_DMEM> elem);
    cVector(std::initializer_list<INT_DMEM> elem);

    // get name of expanded element n
    const char *name(int n, int *lN=NULL) const;

    FLOAT_DMEM getF(int n) const { return dataF[n]; }  // WARNING: index n is not checked!
    INT_DMEM getI(int n) const { return dataI[n]; }    // WARNING: index n is not checked!
    void setF(int n, FLOAT_DMEM v) { dataF[n]=v; }     // WARNING: index n is not checked!
    void setI(int n, INT_DMEM v) { dataI[n]=v; }       // WARNING: index n is not checked!

    const void *data() const { return type == DMEM_FLOAT ? (const void *)dataF : (const void *)dataI; }
    void *data() { return type == DMEM_FLOAT ? (void *)dataF : (void *)dataI; }

    void setTimeMeta(TimeMetaInfo *xtmeta) {
      if ((tmeta != NULL)&&(!tmetaAlien)) {
        if (tmetaArr) delete[] tmeta;
        else delete tmeta;
      }
      tmetaAlien = 1;
      tmetaArr = 0;
      tmeta = xtmeta;
    }

    void copyTimeMeta(const TimeMetaInfo *xtmeta) {
      if ((tmeta != NULL)&&(!tmetaAlien)) {
        if (tmetaArr) delete[] tmeta;
        else delete tmeta;
      }
      if (xtmeta != NULL) {
        tmeta = new TimeMetaInfo(*xtmeta);
      } else {
        tmeta = NULL;
      }
      tmetaAlien = 0;
      tmetaArr=0;
    }

    void resampleTimeMeta(long sourceRate, long targetRate, long vIdxStart) {
      COMP_ERR("cVector::resampleTimeMeta not implemented yet");
    }

    virtual ~cVector();
};


// memory organisation of cMatrix:
// array index x = col*N + row   ( t*N + n )
class DLLEXPORT cMatrix : public cVector { public:
  long nT;

  cMatrix(int lN, int lnT, int ltype=DMEM_FLOAT, bool noTimeMeta=false);  
  cMatrix(std::initializer_list<std::initializer_list<FLOAT_DMEM>> elem);
  cMatrix(std::initializer_list<std::initializer_list<INT_DMEM>> elem);

  FLOAT_DMEM getF(int n, int t) const { return dataF[n+t*N]; }  // WARNING: index n is not checked!
  INT_DMEM getI(int n, int t) const { return dataI[n+t*N]; }    // WARNING: index n is not checked!
  void setF(int n, int t, FLOAT_DMEM v) { dataF[n+t*N]=v; }     // WARNING: index n is not checked!
  void setI(int n, int t, INT_DMEM v) { dataI[n+t*N]=v; }       // WARNING: index n is not checked!

  const void *data() const { return type == DMEM_FLOAT ? (const void *)dataF : (const void *)dataI; }
  void *data() { return type == DMEM_FLOAT ? (void *)dataF : (void *)dataI; }

  cMatrix * getRow(long R) const {
    bool noTimeMeta = tmeta == NULL;
    cMatrix *r = new cMatrix(1,nT,type,noTimeMeta);
    cMatrix *ret = getRow(R,r);
    if (ret==NULL) delete r;
    return ret;
/*    long i;
    if (type==DMEM_FLOAT) for (i=0; i<nT; i++) { r->dataF[i] = dataF[i*N+R]; }
    else if (type==DMEM_INT) for (i=0; i<nT; i++) { r->dataI[i] = dataI[i*N+R]; }
    else { delete r; return NULL; }
    r->setTimeMeta(tmeta);
    return r;*/
  }

  cMatrix * getRow(long R, cMatrix *r) const {
    long i;
    bool noTimeMeta = tmeta == NULL;
    if (r==NULL) r = new cMatrix(1,nT,type,noTimeMeta);
    else {
      if (r->nT != nT) { delete r; r = new cMatrix(1,nT,type,noTimeMeta); }
      if (type != r->type) return NULL;
    }
    long nn = MIN(nT,r->nT);
    if (type==DMEM_FLOAT) {
      FLOAT_DMEM *df = dataF+R;
      for (i=0; i<nn; i++) { r->dataF[i] = *df; df += N; /*dataF[i*N+R];*/ }
      for ( ; i<r->nT; i++) { r->dataF[i] = 0.0; }
    } else if (type==DMEM_INT) {
      INT_DMEM *di = dataI+R;
      for (i=0; i<nn; i++) { r->dataI[i] = *di; di += N; /*dataI[i*N+R];*/ }
    } else { /*delete r;*/ return NULL; }
    r->setTimeMeta(tmeta);
    return r;
  }

  cVector* getCol(long C) const {
    bool noTimeMeta = tmeta == NULL;
    cVector *c = new cVector(N,type,noTimeMeta);
    long i;
    if (type==DMEM_FLOAT) for (i=0; i<N; i++) { c->dataF[i] = dataF[N*C+i]; }
    else if (type==DMEM_INT) for (i=0; i<N; i++) { c->dataI[i] = dataI[N*C+i]; }
    else { delete c; return NULL; }
    c->setTimeMeta(tmeta);
    return c;
  }

  cVector* getCol(long C, cVector *c) const {
    bool noTimeMeta = tmeta == NULL;
    if (c==NULL) c = new cVector(N,type,noTimeMeta);
    long i;
    if (type==DMEM_FLOAT) for (i=0; i<N; i++) { c->dataF[i] = dataF[N*C+i]; }
    else if (type==DMEM_INT) for (i=0; i<N; i++) { c->dataI[i] = dataI[N*C+i]; }
    else { delete c; return NULL; }
    c->setTimeMeta(tmeta);
    return c;
  }

  void setRow(long R, const cMatrix *row) { // NOTE: set row does not change tmeta!!
    long i;
    if (row!=NULL) {
      long nn = MIN(nT,row->nT);
      if (row->type == type) {
        if (type==DMEM_FLOAT) for (i=0; i<nn; i++) { dataF[i*N+R] = row->dataF[i]; }
        else if (type==DMEM_INT) for (i=0; i<nn; i++) { dataI[i*N+R] = row->dataI[i]; }
      } else {
        if (type==DMEM_FLOAT) for (i=0; i<nn; i++) { dataF[i*N+R] = (FLOAT_DMEM)(row->dataI[i]); }
        else if (type==DMEM_INT) for (i=0; i<nn; i++) { dataI[i*N+R] = (INT_DMEM)(row->dataF[i]); }
      }
    }
  }
  
  int resize(long _new_nT) {
    int ret=1;
    if (_new_nT < nT) return 1;

    // dataF / dataI
    if (type == DMEM_FLOAT) {
      FLOAT_DMEM *tmp = (FLOAT_DMEM *)crealloc(dataF, _new_nT*sizeof(FLOAT_DMEM)*N, nT*sizeof(FLOAT_DMEM)*N);
      if (tmp==NULL) ret = 0;
      else dataF = tmp;
    } else if (type == DMEM_INT) {
      INT_DMEM *tmp = (INT_DMEM *)crealloc(dataI, _new_nT*sizeof(INT_DMEM)*N, nT*sizeof(INT_DMEM)*N);
      if (tmp==NULL) ret = 0;
      else dataI = tmp;
    }

    if (ret) {
      // tmeta
      if (tmeta != NULL) {
        TimeMetaInfo *old = tmeta;
        tmeta = new (std::nothrow) TimeMetaInfo[_new_nT];
        if (tmeta == NULL) { ret=0; tmeta = old; }
        else {          
          if (!tmetaAlien) {
            std::move(old, old + nT, tmeta);
            delete[] old;
          } else {
            std::copy(old, old + nT, tmeta);
          }
          tmetaAlien = 0;
        }
      }
    }

    if (ret) nT = _new_nT;

    return ret;
  }

private:
  // helper method to copy blocks of FLOAT_DMEM or INT_DMEM data 
  void copyData(void *dest, long destIdx, void *src, long srcIdx, long nT, long N, int type, bool zeroSource) {
    if (type == DMEM_FLOAT) {
      memcpy(&((FLOAT_DMEM *)dest)[destIdx * N], &((FLOAT_DMEM *)src)[srcIdx * N], sizeof(FLOAT_DMEM) * N * nT);
      if (zeroSource) {
        memset(&((FLOAT_DMEM *)src)[srcIdx * N], 0, sizeof(FLOAT_DMEM) * N * nT);
      }
    } else if (type == DMEM_INT) {
      memcpy(&((INT_DMEM *)dest)[destIdx * N], &((INT_DMEM *)src)[srcIdx * N], sizeof(INT_DMEM) * N * nT);
      if (zeroSource) {
        memset(&((INT_DMEM *)src)[srcIdx * N], 0, sizeof(INT_DMEM) * N * nT);
      }
    }
  }

public:
  // copy part of the data in this matrix to a buffer
  void copyTo(void *dest, long destIdx, long srcIdx, long nT, bool zeroSource = false) {
    copyData(dest, destIdx, data(), srcIdx, nT, N, type, zeroSource);
  }

  // copy part of the data in this matrix to another matrix
  void copyTo(cMatrix &dest, long destIdx, long srcIdx, long nT, bool zeroSource = false) {
    copyData(dest.data(), destIdx, data(), srcIdx, nT, N, type, zeroSource);
  }

  // copy part of the data in another matrix to this matrix
  void copyFrom(cMatrix &src, long srcIdx, long destIdx, long nT, bool zeroSource = false) {
    copyData(data(), destIdx, src.data(), srcIdx, nT, N, type, zeroSource);
  }

  // copy part of the data in a buffer to this matrix
  void copyFrom(void *src, long srcIdx, long destIdx, long nT, bool zeroSource = false) {
    copyData(data(), destIdx, src, srcIdx, nT, N, type, zeroSource);
  }

  // transpose matrix
  void transpose();

  // reshape to given dimensions, perform sanity check...
  void reshape(long R, long C);

  // convert matrix to long vector with all columns concatenated
  cVector* expand();

  void setTimeMeta(TimeMetaInfo *_tmeta) {
    if ((tmeta != NULL)&&(!tmetaAlien)) {
      if (tmetaArr) delete[] tmeta;
      else delete tmeta;
      tmetaAlien = 1;
    }
    tmeta = _tmeta; tmetaArr=1;
  }

  void copyTimeMeta(const TimeMetaInfo *_tmeta, long _nT=-1) {
    if (_nT == -1) _nT = nT;
    if ((tmeta != NULL)&&(!tmetaAlien)) {
      if (tmetaArr) delete[] tmeta;
      else delete tmeta;
    }
    if (_tmeta != NULL) {
      tmeta = new TimeMetaInfo[nT];
      std::copy(_tmeta, _tmeta + (std::min)(nT, _nT), tmeta);
    } else {
      tmeta = NULL;
    }
    tmetaAlien = 0; tmetaArr=1;
  }

  // convert a matrix tmeta to a vector tmeta spanning the whole matrix by adjusting size, period, etc.
  void squashTimeMeta(double period=-1.0) {
    if (tmeta != NULL) {
      tmeta[0].framePeriod = tmeta[0].period;
      if (period != -1.0)
        tmeta[0].period = period;
      //else   // TODO::: there is bug here......
      //  tmeta[0].period = tmeta[nT-1].time - tmeta[0].time + tmeta[nT-1].lengthSec;
      tmeta[0].lengthSec = tmeta[nT-1].time - tmeta[0].time + tmeta[nT-1].lengthSec;
    }
  }

  void resampleTimeMeta(long sourceRate, long targetRate, long vIdxStart) {
    COMP_ERR("cMatrix::resampleTimeMeta not implemented yet");
  }
  
  ~cMatrix();
};

/******* dataMemory level class ************/

/* result codes for getFrame/Matrix setFrame/Matrix */
#define DMRES_OK        0
#define DMRES_ERR       1
#define DMRES_OORleft   4
#define DMRES_OORright  8
#define DMRES_OORbs     16

/* level configuration */
class DLLEXPORT sDmLevelConfig { public:
  /* timing configuration */
  double T;                 // period of frames in this level ("frameStep" in seconds)
  double frameSizeSec;      // size of ONE frame in seconds
  double lastFrameSizeSec;  // size of ONE frame in seconds, previous level (or level before frameSizeSec was modifed)
  long nT;                  // level bufferSize in frames
  double lenSec;            // length of buffer in seconds ("frameStep"*nT)
  double basePeriod;        // period of top most level, if available, else 0

  /* blocksizes */
  long blocksizeWriter;     // blocksize of writer
  long blocksizeReader;     // maximum blocksize of reader(s)
  long minBlocksizeReader;  // minimum blocksize of reader(s)

  /* level type */
  int isRb;                 // whether the level is backed by a ringbuffer
  int noHang;               // controls the 'hang' behaviour for ring-buffer levels, see noHang field of cDataMemory component 
  int growDyn;              // grow level size dynamically if needed
  int type;                 // data type of data stored in the level, DMEM_FLOAT or DMEM_INT

  /* config state: if no flag is set, only timing and type config is set */
  int finalised;            // timing, type, blocksize, and names config is ready AND level mem is allocated
  int blocksizeIsSet;       // timing, type, and blocksize config is ready
  int namesAreSet;          // timing, type, blocksize, and names config is ready

  /* miscellanious */
  long N, Nf;               // number of elements, number of fields (Nf is same as fmeta->N)
  FrameMetaInfo *fmeta;     // pointer to fmeta, which is allocated in the corresponding dataMemory level
  char *name;               // level name
  bool noTimeMeta;          // whether tmeta should not be stored to conserve memory
                            // frames read from the level will have basic time metadata generated on-the-fly

  sDmLevelConfig(double _T, double _frameSizeSec, long _nT=10, int _type=DMEM_FLOAT, int _isRb=1) :
    T(_T), frameSizeSec(_frameSizeSec), lastFrameSizeSec(_frameSizeSec), nT(_nT), lenSec(0.0), basePeriod(0.0),
    blocksizeWriter(1), blocksizeReader(1), minBlocksizeReader(-1),
    isRb(_isRb), noHang(1), growDyn(0),
    type(_type),
    finalised(0), blocksizeIsSet(0), namesAreSet(0),
    N(0), Nf(0), 
    fmeta(NULL),
    name(NULL),
    noTimeMeta(false)
  { 
    lenSec = T*(double)nT; 
  }

  sDmLevelConfig(double _T, double _frameSizeSec, double _lenSec=1.0, int _type=DMEM_FLOAT, int _isRb=1) :
    T(_T), frameSizeSec(_frameSizeSec), lastFrameSizeSec(_frameSizeSec), nT(0), lenSec(_lenSec), basePeriod(0.0),
    blocksizeWriter(1), blocksizeReader(1), minBlocksizeReader(-1),
    isRb(_isRb), noHang(1), growDyn(0),
    type(_type),
    finalised(0), blocksizeIsSet(0), namesAreSet(0),
    N(0), Nf(0), 
    fmeta(NULL),
    name(NULL),
    noTimeMeta(false)
  { 
    if (T!=0.0) nT = (long)ceil(lenSec/T); 
  }

  sDmLevelConfig(const char *_name, double _T, double _frameSizeSec, long _nT=10, int _type=DMEM_FLOAT, int _isRb=1) :
    T(_T), frameSizeSec(_frameSizeSec), lastFrameSizeSec(_frameSizeSec), nT(_nT), lenSec(0.0), basePeriod(0.0),
    blocksizeWriter(1), blocksizeReader(1), minBlocksizeReader(-1),
    isRb(_isRb), noHang(1), growDyn(0),
    type(_type),
    finalised(0), blocksizeIsSet(0), namesAreSet(0),
    N(0), Nf(0), 
    fmeta(NULL),
    name(NULL),
    noTimeMeta(false)
  { 
    lenSec = T*(double)nT; 
    if (_name != NULL) name = strdup(_name);
  }

  sDmLevelConfig(const char *_name, double _T, double _frameSizeSec, double _lenSec=1.0, int _type=DMEM_FLOAT, int _isRb=1) :
    T(_T), frameSizeSec(_frameSizeSec), lastFrameSizeSec(_frameSizeSec), nT(0), lenSec(_lenSec), basePeriod(0.0),
    blocksizeWriter(1), blocksizeReader(1), minBlocksizeReader(-1),
    isRb(_isRb), noHang(1), growDyn(0),
    type(_type),
    finalised(0), blocksizeIsSet(0), namesAreSet(0),
    N(0), Nf(0), 
    fmeta(NULL),
    name(NULL),
    noTimeMeta(false)
  { 
    if (T!=0.0) nT = (long)ceil(lenSec/T); 
    if (_name != NULL) name = strdup(_name);  
  }

  sDmLevelConfig() :
    T(0.0), frameSizeSec(0.0), lastFrameSizeSec(0.0), nT(0), lenSec(0.0), basePeriod(0.0),
    blocksizeWriter(1), blocksizeReader(1), minBlocksizeReader(-1),
    isRb(1), noHang(1), growDyn(0),
    type(DMEM_FLOAT),
    finalised(0), blocksizeIsSet(0), namesAreSet(0),
    N(0), Nf(0), 
    fmeta(NULL),
    name(NULL),
    noTimeMeta(false) {}

  sDmLevelConfig(sDmLevelConfig const &orig) :
    T(orig.T), frameSizeSec(orig.frameSizeSec), lastFrameSizeSec(orig.lastFrameSizeSec), nT(orig.nT), lenSec(orig.lenSec), basePeriod(orig.basePeriod),
    blocksizeWriter(orig.blocksizeWriter), blocksizeReader(orig.blocksizeReader), minBlocksizeReader(orig.minBlocksizeReader),
    isRb(orig.isRb), noHang(orig.noHang), growDyn(orig.growDyn),
    type(orig.type),
    finalised(orig.finalised), blocksizeIsSet(orig.blocksizeIsSet), namesAreSet(orig.namesAreSet),
    N(orig.N), Nf(orig.Nf), 
    fmeta(orig.fmeta),
    name(NULL),
    noTimeMeta(orig.noTimeMeta) { 
      if (orig.name != NULL) name = strdup(orig.name); 
    }


  sDmLevelConfig(const char *_name, sDmLevelConfig &orig) :
    T(orig.T), frameSizeSec(orig.frameSizeSec), lastFrameSizeSec(orig.lastFrameSizeSec), nT(orig.nT), lenSec(orig.lenSec), basePeriod(orig.basePeriod),
    blocksizeWriter(orig.blocksizeWriter), blocksizeReader(orig.blocksizeReader), minBlocksizeReader(orig.minBlocksizeReader),
    isRb(orig.isRb), noHang(orig.noHang), growDyn(orig.growDyn),
    type(orig.type),
    finalised(orig.finalised), blocksizeIsSet(orig.blocksizeIsSet), namesAreSet(orig.namesAreSet),
    N(orig.N), Nf(orig.Nf), 
    fmeta(orig.fmeta),
    name(NULL),
    noTimeMeta(orig.noTimeMeta) { 
      if (_name != NULL) name = strdup(_name); 
      else if (orig.name != NULL) name = strdup(orig.name); 
    }

  void updateFrom(const sDmLevelConfig &orig) 
  {
    T = orig.T;
    frameSizeSec = orig.frameSizeSec;
    lastFrameSizeSec = orig.lastFrameSizeSec;
    nT = orig.nT;
    lenSec = orig.lenSec;
    basePeriod = orig.basePeriod;
    blocksizeWriter = orig.blocksizeWriter;
    blocksizeReader = orig.blocksizeReader;
    minBlocksizeReader = orig.minBlocksizeReader;
    isRb = orig.isRb;
    noHang = orig.noHang;
    growDyn = orig.growDyn;
    type = orig.type;
    finalised = orig.finalised;
    blocksizeIsSet = orig.blocksizeIsSet;
    namesAreSet = orig.namesAreSet;
    N = orig.N; 
    Nf = orig.Nf; 
    fmeta = orig.fmeta;
    if (name != NULL) {
      free(name);
      name = NULL;
    }  
    if (orig.name != NULL) name = strdup(orig.name);
    noTimeMeta = orig.noTimeMeta;
  }

  void setName(const char *_name) {
    if (_name != NULL) {
      if (name != NULL)
        free(name);
      name = strdup(_name);
    }
  }

  ~sDmLevelConfig() {
    if (name != NULL)
      free(name);
  }
};

class DLLEXPORT cDataMemory;

class DLLEXPORT cDataMemoryLevel {
  private:
    //char *name;

    int myId;
    cDataMemory * _parent;

    /* variables for mutex access and locking */
    mutable smileMutex RWptrMtx;  // mutex to lock global curW / curR pointers
    mutable smileMutex RWmtx;     // mutex to lock data area during read / write
    mutable smileMutex RWstatMtx; // mutex to lock nCurRdr and writeReq variables for mut.ex. write/read op. while allowing mutliple parallel reads
    int nCurRdr;
    int writeReqFlag;
    
    /* level configuration */
    sDmLevelConfig lcfg;
    FrameMetaInfo fmeta;
    int fmetaNalloc;

    /* level buffer and buffer status */
    cMatrix *data;          // level buffer
    long curW,curR;         // current write pos, current read pos    (min (read) over all readers / max (write))
    long *curRr;            // current current read pos for each registered reader
    int nReaders;           // number of registered readers (all registered readers will be "waited" for! if you don't want that, don't register your reader)
    long minRAtLastGrowth;  // minimum read index (minR) at the time of the last level growth (only valid for growable ring-buffer levels)

    /* timing information for every frame in the buffer */ /* ISN'T THIS IN *data ?? */
    TimeMetaInfo  *tmeta;

    int EOI;
    int EOIcondition;

    /* check current read indicies of all registered readers for this level and update global (minimal) read index curRm
       also update single reader's read index if they have fallen behind the global read index for whatever reason. */
    void checkCurRr();

    /* resize the level to a bigger size (used by growDyn option) */
    int growLevel(long newSize);
    
    // validate write index and, if applicable, increase curW write counter
    long validateIdxW(long *vIdx, int special=-1);

    // validate write index range, and if applicable set curW write counter to end of range+1
    long validateIdxRangeW(long *vIdx, long vIdxEnd, int special=-1);

    // validate read index, 
    // return value: -1 invalid param, -2 vidx OOR_left, -3 vidx OOR_right, -4 vidx OOR_buffersize(noRb)
    long validateIdxR(long *vIdx, int special=-1, int rdId=-1, int noUpd=0);

    // validate read index range, vIdxEnd is the index after the last index to read... (i.e. vIdx + len)
    // TODO: error codes
    long validateIdxRangeR(long actualVidx, long *vIdx, long vIdxEnd, int special=-1, int rdId=-1, int noUpd=0, int *padEnd=NULL);

    // write frame data from *_data to level's data matrix at pos rIdx
    void frameWr(long rIdx, const FLOAT_DMEM *_data);
    void frameWr(long rIdx, const INT_DMEM *_data);

    // write frame data from level's data matrix at pos rIdx to *_data
    void frameRd(long rIdx, FLOAT_DMEM *_data) const;
    void frameRd(long rIdx, INT_DMEM *_data) const;

    void setTimeMeta(long rIdx, long vIdx, const TimeMetaInfo *tm);
    void getTimeMeta(long rIdx, long vIdx, TimeMetaInfo *tm) const;

  public:

    // create level from given level configuration struct, the name in &cfg will be overwritten via _name parameter
    cDataMemoryLevel(int _levelId, sDmLevelConfig &cfg, const char *_name = NULL) :
      myId(_levelId), _parent(NULL),
      lcfg(_name, cfg), fmetaNalloc(0),
      data(NULL), tmeta(NULL), EOI(0), EOIcondition(0),
      curW(0), curR(0), curRr(NULL), nReaders(0), 
      minRAtLastGrowth(0), nCurRdr(0), writeReqFlag(0)
    {
      //if ((nT == 0)&&(cfg.lenSec > 0.0)&&(cfg.T>0.0)) { nT = (long)ceil( cfg.lenSec / cfg.T ); }
      if (lcfg.T < 0.0) COMP_ERR("cannot create dataMemoryLevel with period (%f) < 0.0",lcfg.T);
      if (lcfg.nT <= 0) COMP_ERR("cannot create empty dataMemoryLevel nT = %i <= 0",lcfg.nT);
      lcfg.fmeta = &fmeta;
    }

    // get level ID
    int getId() const { return myId; }

    // get level name
    const char *getName() const { return lcfg.name; }

    // get/set EOI and EOIcounter
    void setEOI() { EOIcondition = 1; }
    void unsetEOI() { EOIcondition = 0; }
    int isEOI() const { return EOIcondition; }
    void setEOIcounter(int cnt) { EOI = cnt; };
    int getEOIcounter() const { return EOI; }

    // query, if names are set, i.e. if level was fixated
    int namesAreSet() const { return lcfg.namesAreSet; }
    // after the names were set by the writer, this must be indicated to the level via a call to fixateLevel()
    void fixateLevel() { 
      lcfg.namesAreSet = 1; 
//      lcfg.fmeta = &fmeta;
    }

    // set parent dataMemory object
    void setParent(cDataMemory * __parent) { _parent = __parent; }

    // adds a field to this level, _N is the number of elements in an array field, set to 0 or 1 for scalar field
    // arrNameOffset: start index for creating array element names
    int addField(const char *lname, int lN, int arrNameOffset=0);
    
    // set info for field i (dataType and custom data pointer)
    // Note: the memory pointed to by _info must have been allocated with malloc and is freed upon destruction
    // of the data memory level by the framemeta object!
    int setFieldInfo(int i, int _dataType, void *_info, long _infoSize);
    
    // set info for field 'name' (dataType and custom data pointer). Do NOT include array indices in field name!
    // Note: the memory pointed to by _info must have been allocated with malloc and is freed upon destruction
    // of the data memory level by the framemeta object!
    int setFieldInfo(const char *__name, int _dataType, void *_info, long _infoSize) {
      return setFieldInfo(fmeta.findField(__name), _dataType, _info, _infoSize);
    }

    // register a new data reader... IS THAT ALL ??? yes: we only need to know how many there are, the rest is done in allocReaders()
    int registerReader() { return nReaders++; }

    // allocate config for the readers and initialize it with standard values
    void allocReaders();

    // configure level (check buffersize and config)
    int configureLevel();   

    // finalize config and allocate data memory
    int finaliseLevel();   

    // find a field by its full name in a finalised level
    // *arrIdx (if not NULL) will be set to the array index of the requested element, if it is in an array field
    int findField(const char*_fieldName, int *arrIdx) const { 
      return fmeta.findField( _fieldName, arrIdx );
    }

    // sets the arrNameOffset (see addField) of the field that was added last
    void setArrNameOffset(int arrNameOffset);
   
    // get a pointer to the custom level meta data (in fmeta), a cVectorMeta class (same class as for each frame in tmeta, but different data!)
    cVectorMeta * getLevelMetaDataPtr() { return &(fmeta.metaData); }

    /* set a frame or matrix beginning at vIdx, or special index (overrides value of vIdx, see below) */
    // per level always a complete frame must be set...
    // int special = special index, e.g. DMEM_IDX_CURR, DMEM_IDX_CURW
    int setFrame(long vIdx, const cVector *vec, int special=-1);  
    int setMatrix(long vIdx, const cMatrix *mat, int special=-1);

    /* get a frame or matrix beginning at vIdx, or special index (overrides value of vIdx, see below) */
    // int special = special index, e.g. DMEM_IDX_CURR, DMEM_IDX_CURW
    // for getFrame, *result (if not NULL) will contain a result code indicating success or reason of failure (left or right buffer margin exceeded, etc.)
    // for getMatrix, *result (if not NULL) will contain a result code indicating success or failure (but not the reason of failure)
    // rdId is the id of the current reader (or -1 for an unregistered or global reader)
    cVector * getFrame(long vIdx, int special=-1, int rdId=-1, int *result=NULL);  
    cMatrix * getMatrix(long vIdx, long vIdxEnd, int special=-1, int rdId=-1, int *result=NULL);  

    /* check if a read of length "len" at vIdx or "special" will succeed for reader rdId */
    // *result (if not NULL) will contain a result code indicating success or reason of failure (left or right buffer margin exceeded, etc.)
    // possible result values: (Doc TODO) -1 invalid param, -2 vidx OOR_left, -3 vidx OOR_right, -4 vidx OOR_buffersize(noRb)
    int checkRead(long vIdx, int special=-1, int rdId=-1, long len=1, int *result=NULL) 
    {
      long rIdx;
      long vIdxEnd = vIdx + len;
      long vIdxold = vIdx;
      if ((vIdx < 0)&&(vIdxEnd > 0)) vIdx = 0;
      if (len < 0) return 0;

      smileMutexLock(RWptrMtx);
      if (len<=1) rIdx = validateIdxR(&vIdx,special,rdId,1);
      else rIdx = validateIdxRangeR(vIdxold,&vIdx,vIdxEnd,special,rdId,1);

      smileMutexUnlock(RWptrMtx);
      if (result!=NULL) {
        if (rIdx == -2) *result=DMRES_OORleft|DMRES_ERR;
        else if (rIdx == -3) *result=DMRES_OORright|DMRES_ERR;
        else if (rIdx == -4) *result=DMRES_OORbs|DMRES_ERR;
        else if (rIdx >= 0) *result=DMRES_OK;
        else *result=DMRES_ERR;
      }

      // TODO: it might be possible that checkRead returns ok, then another reader reads data, 
      // a writer writes new data, and the read will fail which was confirmed by checkRead.... 
      // ?? what do we do about it ??
      // maybe use level lock() functions, that the caller may (or may not) use...

      if (rIdx >= 0) return 1;  // a read will likely succeed (see above TODO notice)
      else return 0;  // an error occurred, read is not possible
    }

    /* set read index of reader rdId (or global read index, if rdId=-1) 
       to user-defined _curR or to write index curW, if _curR=-1 or omitted.
       if _curR=-1 or omitted, thus, the level buffer will be cleared (not physically but logically)
       use this, if your reader only requires data "part time" (e.g. reading only voiced segments of speech, etc.)
      */
    void catchupCurR(int rdId=-1, int _curR = -1);

    /* methods to get info about current level fill status 
       (e.g. number of frames written, curW, curR(global) and freeSpace, etc.) */

    /* get current write index (index that will be written to NEXT) */
    long getCurW() const 
    {
      smileMutexLock(RWptrMtx);
      long res = curW;
      smileMutexUnlock(RWptrMtx);
      return res;
    }  

    /* current read index (index that will be read NEXT) of reader rdId or global (min of all readers) if rdId is omitted */
    long getCurR(int rdId=-1) const
    {
      long res;
      smileMutexLock(RWptrMtx);
      if ((rdId < 0)||(rdId >= nReaders)) { 
        res = curR;
      } else {
        res = curRr[rdId];
      }
      smileMutexUnlock(RWptrMtx);
      return res;
    }  

    /* get number of free frames in current level, i.e. the number of frames that can 
       currently be written without having to resize the buffer (for ringbuffers: nT-(curW-curR)) */
    long getNFree(int rdId=-1) const
    { 
      long ret=0;
      smileMutexLock(RWptrMtx);
      if (lcfg.isRb) {
        if ((rdId>=0)&&(rdId<nReaders)) {
          SMILE_DBG(5,"getNFree(rdId=%i) level='%s' curW=%i curRr=%i nT=%i free=%i",rdId,getName(),curW,curRr[rdId],lcfg.nT,lcfg.nT - (curW-curRr[rdId]));
          ret = lcfg.nT - (curW-curRr[rdId]);
        } else {
          SMILE_DBG(5,"getNFree:: level='%s' curW=%i curR=%i nT=%i free=%i",getName(),curW,curR,lcfg.nT,lcfg.nT - (curW-curR));
          ret = lcfg.nT - (curW-curR);
        }
      } else {
        ret = (lcfg.nT - curW);
      }
      smileMutexUnlock(RWptrMtx);
      return ret;
    }
    
    /* get number of available frames for reader rdId */
    long getNAvail(int rdId=-1) const
    { 
      long ret=0;
      smileMutexLock(RWptrMtx);
      if ((rdId>=0)&&(rdId<nReaders)) {
        SMILE_DBG(5,"getNAvail(rdId=%i) level='%s' curW=%i curRr=%i nT=%i avail=%i",rdId,getName(),curW,curRr[rdId],lcfg.nT,(curW-curRr[rdId]));
        ret = curW - curRr[rdId];
      } else {
        SMILE_DBG(5,"getNAvail:: level='%s' curW=%i curR=%i nT=%i avail=%i",getName(),curW,curR,lcfg.nT,(curW-curR));
        ret = curW - curR;
      }
      smileMutexUnlock(RWptrMtx);
      return ret;
    }

    /* get maximum readable index ("last" index where data was written to) */
    long getMaxR() const;

    /* get minimum readable index (relevant only for ringbuffers, for regular buffers this function will always return 0) */
    long getMinR() const;  

    /* get level configuration */
    const sDmLevelConfig * getConfig() const { return &lcfg; }

    /* get number of registered readers (if the level is not finalised, it is the number of readers *currently* registered!) */
    long getNreaders() const { return nReaders; }

    /* check if level is ready for reading, get level config (WITHOUT NAMES!) and update blocksizeReader */
    const sDmLevelConfig * queryReadConfig(long blocksizeReader)
    {
      if (!lcfg.blocksizeIsSet) {
        SMILE_MSG(4, "query read config [%s]: %ld (min %ld, max: %ld)", lcfg.name, blocksizeReader,
            lcfg.minBlocksizeReader, lcfg.blocksizeReader);
        if (lcfg.minBlocksizeReader == -1 ||
            lcfg.minBlocksizeReader > blocksizeReader) {
          if (blocksizeReader > 0)
            lcfg.minBlocksizeReader = blocksizeReader;
        }
        // maximum reader blocksize update
        if (lcfg.blocksizeReader < blocksizeReader)
          lcfg.blocksizeReader = blocksizeReader;
        return &lcfg; 
      } else {
        SMILE_ERR(1,"attempt to update blocksizeReader, however blocksize config for level '%s' is already fixed!", lcfg.name);
        return NULL;
      }
    }

    /* check if level is ready for reading, get level config (WITHOUT NAMES!) and update blocksizeReader */
    const sDmLevelConfig * queryReadConfig(double blocksizeReaderSeconds)
    {
      if (lcfg.T != 0.0)
        return queryReadConfig((long)ceil(blocksizeReaderSeconds / lcfg.T));
      else
        return queryReadConfig((long)ceil(blocksizeReaderSeconds));
    }

    // TODO: should these methods be passed to fmeta... or should they use getConfig() and call the proper fmeta methods??
    /* get name of field with index "n" (n = 0..lcfg.Nf-1  OR equivalent: n = 0..lcfg.fmeta.N-1), 
      *_N will be filled with number of elements in this field. 
      for *_arrNameOffset see documentation of addField */
    const char * getFieldName(int n, int *lN=NULL, int *larrNameOffset=NULL) const;

    /* get name of element "n"  (n= 0..lcfg.N) */
    // IMPORTANT! the caller must free() the returned string!!
    char * getElementName(int n) const; 
    
    /**** set config functions ***/
    /* set frame size in seconds */
    void setFrameSizeSec(double fss) { if (fss > 0.0) lcfg.frameSizeSec = fss; }
    void setBlocksizeWriter(long _bsw, int _force=0) { 
      if (_force) { lcfg.blocksizeWriter = _bsw; }
      else if (_bsw > lcfg.blocksizeWriter) { lcfg.blocksizeWriter = _bsw; }
    }

    /**** vIdx from/to real-time conversion functions (preferred method) */
    // TODO: (for periodic levels this function is easy, however for aperiodic levels we must iterate through all frames...
    //       maybe also for periodic to compensate round-off errors?)
    /* seconds to vIdx (for THIS level!) */
    long secToVidx(double sec) const; 
    /* vIdx (for THIS level!) to real-time in seconds */
    double vIdxToSec(long vIdx) const; 

    /** print number of readers, and size of level in frames and seconds, etc.. **/
    // available levels of detail : 0, 1, 2
    void printLevelStats(int detail=1) const;    

    ~cDataMemoryLevel() {
      // if level has never been finalized, the mutexes are remain unitialized
      if (lcfg.finalised) {
        smileMutexDestroy(RWptrMtx);
        smileMutexDestroy(RWstatMtx);
        smileMutexDestroy(RWmtx);
      }

      if (tmeta != NULL) delete[] tmeta;
      if (data != NULL) delete data;
      if (curRr != NULL) free(curRr);
    }
};

#ifdef UNDEFMOD
#undef MODULE
#undef UNDEFMOD
#endif


/************** dataMemory class ***********************/

// represents a request by a component instance to read or write from/to a level
struct DLLEXPORT sDmLevelRWRequest {
  const char *instanceName;
  const char *levelName;

  sDmLevelRWRequest(const char *_instanceName, const char *_levelName) :
    instanceName(_instanceName), levelName(_levelName) {}
};

#define COMPONENT_DESCRIPTION_CDATAMEMORY "central data memory component"
#define COMPONENT_NAME_CDATAMEMORY "cDataMemory"

class DLLEXPORT cDataMemory : public cSmileComponent {
  private:
    cDataMemoryLevel **level;  // data memory levels
    int nLevelsAlloc;          // total number of levels allocated
    int nLevels;               // actual number of levels initialized - 1

    std::vector<sDmLevelRWRequest> rrq;  // read requests of component instances to levels
    std::vector<sDmLevelRWRequest> wrq;  // write requests of component instances to levels

    void _addLevel();  // used internally...

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override {}
    virtual int myRegisterInstance(int *runMe=NULL) override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override { return TICK_INACTIVE; }
    
  public:
    SMILECOMPONENT_STATIC_DECL

    cDataMemory() : cSmileComponent("dataMemory"), level(NULL), 
      nLevelsAlloc(0), nLevels(-1) {}

    cDataMemory(const char *_name) : cSmileComponent(_name), level(NULL),
      nLevelsAlloc(0), nLevels(-1) {}

    /* register a read request (during "register" phase) */
    void registerReadRequest(const char *lvl, const char *componentInstName=NULL);
    void registerWriteRequest(const char *lvl, const char *componentInstName=NULL);

    /* register a new level, and check for uniqueness of name */
    int registerLevel(cDataMemoryLevel *l);

    /* add a new level with given _lcfg parameters */
    int addLevel(sDmLevelConfig *_lcfg, const char *_name=NULL);

    /* set end-of-input flag */
    virtual void setEOI() override {
      // set end-of-input for all levels
      for (int i=0; i<=nLevels; i++) level[i]->setEOI();
      // set end-of-input in this component
      cSmileComponent::setEOI();
    }

    virtual void unsetEOI() override {
      // set end-of-input for all levels
      for (int i=0; i<=nLevels; i++) level[i]->unsetEOI();
      // set end-of-input in this component
      cSmileComponent::unsetEOI();
    }

    virtual int setEOIcounter(int cnt) override {
      // set end-of-input for all levels
      for (int i=0; i<=nLevels; i++) level[i]->setEOIcounter(cnt);
      // set end-of-input in this component
      return cSmileComponent::setEOIcounter(cnt);
    }

    // get index of level 'name'
    int findLevel(const char *name) const;

    // get number of levels
    int getNlevels() const { return nLevels + 1; }

    /**** functions which will be forwarded to the corresponding level *****/

    /* check if level is ready for reading, get level config (WITHOUT NAMES!) and update blocksizeReader */
    const sDmLevelConfig * queryReadConfig(int llevel, long blocksizeReader = 1)
    {
      if ((llevel >= 0) && (llevel <= nLevels))
        return level[llevel]->queryReadConfig(blocksizeReader);
      else
        return NULL;
    }

    const sDmLevelConfig * queryReadConfig(int llevel, double blocksizeReaderSeconds = 1.0)
    {
      if ((llevel >= 0) && (llevel <= nLevels))
        return level[llevel]->queryReadConfig(blocksizeReaderSeconds);
      else
        return NULL;
    }

    // adds a field to level <level>, _N is the size of an array field, set to 0 or 1 for scalar field
    // the name must be unique per level...
    int addField(int llevel, const char *lname, int lN, int arrNameOffset=0)
    {  
       if ((llevel>=0)&&(llevel<=nLevels)) 
         return level[llevel]->addField(lname, lN, arrNameOffset);
       else 
         return 0; 
    }

    // set info for field i (dataType and custom data pointer)
    int setFieldInfo(int _level, int i, int _dataType, void *_info, long _infoSize) {
      if ((_level>=0)&&(_level<=nLevels))
        return level[_level]->setFieldInfo(i,_dataType,_info,_infoSize);
      else
        return 0;
    }
    
    // set info for field 'name' (dataType and custom data pointer). Do NOT include array indicies in field name!
    int setFieldInfo(int _level, const char *_name, int _dataType, void *_info, long _infoSize) {
      if ((_level>=0)&&(_level<=nLevels))
        return level[_level]->setFieldInfo(_name,_dataType,_info,_infoSize);
      else
        return 0;
    }

    // registers a reader for level _level, returns the reader id assigned to the new reader
    int registerReader(int _level) {
      if ((_level>=0)&&(_level<=nLevels))
        return level[_level]->registerReader();
      else
        return -1;
    }

    int setFrame(int _level, long vIdx, const cVector *vec, int special=-1/*, int *result=NULL*/)
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->setFrame(vIdx,vec,special/*, result*/); else return 0; }
    int setMatrix(int _level, long vIdx, const cMatrix *mat, int special=-1/*, int *result=NULL*/)
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->setMatrix(vIdx,mat,special/*, result*/); else return 0; }

    /* check if a read will succeed */
    int checkRead(int _level, long vIdx, int special=-1, int rdId=-1, long len=1, int *result=NULL)
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->checkRead(vIdx,special,rdId,len,result); else return -1; }

    // the memory pointed to by the return value must be freed via delete() by the calling code!!
    // TODO: optimise this... don't allocate a new frame everytime something is returned!! maybe provide an option
    //       for passing a buffer frame or so...
    cVector * getFrame(int _level, long vIdx, int special=-1, int rdId=-1, int *result=NULL)
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getFrame(vIdx,special,rdId,result); else return NULL; }
    cMatrix * getMatrix(int _level, long vIdx, long vIdxEnd, int special=-1, int rdId=-1, int *result=NULL)
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getMatrix(vIdx,vIdxEnd,special,rdId,result); else return NULL; }

    // set current read index to current write index to prevent hangs, if the readers do not read data sequentially, or if the readers skip data
    void catchupCurR(int _level, int rdId=-1, long _curR=-1 /* if >= 0, value that curR[rdId] will be set to! */ ) 
      { if ((_level>=0)&&(_level<=nLevels)) level[_level]->catchupCurR(rdId,_curR); }

    // methods to get info about current level fill status
    long getCurW(int _level) const // current write index
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getCurW(); else return -1; }
    long getCurR(int _level, int rdId=-1) const  // current read index
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getCurR(rdId); else return -1; }
    long getNFree(int _level,int rdId=-1) const // get number of free frames in current level (rb: nT-(curW-curR))
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getNFree(rdId); else return -1; }
    long getNAvail(int _level,int rdId=-1) const // get number of available frames in current level (curW-curR)
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getNAvail(rdId); else return -1; }
    long getMaxR(int _level) const  // maximum readable index (index where data was written to) or -1 if level is empty
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getMaxR(); else return -1; }
    long getMinR(int _level) const  // minimum readable index or -1 if level is empty (relevant only for ringbuffers, otherwise it will always return -1 or 0)
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getMinR(); else return -1; }
    long getNreaders(int _level) const  // number of registered readers
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getNreaders(); else return -1; }

    int namesAreSet(int _level) const
      { if ((_level>=0)&&(_level<=nLevels)) { return level[_level]->namesAreSet(); } else return 0; }
    void fixateLevel(int _level) 
      { if ((_level>=0)&&(_level<=nLevels)) level[_level]->fixateLevel(); } 

    const sDmLevelConfig * getLevelConfig(int _level) const
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getConfig(); else return NULL; }    
    const char * getLevelName(int _level) const
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getName(); else return NULL; }
    const char * getFieldName(int _level, int n, int *lN=NULL, int *_arrNameOffset=NULL) const
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getFieldName(n, lN, _arrNameOffset); else return NULL; }      
    char * getElementName(int _level, int n) const // caller must free() returned string!!
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getElementName(n); else return NULL; }
    cVectorMeta * getLevelMetaDataPtr(int _level)
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->getLevelMetaDataPtr(); else return NULL; }

 	  void setArrNameOffset(int _level, int arrNameOffset)
	    { if ((_level>=0)&&(_level<=nLevels)) level[_level]->setArrNameOffset(arrNameOffset); }
    void setFrameSizeSec(int _level, double fss)
      { if ((_level>=0)&&(_level<=nLevels)) level[_level]->setFrameSizeSec(fss);  }
    void setBlocksizeWriter(int _level, long _bsw)
      { if ((_level>=0)&&(_level<=nLevels)) level[_level]->setBlocksizeWriter(_bsw);  }

    /* print an overview over registered levels and their configuration */
    void printDmLevelStats(int detail=1)
      { for (int i=0; i<=nLevels; i++) level[i]->printLevelStats(detail); }

    long secToVidx(int _level, double sec) const // returns a vIdx
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->secToVidx(sec); else return -1; }
    double vIdxToSec(int _level, long vIdx) const // returns a time in seconds as double
      { if ((_level>=0)&&(_level<=nLevels)) return level[_level]->vIdxToSec(vIdx); else return -1; }
    
    virtual ~cDataMemory();
};


/****************** data memory logger functions ***************************/

/*
These functions can be used for debugging the data flow. they log every read, write, and check operation, thus producing a lot of data.

Log message fields:
-level
(-component?)
-read/write/check
-write/read/check counter
-vIdx
(-rIdx?)
-length
-curR
-curW
-nT
-DATA
*/

void datamemoryLogger(const char *name, const char*dir, long cnt, long vIdx0, long vIdx, long rIdx, long nT, int special, long curR, long curW, long EOI, int nReaders, long *curRr, cVector *vec);
void datamemoryLogger(const char *name, long vIdx0, long vIdx, long rIdx, long nT, int special, long curR, long curW, long EOI, int nReaders, long *curRr, cMatrix *mat);

#endif // __DATA_MEMORY_HPP
