/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#ifndef __DATA_MEMORY_LEVEL_HPP
#define __DATA_MEMORY_LEVEL_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <core/smileThread.hpp>
#include <math.h>
#include <initializer_list>
#include <memory>
#include <string>

// temporal frame ID
#define DMEM_IDX_ABS    -1    // no special index
#define DMEM_IDX_CURR   -11   // current read index
#define DMEM_IDX_CURW   -12   // current write index
#define DMEM_IDXR_CURR  -13   // start is relative to current read index
#define DMEM_IDXR_CURW  -14   // start is relative to current write index
#define DMEM_PAD_ZERO   -101  // pad with zeros
#define DMEM_PAD_FIRST  -102  // pad with first/last available frame (default)
#define DMEM_PAD_NONE   -103  // truncate frame (matrix) to actual available length

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

/**** field meta information *********
   meta information for one field
 **************************************/

class FieldMetaInfo { public:
  char *name;         // name of field
  int Nstart;         // start index (position in FrameMetaInfo::field)
  int N;              // number of elements (>1 ==> array)
  int dataType;       // dataType flag indicating type of data in this field (see dataType definitions in dataMemory.hpp)
  long infoSize;      // size of info struct in BYTES!
  int infoSet;        // flag that indicates whether the info has been set or not (dataType, *info, infoSize)
  void *info;         // custom info data (e.g. spectral scaling for spectra) 
  int arrNameOffset;

  FieldMetaInfo();
  void copyFrom(FieldMetaInfo *f);
  ~FieldMetaInfo();
};

class cVectorMeta { public:
  int ID;               // ID identifying the meta data type
  int iData[8];         // freely usable ints
  FLOAT_DMEM fData[8];  // freely usable floats
  char * text;          // custom meta-data text, will be auto-freed using free() if not NULL
  void * custom;        // custom meta data, will be auto-freed using free() if not NULL
  long customLength;    // length of data in custom (in bytes!), this is required for cloning a cVectorMeta data object! Set this to <= 0 to only copy the pointer and not free the pointer when the cVectorMeta object is freed!
  
  cVectorMeta();
  cVectorMeta(const cVectorMeta &v);
  cVectorMeta(cVectorMeta &&v);
  cVectorMeta &operator=(const cVectorMeta &v);
  cVectorMeta &operator=(cVectorMeta &&v);
  ~cVectorMeta();
};

/**** frame meta information *********
   names, indicies (for each row)
 **************************************/

// meta information for a complete vector 
class FrameMetaInfo {
  public:
    long N, Ne;                // number of fields..
    FieldMetaInfo *field;      // field metadata for each of the fields
    cVectorMeta metaData;      // global metadata for the whole level (use tmeta->metadata for per frame meta data)

    FrameMetaInfo();
    
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

    void printFieldNames() const;

    // n is element(!) index
    const char *getName(int n, int *arrIdx=NULL) const;

    ~FrameMetaInfo();
};

/**** temporal meta information *********
   length, times, etc. (for each column)
 **************************************/

class TimeMetaInfo { public:
  int filled;                             // whether info in this struct was already completed by setTimeMeta or the calling code
  long vIdx;                              // index of this frame in data memory level ?
  double period;                          // frame period of this level
  double time;                            // real start time in seconds
  double lengthSec;                       // real length in seconds
  double framePeriod;                     // frame period of previous level
  double smileTime;                       // smile time at which the frame (or a parent) was initially written to the data memory by a source (time measured in seconds since creation of component manager object)
  std::unique_ptr<cVectorMeta> metadata;  // optional custom meta-data for each vector/matrix, may be NULL if not present

  TimeMetaInfo();
  TimeMetaInfo(const TimeMetaInfo &tm);
  TimeMetaInfo(TimeMetaInfo &&tm);
  TimeMetaInfo &operator=(const TimeMetaInfo &tm);
  TimeMetaInfo &operator=(TimeMetaInfo &&tm);
};

/**** dataMemory datatypes *********
 **************************************/

class cVector {
  protected:
    int tmetaAlien;                      // indicates that tmeta is a pointer to a struct allocated elsewhere
  
  public:
    long N;                              // number of elements
    TimeMetaInfo *tmeta;                 // usually tmeta is allocated individually for each vector, except if tmetaAlien is set
    FrameMetaInfo *fmeta;                // fmeta is always a pointer to memory in the dataMemory
    FLOAT_DMEM *data;                    // contains vector data
    
    cVector(int lN, bool noTimeMeta=false);
    cVector(std::initializer_list<FLOAT_DMEM> elem);

    // get name of expanded element n
    std::string name(int n, int *lN=NULL) const;

    FLOAT_DMEM get(int n) const { return data[n]; }  // WARNING: index n is not checked!
    void set(int n, FLOAT_DMEM v) { data[n]=v; }     // WARNING: index n is not checked!

    void setTimeMeta(TimeMetaInfo *xtmeta);
    void copyTimeMeta(const TimeMetaInfo *xtmeta);

    virtual ~cVector();
};


// memory organisation of cMatrix:
// array index x = col*N + row   ( t*N + n )
class cMatrix : public cVector { public:
  long nT;

  cMatrix(int lN, int lnT, bool noTimeMeta=false);  
  cMatrix(std::initializer_list<std::initializer_list<FLOAT_DMEM>> elem);

  FLOAT_DMEM get(int n, int t) const { return data[n+t*N]; }  // WARNING: index n is not checked!
  void set(int n, int t, FLOAT_DMEM v) { data[n+t*N]=v; }     // WARNING: index n is not checked!

