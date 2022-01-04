/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#include <core/dataMemoryLevel.hpp>
#include <core/dataMemory.hpp>
#include <core/componentManager.hpp>
#include <algorithm>
#include <limits>
#include <new>
#include <utility>

#define MODULE "dataMemoryLevel"

/**** field meta information *********
   meta information for one field
 **************************************/

FieldMetaInfo::FieldMetaInfo() : name(NULL), info(NULL), infoSize(0), dataType(DATATYPE_UNKNOWN), N(0), arrNameOffset(0) {}

void FieldMetaInfo::copyFrom(FieldMetaInfo *f) {
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

FieldMetaInfo::~FieldMetaInfo() { 
  if (name!=NULL) free(name); 
  if (info!=NULL) free(info);
}

cVectorMeta::cVectorMeta() : ID(0), text(NULL), custom(NULL), customLength(0) {
  std::fill(iData, iData + 8, 0);
  std::fill(fData, fData + 8, 0.0);
}

cVectorMeta::cVectorMeta(const cVectorMeta &v) {
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

cVectorMeta::cVectorMeta(cVectorMeta &&v) {
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

cVectorMeta &cVectorMeta::operator=(const cVectorMeta &v) {
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

cVectorMeta &cVectorMeta::operator=(cVectorMeta &&v) {
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

cVectorMeta::~cVectorMeta() { 
  if (text != NULL) free(text); 
  // if customLength <= 0, we are not supposed to free custom
  if ((customLength>0)&&(custom != NULL)) free(custom); 
}

/**** frame meta information *********
   names, indicies (for each row)
 **************************************/

FrameMetaInfo::FrameMetaInfo() : N(0), Ne(0), field(NULL) {}

// the returned name is the base name belonging to the expanded field n
// arrIdx will be the index in the array (if an array)
//   or -1 if no array!
//-> n is element(!) index
const char * FrameMetaInfo::getName(int n, int *_arrIdx) const
{
  long lN=Ne; 

  if ((n>=0)&&(n<lN)) {
    long f=0,tmp=0;

    for(; f<N; f++) {
      tmp += field[f].N;
      if (tmp>n) break;
    }
    if (_arrIdx != NULL) {
      if (field[f].N > 1)
        *_arrIdx = n-(tmp-field[f].N) + field[f].arrNameOffset;
      else
        *_arrIdx = -1;
    }

    return field[f].name;
  }

  return NULL;
}

long FrameMetaInfo::fieldToElementIdx(long _field, long _arrIdx) const
{
  int i;
  int lN=0;
  if (_field >= N) _field = lN-1;
  for(i=0; i<_field; i++) {
    lN += field[i].N;
  }
  return lN + _arrIdx;
}

long FrameMetaInfo::elementToFieldIdx(long _element, long *_arrIdx) const
{
  int i;
  int lN=0, _prevN = 0;
  for(i=0; i<N; i++) {
    lN += field[i].N;
    if (lN > _element) {
      if (_arrIdx != NULL) *_arrIdx = field[i].N - (lN - _element);
      return i;
    }
  }
  if (_arrIdx != NULL) *_arrIdx = 0;
  return -1;
}

void FrameMetaInfo::printFieldNames() const {
  SMILE_PRINT("  Field name & dimension:");
  for (int i = 0; i < N; i++) {
    SMILE_PRINT("    %s %i", field[i].name, field[i].N);
  }
}

// returned *arrIdx will be the real index in the data structure, i.e. named index - arrNameOffset
int FrameMetaInfo::findField(const char*fieldName, int *arrIdx, int *more) const
{
  if (fieldName == NULL)
    return -1;
  // check for [] in _fieldName => array field
  long ai = 0;
  char *fn = strdup(fieldName);
  char *a  = strchr(fn, '[');
  if (a != NULL) {  // yes, array
    a[0] = 0;
    a++;
    char *b = strchr(a, ']');
    if (b == NULL) {
      COMP_ERR("findField: invalid array field name "
        "'%s', expected ']' at the end!", fieldName);
    }
    b[0] = 0;
    char *eptr = NULL;
    ai = strtol(a, &eptr, 10);
    if ((ai == 0) && (eptr == a)) {
      COMP_ERR("findField: error parsing array index in "
          "name '%s', index is not a number!",fieldName);
    }
  }
  if (arrIdx != NULL)
    *arrIdx = ai;

  int i;
  int idx = -1;
  int start = 0;
  if ((more != NULL) && (*more>0)) {
    start = *more;
    *more = 0;
  }
  int foundFlag = 0;
  for (i=start; i<N; i++) {
    int myAi;
    if (!foundFlag) {
      if (a != NULL) // array name
        myAi = ai - field[i].arrNameOffset;
      else // non array name (first field of array, always index 0 in data structure)
        myAi = 0;
    }
    if (!strcmp(fn,field[i].name)) {
      if (!foundFlag) {
        idx = i;
        if (myAi < field[i].N) {
          //idx = field[i].Nstart + myAi;
          if (arrIdx != NULL) *arrIdx = myAi; // update *arrIdx with correct index (real index in data structure, not 'named' index)
          if (more == NULL) break;
          else foundFlag=1;
        } else {
          COMP_ERR("array index out of bounds (field '%s') %i > %i (must from %i - %i) (NOTE: first index is 0, not 1!)",fieldName,ai,field[i].N-1+field[N].arrNameOffset,field[N].arrNameOffset,field[i].N-1+field[N].arrNameOffset);
        }
      } else {
        if (more != NULL) (*more)++;
      }
    }
  }
  free(fn);
  return idx;
}

// returned *arrIdx will be the real index in the data structure, i.e. named index - arrNameOffset
int FrameMetaInfo::findFieldByPartialName(const char*_fieldNamePart, int *arrIdx, int *more) const
{
  if (_fieldNamePart == NULL) return -1;

  // check for [] in _fieldName => array field
  long ai = 0;
  char *fn = strdup(_fieldNamePart);
  char *a  = strchr(fn,'[');
  if (a != NULL) {  // yes, array
    a[0] = 0;
    a++;
    char *b = strchr(fn,']');
    if (b == NULL) { COMP_ERR("findField: invalid array field name part '%s', expected ']' at the end!",_fieldNamePart); }
    b[0] = 0;
    char *eptr = NULL;
    ai = strtol(a,&eptr,10);
    if ((ai==0)&&(eptr == a)) { COMP_ERR("findField: error parsing array index in name part '%s', index is not a number!",_fieldNamePart); }
  }
  if (arrIdx != NULL) *arrIdx = ai;

  int i;
  int idx = -1;
  int _start = 0;
  if ((more != NULL)&&(*more>0)) { _start = *more; *more = 0; }
  int foundFlag = 0;
  for (i=_start; i<N; i++) {
    int myAi;
    if (!foundFlag) {
      if (a != NULL) // array name
        myAi = ai - field[i].arrNameOffset;
      else // non array name (first field of array, always index 0 in data structure)
        myAi = 0;
    }
    if (strstr(field[i].name,fn)) {
      if (!foundFlag) {
        idx = i;
        if (myAi < field[i].N) {
          if (arrIdx != NULL) 
            *arrIdx = myAi; // update *arrIdx with correct index (real index in data structure, not 'named' index)
          if (more == NULL) 
            break;
          else foundFlag=1;
        } else {
          COMP_ERR("array index out of bounds (partial field name '%s') %i > %i (must from %i - %i) (NOTE: first index is 0, not 1!)",_fieldNamePart,ai,field[i].N-1+field[N].arrNameOffset,field[N].arrNameOffset,field[i].N-1+field[N].arrNameOffset);
        }
      } else {
        if (more != NULL) 
          *more = *more + 1;
      }
    }
  }
  free(fn);
  return idx;
}

FrameMetaInfo::~FrameMetaInfo() {
  if (field != NULL) {
    for (int i=0; i<N; i++) { 
      if (field[i].name != NULL) free(field[i].name); 
      if (field[i].info!=NULL) free(field[i].info);
    }
    free(field);
  }
}

/**** time meta information *********
 **************************************/

TimeMetaInfo::TimeMetaInfo() :
  filled(0), vIdx(0), period(0.0), time(0.0),
  lengthSec(0.0), framePeriod(0.0), smileTime(-1.0)
{}

TimeMetaInfo::TimeMetaInfo(const TimeMetaInfo &tm) :
  filled(tm.filled), vIdx(tm.vIdx), period(tm.period), time(tm.time),
  lengthSec(tm.lengthSec), framePeriod(tm.framePeriod), smileTime(tm.smileTime) {
  if (tm.metadata != NULL) {
    metadata = std::unique_ptr<cVectorMeta>(new cVectorMeta(*tm.metadata));
  }
}

TimeMetaInfo::TimeMetaInfo(TimeMetaInfo &&tm) : 
  filled(tm.filled), vIdx(tm.vIdx), period(tm.period), time(tm.time),
  lengthSec(tm.lengthSec), framePeriod(tm.framePeriod), smileTime(tm.smileTime),
  metadata(std::move(tm.metadata))
{}

TimeMetaInfo &TimeMetaInfo::operator=(const TimeMetaInfo &tm) {
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

TimeMetaInfo &TimeMetaInfo::operator=(TimeMetaInfo &&tm) {
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

/******* datatypes ************/

cVector::cVector(int lN, bool noTimeMeta) :
  N(0), data(NULL), fmeta(NULL), tmeta(NULL), tmetaAlien(0)
{
  if (lN>0) {
    data = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*lN);
    if (data==NULL) OUT_OF_MEMORY;
    N = lN;
    if (!noTimeMeta) {
      tmeta = new TimeMetaInfo[1]();
      if (tmeta == NULL) OUT_OF_MEMORY;
    }
  }
}

cVector::cVector(std::initializer_list<FLOAT_DMEM> elem) :
  cVector(elem.size())
{
  std::copy(elem.begin(), elem.end(), data);
}

std::string cVector::name(int n, int *lN) const
{
  if ((fmeta!=NULL)&&(fmeta->field!=NULL)) {
    int llN=-1;
    const char *t = fmeta->getName(n,&llN);
    if (lN!=NULL) *lN = llN;
    if (llN>=0) {
      char *ntmp=myvprint("%s[%i]",t,llN);
      std::string n = ntmp;
      free(ntmp);
      return n;
    } else {
      return t;
    }
  }
  return std::string();
}

void cVector::setTimeMeta(TimeMetaInfo *xtmeta) {
  if ((tmeta != NULL)&&(!tmetaAlien)) {
    delete[] tmeta;
  }
  tmetaAlien = 1;
  tmeta = xtmeta;
}

void cVector::copyTimeMeta(const TimeMetaInfo *xtmeta) {
  if ((tmeta != NULL)&&(!tmetaAlien)) {
    delete[] tmeta;
  }
  if (xtmeta != NULL) {
    tmeta = new TimeMetaInfo[1] { *xtmeta };
  } else {
    tmeta = NULL;
  }
  tmetaAlien = 0;
}

cVector::~cVector() {
  if (data!=NULL) free(data);
  if ((tmeta!=NULL)&&(!tmetaAlien)) {
    delete[] tmeta;
  }
}

cMatrix::~cMatrix()
{
  if ((tmeta!=NULL)&&(!tmetaAlien)) {
    delete[] tmeta;
  }
  tmeta = NULL;
}

cMatrix::cMatrix(int lN, int lnT, bool noTimeMeta) :
  cVector(0), nT(0)
{
  if ((lN>0)&&(lnT>0)) {
    data = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*lN*lnT);
    if (data==NULL) OUT_OF_MEMORY;
    N = lN;
    nT = lnT;
    if (!noTimeMeta) {
      tmeta = new TimeMetaInfo[lnT];
      if (tmeta == NULL) OUT_OF_MEMORY;
    }
  }
}

cMatrix::cMatrix(std::initializer_list<std::initializer_list<FLOAT_DMEM>> elem) :
  cMatrix(elem.size(), elem.size() > 0 ? elem.begin()->size() : 0)
{
  int r = 0;
  for (const std::initializer_list<FLOAT_DMEM> &row : elem) {
    int c = 0;
    if (row.size() != nT) {
      COMP_ERR("cMatrix: initializer list contains rows of non-uniform length");
    }
    for (FLOAT_DMEM v : row) {
      set(r, c, v);
      c++;
    }
    r++;
  }
}

cMatrix * cMatrix::getRow(long R) const {
  bool noTimeMeta = tmeta == NULL;
  cMatrix *r = new cMatrix(1,nT,noTimeMeta);
  cMatrix *ret = getRow(R,r);
  if (ret==NULL) delete r;
  return ret;
}

cMatrix * cMatrix::getRow(long R, cMatrix *r) const {
  long i;
  bool noTimeMeta = tmeta == NULL;
  if (r==NULL) r = new cMatrix(1,nT,noTimeMeta);
  else {
    if (r->nT != nT) { delete r; r = new cMatrix(1,nT,noTimeMeta); }
  }
  long nn = MIN(nT,r->nT);
  FLOAT_DMEM *df = data+R;
  for (i=0; i<nn; i++) { r->data[i] = *df; df += N; /*data[i*N+R];*/ }
  for ( ; i<r->nT; i++) { r->data[i] = 0.0; }
  r->setTimeMeta(tmeta);
  return r;
}

cVector* cMatrix::getCol(long C) const {
  bool noTimeMeta = tmeta == NULL;
  cVector *c = new cVector(N,noTimeMeta);
  long i;
  for (i=0; i<N; i++) { c->data[i] = data[N*C+i]; }
  c->setTimeMeta(tmeta);
  return c;
}

cVector* cMatrix::getCol(long C, cVector *c) const {
  bool noTimeMeta = tmeta == NULL;
  if (c==NULL) c = new cVector(N,noTimeMeta);
  long i;
  for (i=0; i<N; i++) { c->data[i] = data[N*C+i]; }
  c->setTimeMeta(tmeta);
  return c;
}

void cMatrix::setRow(long R, const cMatrix *row) { // NOTE: set row does not change tmeta!!
  long i;
  if (row!=NULL) {
    long nn = MIN(nT,row->nT);
    for (i=0; i<nn; i++) { data[i*N+R] = row->data[i]; }
  }
}

int cMatrix::resize(long _new_nT) {
  int ret=1;
  if (_new_nT < nT) return 1;

  FLOAT_DMEM *tmp = (FLOAT_DMEM *)crealloc(data, _new_nT*sizeof(FLOAT_DMEM)*N, nT*sizeof(FLOAT_DMEM)*N);
  if (tmp==NULL) ret = 0;
  else data = tmp;

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

void cMatrix::copyData(void *dest, long destIdx, void *src, long srcIdx, long nT, long N, bool zeroSource) {
  memcpy(&((FLOAT_DMEM *)dest)[destIdx * N], &((FLOAT_DMEM *)src)[srcIdx * N], sizeof(FLOAT_DMEM) * N * nT);
  if (zeroSource) {
    memset(&((FLOAT_DMEM *)src)[srcIdx * N], 0, sizeof(FLOAT_DMEM) * N * nT);
  }
}

void cMatrix::copyTo(void *dest, long destIdx, long srcIdx, long nT, bool zeroSource) {
  copyData(dest, destIdx, data, srcIdx, nT, N, zeroSource);
}

void cMatrix::copyTo(cMatrix &dest, long destIdx, long srcIdx, long nT, bool zeroSource) {
  copyData(dest.data, destIdx, data, srcIdx, nT, N, zeroSource);
}

void cMatrix::copyFrom(cMatrix &src, long srcIdx, long destIdx, long nT, bool zeroSource) {
  copyData(data, destIdx, src.data, srcIdx, nT, N, zeroSource);
}

void cMatrix::copyFrom(void *src, long srcIdx, long destIdx, long nT, bool zeroSource) {
  copyData(data, destIdx, src, srcIdx, nT, N, zeroSource);
}

void cMatrix::setTimeMeta(TimeMetaInfo *_tmeta) {
  if ((tmeta != NULL)&&(!tmetaAlien)) {
    delete[] tmeta;
    tmetaAlien = 1;
  }
  tmeta = _tmeta;
}

void cMatrix::copyTimeMeta(const TimeMetaInfo *_tmeta, long _nT) {
  if (_nT == -1) _nT = nT;
  if ((tmeta != NULL)&&(!tmetaAlien)) {
    delete[] tmeta;
  }
  if (_tmeta != NULL) {
    tmeta = new TimeMetaInfo[nT];
    std::copy(_tmeta, _tmeta + (std::min)(nT, _nT), tmeta);
  } else {
    tmeta = NULL;
  }
  tmetaAlien = 0;
}

// convert a matrix tmeta to a vector tmeta spanning the whole matrix by adjusting size, period, etc.
void cMatrix::squashTimeMeta(double period) {
  if (tmeta != NULL) {
    tmeta[0].framePeriod = tmeta[0].period;
    if (period != -1.0)
      tmeta[0].period = period;
    //else   // TODO::: there is bug here......
    //  tmeta[0].period = tmeta[nT-1].time - tmeta[0].time + tmeta[nT-1].lengthSec;
    tmeta[0].lengthSec = tmeta[nT-1].time - tmeta[0].time + tmeta[nT-1].lengthSec;
  }
}

/******* level configuration ************/

sDmLevelConfig::sDmLevelConfig(double _T, double _frameSizeSec, long _nT, int _isRb) :
  T(_T), frameSizeSec(_frameSizeSec), lastFrameSizeSec(_frameSizeSec), nT(_nT), lenSec(0.0), basePeriod(0.0),
  blocksizeWriter(1), blocksizeReader(1), minBlocksizeReader(-1),
  isRb(_isRb), noHang(1), growDyn(0),
  finalised(0), blocksizeIsSet(0), namesAreSet(0),
  N(0), Nf(0), 
  fmeta(NULL),
  name(NULL),
  noTimeMeta(false)
{ 
  lenSec = T*(double)nT; 
}

sDmLevelConfig::sDmLevelConfig(double _T, double _frameSizeSec, double _lenSec, int _isRb) :
  T(_T), frameSizeSec(_frameSizeSec), lastFrameSizeSec(_frameSizeSec), nT(0), lenSec(_lenSec), basePeriod(0.0),
  blocksizeWriter(1), blocksizeReader(1), minBlocksizeReader(-1),
  isRb(_isRb), noHang(1), growDyn(0),
  finalised(0), blocksizeIsSet(0), namesAreSet(0),
  N(0), Nf(0), 
  fmeta(NULL),
  name(NULL),
  noTimeMeta(false)
{ 
  if (T!=0.0) nT = (long)ceil(lenSec/T); 
}

sDmLevelConfig::sDmLevelConfig(const char *_name, double _T, double _frameSizeSec, long _nT, int _isRb) :
  T(_T), frameSizeSec(_frameSizeSec), lastFrameSizeSec(_frameSizeSec), nT(_nT), lenSec(0.0), basePeriod(0.0),
  blocksizeWriter(1), blocksizeReader(1), minBlocksizeReader(-1),
  isRb(_isRb), noHang(1), growDyn(0),
  finalised(0), blocksizeIsSet(0), namesAreSet(0),
  N(0), Nf(0), 
  fmeta(NULL),
  name(NULL),
  noTimeMeta(false)
{ 
  lenSec = T*(double)nT; 
  if (_name != NULL) name = strdup(_name);
}

sDmLevelConfig::sDmLevelConfig(const char *_name, double _T, double _frameSizeSec, double _lenSec, int _isRb) :
  T(_T), frameSizeSec(_frameSizeSec), lastFrameSizeSec(_frameSizeSec), nT(0), lenSec(_lenSec), basePeriod(0.0),
  blocksizeWriter(1), blocksizeReader(1), minBlocksizeReader(-1),
  isRb(_isRb), noHang(1), growDyn(0),
  finalised(0), blocksizeIsSet(0), namesAreSet(0),
  N(0), Nf(0), 
  fmeta(NULL),
  name(NULL),
  noTimeMeta(false)
{ 
  if (T!=0.0) nT = (long)ceil(lenSec/T); 
  if (_name != NULL) name = strdup(_name);  
}

sDmLevelConfig::sDmLevelConfig() :
  T(0.0), frameSizeSec(0.0), lastFrameSizeSec(0.0), nT(0), lenSec(0.0), basePeriod(0.0),
  blocksizeWriter(1), blocksizeReader(1), minBlocksizeReader(-1),
  isRb(1), noHang(1), growDyn(0),
  finalised(0), blocksizeIsSet(0), namesAreSet(0),
  N(0), Nf(0), 
  fmeta(NULL),
  name(NULL),
  noTimeMeta(false) {}

sDmLevelConfig::sDmLevelConfig(sDmLevelConfig const &orig) :
  T(orig.T), frameSizeSec(orig.frameSizeSec), lastFrameSizeSec(orig.lastFrameSizeSec), nT(orig.nT), lenSec(orig.lenSec), basePeriod(orig.basePeriod),
  blocksizeWriter(orig.blocksizeWriter), blocksizeReader(orig.blocksizeReader), minBlocksizeReader(orig.minBlocksizeReader),
  isRb(orig.isRb), noHang(orig.noHang), growDyn(orig.growDyn),
  finalised(orig.finalised), blocksizeIsSet(orig.blocksizeIsSet), namesAreSet(orig.namesAreSet),
  N(orig.N), Nf(orig.Nf), 
  fmeta(orig.fmeta),
  name(NULL),
  noTimeMeta(orig.noTimeMeta) { 
    if (orig.name != NULL) name = strdup(orig.name); 
  }


sDmLevelConfig::sDmLevelConfig(const char *_name, sDmLevelConfig &orig) :
  T(orig.T), frameSizeSec(orig.frameSizeSec), lastFrameSizeSec(orig.lastFrameSizeSec), nT(orig.nT), lenSec(orig.lenSec), basePeriod(orig.basePeriod),
  blocksizeWriter(orig.blocksizeWriter), blocksizeReader(orig.blocksizeReader), minBlocksizeReader(orig.minBlocksizeReader),
  isRb(orig.isRb), noHang(orig.noHang), growDyn(orig.growDyn),
  finalised(orig.finalised), blocksizeIsSet(orig.blocksizeIsSet), namesAreSet(orig.namesAreSet),
  N(orig.N), Nf(orig.Nf), 
  fmeta(orig.fmeta),
  name(NULL),
  noTimeMeta(orig.noTimeMeta) { 
    if (_name != NULL) name = strdup(_name); 
    else if (orig.name != NULL) name = strdup(orig.name); 
  }

void sDmLevelConfig::updateFrom(const sDmLevelConfig &orig) 
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

void sDmLevelConfig::setName(const char *_name) {
  if (_name != NULL) {
    if (name != NULL)
      free(name);
    name = strdup(_name);
  }
}

sDmLevelConfig::~sDmLevelConfig() {
  if (name != NULL)
    free(name);
}


/******* dataMemory level class ************/

cDataMemoryLevel::cDataMemoryLevel(int _levelId, sDmLevelConfig &cfg, const char *_name) :
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

void cDataMemoryLevel::checkCurRr() {
  if (nReaders>0) {
    long newCurR=(std::numeric_limits<long>::max)();
    for (int i=0; i<nReaders; i++) {
      // if any curRr is behind global curR, set it to curR
      if (curRr[i] < curR) {
        SMILE_DBG(3,"(level='%s') auto increasing reader %i 's read index to %i",getName(),i,curR);
        curRr[i] = curR;
      }
      // new global curR is minimum of all curRr
      if (curRr[i] < newCurR) newCurR=curRr[i];
    }
    curR = newCurR;
  }
}

int cDataMemoryLevel::growLevel(long newSize)
{
  SMILE_DBG(3,"increasing buffer size of level '%s' from nT=%i to nT=%i",getName(),lcfg.nT,newSize);
  // resize underlying data buffer
  if (!data->resize(newSize)) {
    SMILE_ERR(1,"could not increase buffer size of level '%s' from nT=%i to nT=%i. Out of memory.",getName(),lcfg.nT,newSize);
    return 0;
  }
  if (lcfg.isRb) {
    // for ring buffers, we need to re-order the data in the buffer: 
    // data at location vIdx % lcfg.nT must be moved to vIdx % newSize in the new buffer
    // we only re-order data from curR to curW and zero-fill the remaining space
    // (the latter might not be strictly necessary but ensures components read consistent data
    // in case they do somehow read data beyond their read pointer)

    // re-order actual data
    if (newSize == lcfg.nT * 2) {
      // optimization for the common case of doubling the level size
      // in this case, we can do the re-ordering in-place
      if (curR % lcfg.nT < curW % lcfg.nT) {
        // data is stored contiguously (i.e. does not wrap around the end of the ring buffer)
        if (curR % lcfg.nT == curR % newSize) {
          // data can stay at its place
        } else {
          // move all data between curR and curW as one contiguous block to the second half of the buffer
          data->copyTo(*data, curR % newSize, curR % lcfg.nT, curW - curR, true);
        }
      } else {
        // data wraps around the end of the ring buffer
        if (curR % lcfg.nT == curR % newSize) {
          // second part of data at beginning of ring buffer needs to be moved to the beginning of second half of the buffer
          data->copyTo(*data, lcfg.nT, 0, curW % lcfg.nT, true);
        } else {
          // first part of data up to end of first half of buffer needs to be moved to the end of the second half of the buffer
          data->copyTo(*data, (curR % lcfg.nT) + lcfg.nT, curR % lcfg.nT, lcfg.nT - (curR % lcfg.nT), true);
        }
      }
    } else {
      // general case (temporarily needs more space but works for any buffer size)
      void *tmp = calloc(sizeof(FLOAT_DMEM), lcfg.nT * data->N);            
      if (tmp == NULL) {
        SMILE_ERR(1,"could not allocate temporary buffer for increasing buffer size of level '%s' from nT=%i to nT=%i. Out of memory.",getName(),lcfg.nT,newSize);
        return 0;
      }
      data->copyTo(tmp, 0, 0, lcfg.nT, true);
      for (long t = curR; t < curW; t++) {
        data->copyFrom(tmp, t % lcfg.nT, t % newSize, 1, false);
      }
      free(tmp);
    }

    if (data->tmeta != NULL) {
      // also re-order data->tmeta in the same way
      TimeMetaInfo *tmetatmp = new TimeMetaInfo[lcfg.nT];
      std::move(data->tmeta, data->tmeta + lcfg.nT, tmetatmp);
      std::fill(data->tmeta, data->tmeta + lcfg.nT, TimeMetaInfo());
      for (long t = curR; t < curW; t++) {
        data->tmeta[t % newSize] = std::move(tmetatmp[t % lcfg.nT]);
      }
      delete[] tmetatmp;

      // resize tmeta
      TimeMetaInfo * tmeta_new = new TimeMetaInfo[newSize]();
      for (long t = curR; t < curW; t++) {
        tmeta_new[t % newSize] = tmeta[t % lcfg.nT];
      }
      delete[] tmeta;
      tmeta = tmeta_new;
    }

    minRAtLastGrowth = curR;
  } else {
    if (data->tmeta != NULL) {
      // for non-ring buffers, no re-ordering of the data is needed
      // resize tmeta
      TimeMetaInfo * tmeta_new = new TimeMetaInfo[newSize]();
      std::copy(tmeta, tmeta + lcfg.nT, tmeta_new);
      delete[] tmeta;
      tmeta = tmeta_new;
    }
  }
  lcfg.lenSec *= (double)newSize / lcfg.nT;
  lcfg.nT = newSize;    
  return 1;  
}

long cDataMemoryLevel::validateIdxW(long *vIdx, int special)
{
  SMILE_DBG(5,"validateIdxW ('%s')\n         vidx=%i special=%i curW=%i curR=%i nT=%i",getName(),*vIdx,special,curW,curR,lcfg.nT);

  if (special == DMEM_IDX_CURW) *vIdx = curW;
  else if (special != -1) return -1;
  if (*vIdx < 0 || *vIdx > curW) return -1;

  if (lcfg.isRb) {
    if (lcfg.nT - (curW-curR) <= 0) {
      if (!lcfg.growDyn) {
        bool nh = (lcfg.noHang == 1 && nReaders == 0) || lcfg.noHang == 2;
        if (!nh) {
          SMILE_DBG(3,"data lost while writing value to ringbuffer level '%s'",getName());
          return -1;
        }
      } else {
        if (!growLevel(lcfg.nT * 2)) return -1;
      }
    }

    if (*vIdx==curW) curW++; 
    if ((lcfg.noHang==2)&&((lcfg.nT - (curW-curR)) <= 0)) {
      SMILE_DBG(3,"data lost while writing value to ringbuffer level '%s'",getName());
      curR = curW-lcfg.nT+1;
    }
    return *vIdx%lcfg.nT;      
  } else { // no ringbuffer
    if (*vIdx >= lcfg.nT) {
      if (!lcfg.growDyn) {
        SMILE_DBG(3,"data lost while writing value to level '%s'",getName());
        return -1;
      } 
      if (!growLevel(lcfg.nT * 2)) return -1;
    }
    if (*vIdx==curW) curW++;
    return *vIdx;
  }
}

long cDataMemoryLevel::validateIdxRangeW(long *vIdx, long vIdxEnd, int special)
{
  SMILE_DBG(5,"validateIdxRangeW ('%s')\n         vidx=%i vidxend=%i special=%i curW=%i curR=%i nT=%i",getName(),*vIdx,vIdxEnd,special,curW,curR,lcfg.nT);

  if (vIdxEnd < *vIdx) { SMILE_ERR(3,"validateIdxRangeW: vIdxEnd (%i) cannot be smaller than vIdx (%i)!",vIdxEnd,*vIdx); return -1; }
  if (special == DMEM_IDX_CURW) { vIdxEnd -= *vIdx; *vIdx = curW; vIdxEnd += curW; }
  else if (special != -1) return -1;
  if (*vIdx < 0 || *vIdx > curW) return -1;
  SMILE_DBG(5,"validateIdxRangeW(2) vidx=%i vidxend=%i special=%i curW=%i curR=%i nT=%i",*vIdx,vIdxEnd,special,curW,curR,lcfg.nT);

  if (lcfg.isRb) {
    if (vIdxEnd-*vIdx > (lcfg.nT - (curW-curR))) {
      if (!lcfg.growDyn) {
        bool nh = (lcfg.noHang == 1 && nReaders == 0) || lcfg.noHang == 2;
        if (!nh) {
          SMILE_DBG(3,"data lost while writing value to ringbuffer level '%s'",getName());
          return -1;
        }
      } else {
        long newS = lcfg.nT*2;
        if (newS < (curW-curR) + (vIdxEnd-*vIdx)) newS = (curW-curR) + (vIdxEnd-*vIdx) + 10;

        if (!growLevel(newS)) return -1;  
      }
    }
    if (vIdxEnd>=curW) curW = vIdxEnd;
    if ((lcfg.noHang==2)&&(vIdxEnd-*vIdx > (lcfg.nT - (curW-curR)))) {
      SMILE_DBG(3,"data lost while writing matrix to ringbuffer level '%s' (vIdxEnd %i, *vIdx %i, lcfg.nT %i, curW %i, curR %i)",getName(),vIdxEnd,*vIdx,lcfg.nT,curW,curR);
    }
    return *vIdx%lcfg.nT;
  } else { // no ringbuffer
    if ((*vIdx >= lcfg.nT)||(vIdxEnd > lcfg.nT)) {
      if (!lcfg.growDyn) {
        SMILE_ERR(3, "Data lost while writing matrix of size %ld to level '%s'", vIdxEnd - *vIdx, getName());
        return -1;
      }
      long newS = lcfg.nT*2;
      if (newS < vIdxEnd) newS = vIdxEnd+10;
      if (newS < *vIdx) newS = *vIdx+10;

      if (!growLevel(newS)) return -1;
    }
    if (vIdxEnd>=curW) curW = vIdxEnd;
    return *vIdx;
  }
}

long cDataMemoryLevel::validateIdxR(long *vIdx, int special, int rdId, int noUpd)
{
  long *_curR;
  if ((rdId >= 0)&&(rdId<nReaders)) _curR = curRr+rdId;
  else _curR=&curR;
  SMILE_DBG(5,"validateIdxR ('%s')\n         vidx=%i special=%i curW=%i curR=%i nT=%i",getName(),*vIdx,special,curW,*_curR,lcfg.nT);

  if ((lcfg.isRb) && (*_curR < curW-lcfg.nT)) { *_curR = curW-lcfg.nT; SMILE_DBG(3,"validateIdxR: rb data possibly lost, curR < curW-nT, curR was automatically increased!"); }
  if (special == DMEM_IDX_CURR) *vIdx = *_curR;
  else if ((special != DMEM_IDX_ABS)&&(special!=DMEM_PAD_ZERO)&&(special!=DMEM_PAD_FIRST)&&(special!=DMEM_PAD_NONE)) return -1;
  if (*vIdx < 0) return -2;
  // TODO:: set curR to min curRr
  if (lcfg.isRb) {
    if ((*vIdx < curW)&&(*vIdx >= curW-lcfg.nT)) { 
      if (!noUpd) { 
        if ((*vIdx>=*_curR)&&(!noUpd)) *_curR = *vIdx+1; 
        if (rdId >= 0) checkCurRr(); 
      } 
      return *vIdx%lcfg.nT; 
    } else if (*vIdx >= curW) { return -3; } // OOR_right
    else if (*vIdx < curW-lcfg.nT) { return -2; } // OOR_left
  } else { // no ringbuffer
    if ((*vIdx < curW)&&(*vIdx < lcfg.nT)) { 
      if (!noUpd) { 
        if (*vIdx>=*_curR) *_curR = *vIdx+1; 
        if (rdId >= 0) checkCurRr(); 
      } 
      return *vIdx; 
    } else if (*vIdx >= curW) { return -3; } // OOR_right
    else if (*vIdx >= lcfg.nT) { return -4; } // OOR_buffersize
  }
  return -1;
}

// TODO: the variable "special" is used both for a special (relative) vIdx AND the padding behaviour for matrix reading.
// This means when using padding, the special index is not available, as it is assumed to always be DMEM_IDX_ABS when a padding flag is given.
// This is fine for now, but at some time it would be cleaner to reserve the lower bits for "special" index and the higher bits for padding.
// Then we would need tests to show if special (start) index works in combination with padding flags (should be ok, but needs to be tested). 
// Or we get rid of special indices completely as it does not seem to be used anywhere (except in unit tests).
long cDataMemoryLevel::validateIdxRangeR(long actualVidx, long *vIdx, long vIdxEnd, int special, int rdId, int noUpd, int *padEnd)
{
  SMILE_DBG(5,"validateIdxRangeR ('%s')\n         vidx=%i vidxend=%i special=%i curW=%i curR=%i nT=%i",getName(),*vIdx,vIdxEnd,special,curW,curR,lcfg.nT);
  long *_curR;
  if ((rdId >= 0)&&(rdId<nReaders)) _curR = curRr+rdId;
  else _curR=&curR;
  SMILE_DBG(5,"validateIdxRangeR(2) '%s' vidx=%i vidxend=%i special=%i curW=%i _curR=%i nT=%i",this->lcfg.name,*vIdx,vIdxEnd,special,curW,*_curR,lcfg.nT);

  if ((lcfg.isRb) && (*_curR < curW-lcfg.nT)) {
    *_curR = curW-lcfg.nT;
    SMILE_WRN(4, "level: '%s': validateIdxRangeR: rb data possibly lost, curR < curW-nT, curR was automatically increased!", lcfg.name);
  }
  if (vIdxEnd < *vIdx) { SMILE_ERR(2,"validateIdxRangeR: vIdxEnd (%i) cannot be smaller than vIdx (%i)!",vIdxEnd,*vIdx); return -1; }
  if (special == DMEM_IDX_CURR) { vIdxEnd -= *vIdx; actualVidx = *vIdx = *_curR; vIdxEnd += *_curR; }
  else if ((special != DMEM_IDX_ABS)&&(special!=DMEM_PAD_ZERO)&&(special!=DMEM_PAD_FIRST)&&(special!=DMEM_PAD_NONE)) return -1;
  if (*vIdx < 0) return -1;

  if ((vIdxEnd > curW)&&(isEOI())) { // pad
    if (padEnd != NULL) {
      *padEnd = vIdxEnd - curW;
      if (*padEnd >= vIdxEnd-*vIdx) { *padEnd = vIdxEnd-*vIdx;  return -1; }
    }
    // TODO: pad option for "truncate" at the end of input!
    vIdxEnd = curW;
  }
  if ((lcfg.isRb)&&(*vIdx < curW)&&(vIdxEnd <= curW)&&(*vIdx >= curW-lcfg.nT)) { 
    if (!noUpd) { 
      //if (vIdxEnd>=*_curR) *_curR = *vIdx+1; if (rdId >= 0) checkCurRr(); 
      if (vIdxEnd>=*_curR) *_curR = actualVidx+1; if (rdId >= 0) checkCurRr(); 
    } 
    return *vIdx%lcfg.nT; 
  } else if ((!lcfg.isRb)&&(*vIdx < curW)&&(*vIdx < lcfg.nT)&&(vIdxEnd <= curW)&&(vIdxEnd <= lcfg.nT)) { // +1 ????? XXX
    if (!noUpd) { 
      if (vIdxEnd>=*_curR) *_curR = actualVidx+1; if (rdId >= 0) checkCurRr(); 
      //if (vIdxEnd>=*_curR) *_curR = *vIdx+1; if (rdId >= 0) checkCurRr(); 
    } 
    return *vIdx; 
  }
  if (padEnd != NULL) *padEnd = 0;
  return -1;
}

void cDataMemoryLevel::frameWr(long rIdx, const FLOAT_DMEM *_data)
{
  FLOAT_DMEM *f = data->data + rIdx*lcfg.N;
  FLOAT_DMEM *end = f+lcfg.N;
  for ( ; f < end; f++) {
    *f = *_data;
    _data++;
  }
}

void cDataMemoryLevel::frameRd(long rIdx, FLOAT_DMEM *_data) const
{
  FLOAT_DMEM *f = data->data + rIdx*lcfg.N;
  FLOAT_DMEM *end = f+lcfg.N;
  for ( ; f < end; f++) {
    *_data = *f;
    _data++;
  }
}

void cDataMemoryLevel::printLevelStats(int detail) const
{
  if (detail) {
    SMILE_PRINT("==> LEVEL '%s'  +++  Buffersize(frames) = %i  +++  nReaders = %i",getName(),lcfg.nT,nReaders);
    if (detail >= 2) {
    // TODO: more details  AND warn if nReaders == 0 or size == 0, etc.
      SMILE_PRINT("     Period(in seconds) = %f \t frameSize(in seconds) = %f (last: %f)",
          lcfg.T, lcfg.frameSizeSec, lcfg.lastFrameSizeSec);
      SMILE_PRINT("     BlocksizeRead(frames) = %i \t BlocksizeWrite(frames) = %i",
          lcfg.blocksizeReader, lcfg.blocksizeWriter);
      SMILE_PRINT("     noTimeMeta = %d", lcfg.noTimeMeta);
      if (detail >= 3) {
        SMILE_PRINT("     Number of elements: %i \t Number of fields: %i",lcfg.N,lcfg.Nf);
        if (detail >= 4) {
          const char * typeStr = "float";
          SMILE_PRINT("     type = %s   noHang = %i   isRingbuffer(isRb) = %i   growDyn = %i",typeStr,lcfg.noHang,lcfg.isRb,lcfg.growDyn);
          if (detail >= 5) {
            // TODO: print data element names ??
            int i; long idx = 0;
            SMILE_PRINT("     Fields: index (range) : fieldname[array indicies]  (# elements)");
            for (i=0; i<fmeta.N; i++) {
              if (fmeta.field[i].N > 1) {
                SMILE_PRINT("      %2i. - %2i. : %s[%i-%i]  (%i)",idx,idx+fmeta.field[i].N-1,fmeta.field[i].name,fmeta.field[i].arrNameOffset,fmeta.field[i].N+fmeta.field[i].arrNameOffset-1,fmeta.field[i].N);
                idx += fmeta.field[i].N;
              } else {
                SMILE_PRINT("      %2i.       : %s",idx,fmeta.field[i].name);
                idx++;
              }
            }
          }
          if (detail >= 6) {
            SMILE_PRINT("     Fields with info struct set: (index (range) : info struct size in bytes (dt = datatype))");
            long idx = 0;
            for (int i = 0; i < fmeta.N; i++) {
              if (fmeta.field[i].infoSet) {
                if (fmeta.field[i].N > 1) {
                  SMILE_PRINT("       %2i. - %2i. : infoSize = %i (dt = %i)", idx, idx+fmeta.field[i].N-1, fmeta.field[i].infoSize, fmeta.field[i].dataType);
                  idx += fmeta.field[i].N;
                } else {
                  SMILE_PRINT("       %2i.       : infoSize = %i (dt = %i)", idx, fmeta.field[i].infoSize, fmeta.field[i].dataType);
                  idx++;
                }
              }
            }
          }
        }
      }
    }
  }
  if (nReaders <= 0) SMILE_WRN(1,"   Level '%s' might be a DEAD-END (nReaders <= 0!)",getName());
}

const sDmLevelConfig * cDataMemoryLevel::queryReadConfig(long blocksizeReader)
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

const sDmLevelConfig * cDataMemoryLevel::queryReadConfig(double blocksizeReaderSeconds)
{
  if (lcfg.T != 0.0)
    return queryReadConfig((long)ceil(blocksizeReaderSeconds / lcfg.T));
  else
    return queryReadConfig((long)ceil(blocksizeReaderSeconds));
}

const char * cDataMemoryLevel::getFieldName(int n, int *lN, int *_arrNameOffset) const
{
  //TODO::: use fmeta getName and implement expanding of array indices... (see cVector::name)
  if ( (n>=0)&&(n<lcfg.N) ) {
    if (lN!=NULL) *lN = fmeta.field[n].N;
	if (_arrNameOffset!=NULL) *_arrNameOffset = fmeta.field[n].arrNameOffset;
    return fmeta.field[n].name;
  }
  else return NULL;
}

char * cDataMemoryLevel::getElementName(int n) const
{
  if ( (n>=0)&&(n<lcfg.N) ) { // TODO:: is N correct? number of elements??
    int llN=0;
    const char *tmp = fmeta.getName(n,&llN);
    char *ret;
    if (llN >= 0) ret = myvprint("%s[%i]",tmp,llN);
    else ret = strdup(tmp); //myvprint("%s",tmp,__N);
    return ret;
  }
  else return NULL;
}

void cDataMemoryLevel::setFrameSizeSec(double fss)
{
  if (fss > 0.0) lcfg.frameSizeSec = fss;
}

void cDataMemoryLevel::setBlocksizeWriter(long _bsw, int _force)
{ 
  if (_force) { lcfg.blocksizeWriter = _bsw; }
  else if (_bsw > lcfg.blocksizeWriter) { lcfg.blocksizeWriter = _bsw; }
}

void cDataMemoryLevel::allocReaders() {
  // allocate and initialize *curRr
  if (nReaders > 0) { // if registered readers are present...
    curRr = (long*)calloc(1,sizeof(long)*nReaders);
  }
}

// set time meta information for frame at rIdx
void cDataMemoryLevel::setTimeMeta(long rIdx, long vIdx, const TimeMetaInfo *tm)
{
  if (lcfg.noTimeMeta)
    COMP_ERR("cannot set time meta information for a level without TimeMetaInfo (noTimeMeta = 1)");
  TimeMetaInfo *cur = tmeta+rIdx;
  if (tm!=NULL) {
    *cur = *tm; 
    cur->vIdx = vIdx;
    cur->period = lcfg.T;
    // keep an existing smiletime, else set the current smile time
    if (tm->smileTime >= 0.0) cur->smileTime = tm->smileTime;
    else {
      if (cur->smileTime == -1.0 ) {
        if (_parent != NULL) {
          cComponentManager * cm = (cComponentManager *)_parent->getCompMan();
          if (cm != NULL) {
            cur->smileTime = cm->getSmileTime();
          } else { cur->smileTime = -1.0; }
        } else { cur->smileTime = -1.0; }
      }
    }
    if (cur->time == 0.0) {
      if (lcfg.T!=0.0) cur->time = (double)vIdx * lcfg.T;
      else if (tm->time != 0.0) {
        // TODO: check if the above if does not break anything else...
        cur->time = tmeta[(rIdx-1+lcfg.nT)%lcfg.nT].time + tmeta[(rIdx-1+lcfg.nT)%lcfg.nT].lengthSec;
      }
    }
    if (!(cur->filled)) {
      if (cur->lengthSec == 0.0) cur->lengthSec = tmeta[(rIdx-1+lcfg.nT)%lcfg.nT].lengthSec;
      if (cur->lengthSec == 0.0) cur->lengthSec = lcfg.T;
      cur->filled = 1;
    }
  } else {
    // zero current time meta??
  }
}

// get time meta information for frame at rIdx
void cDataMemoryLevel::getTimeMeta(long rIdx, long vIdx, TimeMetaInfo *tm) const
{
  if (!lcfg.noTimeMeta) {
    TimeMetaInfo *cur = tmeta+rIdx;
    *tm = *cur;
  } else {
    // if no time meta was stored, we reconstruct basic information from vIdx and lcfg.T
    // at least, this should ensure compatibility of components that do not depend on any special metadata fields
    tm->filled = 1;
    tm->vIdx = vIdx;    
    tm->period = lcfg.T;
    tm->time = (double)vIdx * lcfg.T;
    tm->lengthSec = lcfg.T;
    tm->framePeriod = 0.0;
    tm->smileTime = -1.0;
    tm->metadata = NULL;
  }
}

int cDataMemoryLevel::setFieldInfo(int i, int _dataType, void *_info, long _infoSize)
{
  if ((i>=0)&&(i < fmeta.N)) {
    if (_dataType >= 0) fmeta.field[i].dataType = _dataType;
    if (_infoSize >= 0) fmeta.field[i].infoSize = _infoSize;
    if (_info != NULL) {
      if (fmeta.field[i].info != NULL) free(fmeta.field[i].info);
      fmeta.field[i].info = _info;
    }
    fmeta.field[i].infoSet = 1;
    return 1;
  }
  return 0;
}

int cDataMemoryLevel::setFieldInfo(const char *__name, int _dataType, void *_info, long _infoSize) {
  return setFieldInfo(fmeta.findField(__name), _dataType, _info, _infoSize);
}

int cDataMemoryLevel::addField(const char *_name, int lN, int arrNameOffset)
{
  if (lcfg.finalised) {
    SMILE_ERR(2,"cannot add field '%s' to level '%s' , level is already finalised!",_name,getName());
    return 0;
  }
  //TODO: check for uniqueness of name!!

  if (fmeta.N >= fmetaNalloc) { //realloc:
    FieldMetaInfo *f;
    f = (FieldMetaInfo *)realloc(fmeta.field,sizeof(FieldMetaInfo)*(fmeta.N+LOOKAHEAD_ALLOC));
    if (f==NULL) OUT_OF_MEMORY;
    // zero newly allocated data
    int i;
    for (i=fmeta.N; i<fmeta.N+LOOKAHEAD_ALLOC; i++) {
      f[i].name = NULL;
      f[i].N = 0;
      f[i].dataType = 0;
      f[i].info = NULL;
      f[i].infoSize = 0;
      f[i].infoSet = 0;
    }
    fmeta.field = f;
    fmetaNalloc = fmeta.N + LOOKAHEAD_ALLOC;
  }
  if (lN <= 0) 
    lN = 1;
  fmeta.field[fmeta.N].N = lN;
  fmeta.field[fmeta.N].Nstart = lcfg.N;
  fmeta.field[fmeta.N].name = strdup(_name);
  fmeta.field[fmeta.N].arrNameOffset = arrNameOffset;
  fmeta.N++; 
  lcfg.Nf++; 
  fmeta.Ne += lN;
  lcfg.N += lN;
    
  SMILE_DBG(4,"added field '%s' to level '%s' (size %i).",_name,getName(),lN);
  return 1;
}

int cDataMemoryLevel::findField(const char*_fieldName, int *arrIdx) const { 
  return fmeta.findField( _fieldName, arrIdx );
}

void cDataMemoryLevel::setArrNameOffset(int arrNameOffset)
{
  if ((fmeta.N-1 < fmetaNalloc)&&(fmeta.N-1 >= 0)) {
    fmeta.field[fmeta.N-1].arrNameOffset = arrNameOffset;
  }
}

// finalize config and allocate data memory
int cDataMemoryLevel::configureLevel()
{
  /*
  if (lcfg.blocksizeIsSet) return 1;

  // TODO: what is the actual minimum buffersize, given blocksizeRead and blocksizeWrite??
  long minBuf;  // = blocksizeRead + 2*blocksizeWrite;
  if (lcfg.blocksizeReader <= lcfg.blocksizeWriter) {
    minBuf = 2*lcfg.blocksizeWriter + 1;  // +1 just for safety...
  } else {
    minBuf = MAX( 2*lcfg.blocksizeWriter, (lcfg.blocksizeReader/lcfg.blocksizeWriter) * lcfg.blocksizeWriter ) + 1; // +1 just for safety...
  }

  // adjust level buffersize based on blocksize from write requests...
  if (lcfg.nT<minBuf) {
    lcfg.nT = minBuf;
  }

  lcfg.blocksizeIsSet = 1;
  */
  return 1;
}

// Finalizes config and allocates data memory buffer
int cDataMemoryLevel::finaliseLevel()
{
  if (lcfg.finalised)
    return 1;
  long minBuf;
  if (lcfg.blocksizeReader <= lcfg.blocksizeWriter) {
    minBuf = 2 * lcfg.blocksizeWriter + 1;  // +1 just for safety...
  } else {
    minBuf = lcfg.blocksizeReader + 2 * lcfg.blocksizeWriter; // +1 just for safety...
  }
  // adjust level buffersize based on blocksize from write requests...
  if (lcfg.nT < minBuf) {
    lcfg.nT = minBuf;
  }
  lcfg.blocksizeIsSet = 1;
  if ( (!lcfg.blocksizeIsSet) || (!lcfg.namesAreSet) ) {
    COMP_ERR("cannot finalise level '%s' : blocksizeIsSet=%i, namesAreSet=%i (both should be 1...)",
        getName(), lcfg.blocksizeIsSet, lcfg.namesAreSet);
  }

  // allocate data matrix
  if ((lcfg.N<=0)||(lcfg.nT<=0)) COMP_ERR("cDataMemoryLevel::finaliseLevel: cannot allocate matrix with one (or more) dimensions == 0. did you add fields to this level ['%s']? (N=%i, nT=%i)",getName(),lcfg.N,lcfg.nT);
  data = new cMatrix(lcfg.N,lcfg.nT,lcfg.noTimeMeta);
  if (data==NULL) COMP_ERR("cannot allocate level (out of memory)!");
  
  if (!lcfg.noTimeMeta) {
    // allocate tmeta
    tmeta = new TimeMetaInfo[lcfg.nT]();
    if (tmeta == NULL) OUT_OF_MEMORY;
  }
  
  // initialise mutexes:
  smileMutexCreate(RWptrMtx);
  smileMutexCreate(RWmtx);
  smileMutexCreate(RWstatMtx);
  
  lcfg.finalised = 1;
  return 1;
}

int cDataMemoryLevel::checkRead(long vIdx, int special, int rdId, long len, int *result) 
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

void cDataMemoryLevel::catchupCurR(int rdId, int _curR) 
{
  smileMutexLock(RWptrMtx);
  if ((rdId < 0)||(rdId >= nReaders)) { 
    if ((_curR >= 0)&&(_curR <= curW)) curR = _curR;
    else curR = curW;
    checkCurRr(); // sync new global with readers' indicies
  } else {
    if ((_curR >= 0)&&(_curR <= curW)) curRr[rdId] = _curR;
    else curRr[rdId] = curW;
    checkCurRr(); // sync new readers' indicies with global
  }
  smileMutexUnlock(RWptrMtx);
}

long cDataMemoryLevel::getCurW() const 
{
  smileMutexLock(RWptrMtx);
  long res = curW;
  smileMutexUnlock(RWptrMtx);
  return res;
}  

long cDataMemoryLevel::getCurR(int rdId) const
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

long cDataMemoryLevel::getNFree(int rdId) const
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

long cDataMemoryLevel::getNAvail(int rdId) const
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

int cDataMemoryLevel::setFrame(long vIdx, const cVector *vec, int special)  // id must already be resolved...!
{
  if (!lcfg.finalised) { COMP_ERR("cannot set frame in non-finalised level! call finalise() first!"); }
  if (vec == NULL) {
    SMILE_ERR(3,"cannot set frame in dataMemory from a NULL cVector object!");
    return 0;
  }

  if (lcfg.N != vec->N) { COMP_ERR("setFrame: cannot set frame in level '%s', framesize mismatch: %i != %i (expected)",getName(),vec->N,lcfg.N); }

//****** acquire write lock.... *******
  smileMutexLock(RWstatMtx);
  // set write request flag, in case the level is currently locked for reading
  writeReqFlag = 1;
  smileMutexUnlock(RWstatMtx);
  smileMutexLock(RWmtx); // get exclusive lock for writing..
  smileMutexLock(RWstatMtx);
  writeReqFlag = 0;
  smileMutexUnlock(RWstatMtx);
//****************

  smileMutexLock(RWptrMtx);
#ifdef DM_DEBUG_LOGGER
  long vIdx0=vIdx;
#endif
  long rIdx = validateIdxW(&vIdx,special);
#ifdef DM_DEBUG_LOGGER
  // logging in setFrame:
  datamemoryLogger(this->lcfg.name, vIdx0, vIdx, rIdx, lcfg.nT, special, this->curR, this->curW, this->EOI, this->nReaders, this->curRr, vec);
#endif
  smileMutexUnlock(RWptrMtx);
  
  int ret = 0;
  if (rIdx>=0) {
    frameWr(rIdx, vec->data);
    if (!lcfg.noTimeMeta) {
      setTimeMeta(rIdx,vIdx,vec->tmeta);
    }
    ret= 1;
  } else {
    SMILE_ERR(4,"setFrame: frame index (vIdx %i -> rIdx %i) out of range, frame was not set (level '%s')!",vIdx,rIdx,getName());
  }

  smileMutexUnlock(RWmtx);
  return ret;
}

int cDataMemoryLevel::setMatrix(long vIdx, const cMatrix *mat, int special)  // id must already be resolved...!
{
  if (!lcfg.finalised) { COMP_ERR("cannot set matrix in non-finalised level '%s'! call finalise() first!",getName()); }
  if (mat == NULL) {
    SMILE_ERR(3,"cannot set frame in dataMemory from a NULL cMatrix object!");
    return 0;
  }

  if (lcfg.N != mat->N) { COMP_ERR("setMatrix: cannot set frames in level '%s', framesize mismatch: %i != %i (expected)",getName(),mat->N,lcfg.N); }

//****** acquire write lock.... *******
  smileMutexLock(RWstatMtx);
  // set write request flag, in case the level is currently locked for reading
  writeReqFlag = 1;
  smileMutexUnlock(RWstatMtx);
  smileMutexLock(RWmtx); // get exclusive lock for writing..
  smileMutexLock(RWstatMtx);
  writeReqFlag = 0;
  smileMutexUnlock(RWstatMtx);
//****************

  // validate start index
  smileMutexLock(RWptrMtx);
  long rIdx = validateIdxRangeW(&vIdx,vIdx+mat->nT,special);
  smileMutexUnlock(RWptrMtx);

  int ret = 0;
  if (rIdx>=0) {
    long i;
    /* double smileTm = -1.0;
    if (_parent != NULL) {
      cComponentManager * cm = (cComponentManager *)_parent->getCompMan();
      if (cm != NULL) {
        smileTm = cm->getSmileTime();
      }
    }
    */

    for (i=0; i<mat->nT; i++) { 
      //if ((mat->tmeta + i)->smileTime == -1.0 ) (mat->tmeta + i)->smileTime = smileTm;
      frameWr((rIdx+i)%lcfg.nT, mat->data + i*lcfg.N);
      if (!lcfg.noTimeMeta) {
        setTimeMeta((rIdx+i)%lcfg.nT,vIdx+i,mat->tmeta + i); 
      }
    }
    ret = 1;
  } else {
    SMILE_DBG(4,"ERROR, setMatrix: frame index range (vIdxStart %i - vIdxEnd %i  => rIdxStart %i) out of range, frame was not set (level '%s')!",vIdx,vIdx+mat->nT,rIdx,getName());
  }

  smileMutexUnlock(RWmtx);
  return ret;
}


//TODO: implement concealment strategies, when frames are not available (also report concealment method to caller)
//    for vector: return 0'ed frame instead of NULL pointer
//                return last available frame (beginning / end)
//    for matrix: return NULL pointer if complete matrix is out of range
//                fill partially (!) missing frames with 0es
//                repeat first/last possible frames...

// NOTE: caller must free the returned vector!!
cVector * cDataMemoryLevel::getFrame(long vIdx, int special, int rdId, int *result)
{
  if (!lcfg.finalised) { COMP_ERR("cannot get frame from non-finalised level '%s'! call finalise() first!",getName()); }

//****** acquire read lock.... *******
  smileMutexLock(RWstatMtx);
  // check for urgent write request:
  while (writeReqFlag) { // wait until write request has been served!
    smileMutexUnlock(RWstatMtx);
    smileYield();
    smileMutexLock(RWstatMtx);
  }
  if (nCurRdr == 0) {
    smileMutexLock(RWmtx); // no other readers, so lock mutex to exclude writes...
  }
  nCurRdr++;
  smileMutexUnlock(RWstatMtx);
//****************

  smileMutexLock(RWptrMtx);
  long rIdx = validateIdxR(&vIdx,special,rdId);
  smileMutexUnlock(RWptrMtx);

  cVector *vec=NULL;
  if (rIdx>=0) {
    vec = new cVector(lcfg.N);
    if (vec == NULL) OUT_OF_MEMORY;
    frameRd(rIdx, vec->data);
    getTimeMeta(rIdx,vIdx,vec->tmeta);
    vec->fmeta = &(fmeta);
    if (result!=NULL) *result=DMRES_OK;
  } else {
    SMILE_DBG(4,"getFrame: frame index (vIdx %i -> rIdx %i) out of range, frame cannot be read (level '%s')!",vIdx,rIdx,getName());
    if (result!=NULL) {
      if (rIdx == -2) *result=DMRES_OORleft|DMRES_ERR;
      else if (rIdx == -3) *result=DMRES_OORright|DMRES_ERR;
      else if (rIdx == -4) *result=DMRES_OORbs|DMRES_ERR;
      else *result=DMRES_ERR;
    }
  }

  //**** now unlock ******
  smileMutexLock(RWstatMtx);
  nCurRdr--;
  if (nCurRdr < 0) { // ERROR!!
    SMILE_ERR(1,"nCurRdr < 0  while unlocking dataMemory!! This is a BUG!!!");
    nCurRdr = 0;
  }
  if (nCurRdr==0) smileMutexUnlock(RWmtx);
  smileMutexUnlock(RWstatMtx);
  //********************

  return vec;
}

//TODO: add an optimized 'simple' level for high performance and low overhead wave handling
//  no tmeta, very simple access functions etc.
//  tmeta if accessed will be emulated?
cMatrix * cDataMemoryLevel::getMatrix(long vIdx, long vIdxEnd, int special, int rdId, int *result)
{
  if (!lcfg.finalised) { COMP_ERR("cannot get matrix from non-finalised level! call finalise() first!"); }

  long vIdxold=vIdx;
//  long vIdxEndO = vIdxEnd;
  if (vIdx < 0) vIdx = 0;
  int padEnd = 0; // will be filled with the number of samples at the end of the matrix to be padded

//****** acquire read lock.... *******
  smileMutexLock(RWstatMtx);
  // check for urgent write request:
  while (writeReqFlag) { // wait until write request has been served!
    smileMutexUnlock(RWstatMtx);
    smileYield();
    smileMutexLock(RWstatMtx);
  }
  if (nCurRdr == 0) {
    smileMutexLock(RWmtx); // no other readers, so lock mutex to exclude writes...
  }
  nCurRdr++;
  smileMutexUnlock(RWstatMtx);
//****************

  smileMutexLock(RWptrMtx);
  // TODO : if EOI state, then allow vIdxEnd out of range! pad frame...
  long rIdx = validateIdxRangeR(vIdxold, &vIdx, vIdxEnd, special, rdId, 0, &padEnd);
  smileMutexUnlock(RWptrMtx);

  cMatrix *mat=NULL;
  if (rIdx>=0) {
    if (vIdxold < 0) mat = new cMatrix(lcfg.N,vIdxEnd-vIdxold);
    else mat = new cMatrix(lcfg.N,vIdxEnd-vIdx);
    SMILE_DBG(4,"creating new data matrix (%s)  vIdxold=%i , vIdx=%i, vIdxEnd=%i, lcfg.N=%i",this->getName(),vIdxold,vIdx,vIdxEnd,lcfg.N)
    if (mat == NULL) OUT_OF_MEMORY;
    long i,j;
    if (vIdxold < 0) {
      long i0 = 0-vIdxold;
      for (i=0; i<i0; i++) {
        if (special == DMEM_PAD_ZERO) for (j=0; j<mat->N; j++) mat->data[i*lcfg.N+j] = 0.0; // pad with value
        else frameRd((rIdx)%lcfg.nT, mat->data + (i*lcfg.N)); // fill with first frame
        getTimeMeta((rIdx)%lcfg.nT, vIdx + i, mat->tmeta + i);
      }
      for (i=0; i<vIdxEnd; i++) { 
        frameRd((rIdx+i)%lcfg.nT, mat->data + (i+i0)*lcfg.N);
        getTimeMeta((rIdx+i)%lcfg.nT, vIdx + i + i0, mat->tmeta + i +i0);
      }
    } else if (padEnd>0) {
      for (i=0; i<(vIdxEnd-vIdx)-padEnd; i++) {
        frameRd((rIdx+i)%lcfg.nT, mat->data + (i*lcfg.N));
        getTimeMeta((rIdx+i)%lcfg.nT, vIdx + i, mat->tmeta + i);
      }
      long i0 = i-1;
      for (; i<(vIdxEnd-vIdx); i++) {
        if (special == DMEM_PAD_ZERO) for (j=0; j<mat->N; j++) mat->data[i*lcfg.N+j] = 0.0; // pad with value
        else frameRd((rIdx+i0)%lcfg.nT, mat->data + (i*lcfg.N)); // fill with last frame
        getTimeMeta((rIdx+i0)%lcfg.nT, vIdx + i, mat->tmeta + i);
      }
      // TODO: Test DMEM_PAD_NONE option to truncate the frame!!
      if (special == DMEM_PAD_NONE) {
        mat->nT = (vIdxEnd-vIdx)-padEnd;
      }
    } else {
      for (i=0; i<mat->nT; i++) {
        frameRd((rIdx+i)%lcfg.nT, mat->data + i*lcfg.N);
        getTimeMeta((rIdx+i)%lcfg.nT, vIdx + i, mat->tmeta + i);
      }
    }
    mat->fmeta = &(fmeta);
    if (result!=NULL) *result=DMRES_OK;
  } else {
    SMILE_DBG(4,"ERROR, getMatrix: frame index range (vIdxStart %i - vIdxEnd %i  => rIdxStart %i) out of range, matrix cannot be read (level '%s')!",vIdx,vIdxEnd,rIdx,getName());
    if (result!=NULL) *result=DMRES_ERR;
  }
  
  //**** now unlock ******
  smileMutexLock(RWstatMtx);
  nCurRdr--;
  if (nCurRdr < 0) { // ERROR!!
    SMILE_ERR(1,"nCurRdr < 0  while unlocking dataMemory!! This is a BUG!!!");
    nCurRdr = 0;
  }
  if (nCurRdr==0) smileMutexUnlock(RWmtx);
  smileMutexUnlock(RWstatMtx);
  //********************
  
  return mat;
}

// methods to get info about current level fill status (e.g. number of frames written, curW, curR(global) and freeSpace, etc.)
long cDataMemoryLevel::getMaxR() const  // maximum readable index or -1 if level is empty
{ 
  smileMutexLock(RWptrMtx);
  long res = curW-1;
  smileMutexUnlock(RWptrMtx);
  return res;
}

long cDataMemoryLevel::getMinR() const {  // minimum readable index or -1 if level is empty (relevant only for ringbuffers, otherwise it will always return -1 or 0)
  long res;
  smileMutexLock(RWptrMtx);
  if (lcfg.isRb) {
    if (curW > 0) {
      if (!lcfg.growDyn) {
        res = (std::max)(curW-lcfg.nT, 0L);
      } else {
        res = (std::max)(curW-lcfg.nT, minRAtLastGrowth);
      }
    } else {
      res = -1;
    }
  } else {
    if (curW > 0) {
      res = 0;
    } else {
      res = -1;
    }
  }
  smileMutexUnlock(RWptrMtx);
  return res;
}

// functions to convert absolute times (in seconds) to level frame indicies!
long cDataMemoryLevel::secToVidx(double sec) const { // returns a vIdx
   //(for periodic levels this function is easy, however for aperiodic levels we must iterate through all frames... maybe also for periodic to compensate round-off errors?)
  if (lcfg.T!=0.0) {
    return (long)round(sec/lcfg.T);
  } else {
    SMILE_WRN(0,"cDataMemoryLevel::secToVidx: NOT YET IMPLEMENTED for variable period levels!");
  }
  return 0;
}

double cDataMemoryLevel::vIdxToSec(long vIdx) const { // returns a vIdx
   //(for periodic levels this function is easy, however for aperiodic levels we must iterate through all frames... maybe also for periodic to compensate round-off errors?)
  if (lcfg.T!=0.0) {
    return ((double)(vIdx))*lcfg.T;
  } else {
    SMILE_WRN(0,"cDataMemoryLevel::vIdxToSec: NOT YET IMPLEMENTED for variable period levels!");
  }
  return 0;
}

cDataMemoryLevel::~cDataMemoryLevel() {
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