  cMatrix * getRow(long R) const;
  cMatrix * getRow(long R, cMatrix *r) const;
  cVector* getCol(long C) const;
  cVector* getCol(long C, cVector *c) const;
  void setRow(long R, const cMatrix *row);
  
  int resize(long _new_nT);

private:
  // helper method to copy blocks of data 
  void copyData(void *dest, long destIdx, void *src, long srcIdx, long nT, long N, bool zeroSource);

public:
  // copy part of the data in this matrix to a buffer
  void copyTo(void *dest, long destIdx, long srcIdx, long nT, bool zeroSource = false);
  // copy part of the data in this matrix to another matrix
  void copyTo(cMatrix &dest, long destIdx, long srcIdx, long nT, bool zeroSource = false);
  // copy part of the data in another matrix to this matrix
  void copyFrom(cMatrix &src, long srcIdx, long destIdx, long nT, bool zeroSource = false);
  // copy part of the data in a buffer to this matrix
  void copyFrom(void *src, long srcIdx, long destIdx, long nT, bool zeroSource = false);

  void setTimeMeta(TimeMetaInfo *_tmeta);
  void copyTimeMeta(const TimeMetaInfo *_tmeta, long _nT=-1);
  // convert a matrix tmeta to a vector tmeta spanning the whole matrix by adjusting size, period, etc.
  void squashTimeMeta(double period=-1.0);
  
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
class sDmLevelConfig { public:
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

  sDmLevelConfig(double _T, double _frameSizeSec, long _nT=10, int _isRb=1);
  sDmLevelConfig(double _T, double _frameSizeSec, double _lenSec=1.0, int _isRb=1);
  sDmLevelConfig(const char *_name, double _T, double _frameSizeSec, long _nT=10, int _isRb=1);
  sDmLevelConfig(const char *_name, double _T, double _frameSizeSec, double _lenSec=1.0, int _isRb=1);
  sDmLevelConfig();
  sDmLevelConfig(sDmLevelConfig const &orig);
  sDmLevelConfig(const char *_name, sDmLevelConfig &orig);

  void updateFrom(const sDmLevelConfig &orig);

  void setName(const char *_name);

  ~sDmLevelConfig();
};

class cDataMemory;

class cDataMemoryLevel {
  private:
    //char *name;

    int myId;
    cDataMemory * _parent;

    /* variables for mutex access and locking */
    mutable smileMutex RWptrMtx;  // mutex to lock global curW / curR pointers
    mutable smileMutex RWmtx;     // mutex to lock data area during read / write
    mutable smileMutex RWstatMtx; // mutex to lock nCurRdr and writeReqFlag variables for mut.ex. write/read op. while allowing multiple parallel reads
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

    // write frame data from level's data matrix at pos rIdx to *_data
    void frameRd(long rIdx, FLOAT_DMEM *_data) const;

    void setTimeMeta(long rIdx, long vIdx, const TimeMetaInfo *tm);
    void getTimeMeta(long rIdx, long vIdx, TimeMetaInfo *tm) const;

  public:

    // create level from given level configuration struct, the name in &cfg will be overwritten via _name parameter
    cDataMemoryLevel(int _levelId, sDmLevelConfig &cfg, const char *_name = NULL);

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
    int setFieldInfo(const char *__name, int _dataType, void *_info, long _infoSize);

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
    int findField(const char*_fieldName, int *arrIdx) const;

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
    int checkRead(long vIdx, int special=-1, int rdId=-1, long len=1, int *result=NULL);

    /* set read index of reader rdId (or global read index, if rdId=-1) 
       to user-defined _curR or to write index curW, if _curR=-1 or omitted.
       if _curR=-1 or omitted, thus, the level buffer will be cleared (not physically but logically)
       use this, if your reader only requires data "part time" (e.g. reading only voiced segments of speech, etc.)
      */
    void catchupCurR(int rdId=-1, int _curR = -1);

    /* methods to get info about current level fill status 
       (e.g. number of frames written, curW, curR(global) and freeSpace, etc.) */

    /* get current write index (index that will be written to NEXT) */
    long getCurW() const;

    /* current read index (index that will be read NEXT) of reader rdId or global (min of all readers) if rdId is omitted */
    long getCurR(int rdId=-1) const; 

    /* get number of free frames in current level, i.e. the number of frames that can 
       currently be written without having to resize the buffer (for ringbuffers: nT-(curW-curR)) */
    long getNFree(int rdId=-1) const;
    
    /* get number of available frames for reader rdId */
    long getNAvail(int rdId=-1) const;

    /* get maximum readable index ("last" index where data was written to) */
    long getMaxR() const;

    /* get minimum readable index (relevant only for ringbuffers, for regular buffers this function will always return 0) */
    long getMinR() const;  

    /* get level configuration */
    const sDmLevelConfig * getConfig() const { return &lcfg; }

    /* get number of registered readers (if the level is not finalised, it is the number of readers *currently* registered!) */
    long getNreaders() const { return nReaders; }

    /* check if level is ready for reading, get level config (WITHOUT NAMES!) and update blocksizeReader */
    const sDmLevelConfig * queryReadConfig(long blocksizeReader);

    /* check if level is ready for reading, get level config (WITHOUT NAMES!) and update blocksizeReader */
    const sDmLevelConfig * queryReadConfig(double blocksizeReaderSeconds);

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
    void setFrameSizeSec(double fss);
    void setBlocksizeWriter(long _bsw, int _force=0);

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

    ~cDataMemoryLevel();
};

#endif // __DATA_MEMORY_LEVEL_HPP
