/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#include <core/dataMemory.hpp>
#include <core/componentManager.hpp>
#include <algorithm>
#include <limits>

#define MODULE "dataMemory"

SMILECOMPONENT_STATICS(cDataMemory)

SMILECOMPONENT_REGCOMP(cDataMemory)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CDATAMEMORY;
  sdescription = COMPONENT_DESCRIPTION_CDATAMEMORY;

  // dataMemory level's config type:
  ConfigType *dml = new ConfigType("cDataMemoryLevel");
  if (dml == NULL) OUT_OF_MEMORY;
  dml->setField("name", "The name of this data memory level, must be unique within one data memory instance.", (char*)NULL);
  dml->setField("type", "The data type of the level [can be: 'float' or 'int'(eger) , currently only float is supported by the majority of processing components ]", "float");
  dml->setField("isRb", "Flag that indicates whether this level is a ring-buffer level (1) or not (0). I.e. this level stores the last 'nT' frames, and discards old data as new data comes in (if the old data has already been read by all registered readers; if this is not the case, the level will report that it is full to the writer attempting the write operation)", 1);
  dml->setField("nT", "The size of the level buffer in frames (this overwrites the 'lenSec' option)", 100,0,0);
  dml->setField("T", "The frame period of this level in seconds. Use a frame period of 0 for a-periodic levels (i.e. data that does not occur periodically)", 0.0);
  dml->setField("lenSec", "The size of the level buffer in seconds", 0.0);
  dml->setField("frameSizeSec", "The size of one frame in seconds. (This is generally NOT equal to 1/T, because frames may overlap)", 0.0);
  dml->setField("growDyn", "Supported currently only if 'ringbuffer=0'. If this option is set to 1, the level grows dynamically, if more memory is needed. You can use this to store live input of arbitrary length in memory. However, be aware that if openSMILE runs for a long time, it will allocate more and more memory!", 0);
  dml->setField("noHang", "This option controls the 'hang' behaviour for ring-buffer levels, i.e. the behaviour exhibited, when the level is full because data from the ring-buffer has not been marked as read because not all readers registered to read from this level have read data. Valid options are, 0, 1, 2 :\n   0 = always wait for readers, mark the level as full and make writes fail until all readers have read at least the next frame from this level\n   1 = don't wait for readers, if no readers are registered, i.e. this level is a dead-end (this is the default) / 2 = never wait for readers, writes to this level will always succeed (reads to non existing data regions might then fail), use this option with a bit of caution.", 1);

  // dataMemory's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE

  ct->setField("isRb", "The default for the isRb option for all levels.", 1,0,0);
  ct->setField("nT", "The default level buffer size in frames for all levels.", 100,0,0);
  if (ct->setField("level", "An associative array containing the level configuration (obsolete, you should use the cDataWriter configuration in the components that write to the dataMemory to properly configure the dataMemory!)",
                  dml, 1) == -1) {
     rA=1; // if subtype not yet found, request , re-register in the next iteration
  }

  SMILECOMPONENT_IFNOTREGAGAIN( {} )
  
  SMILECOMPONENT_MAKEINFO(cDataMemory);
}

SMILECOMPONENT_CREATE(cDataMemory)


/**** frame meta information *********
   names, indicies (for each row)
 **************************************/

// the returned name is the base name belonging to the expanded field n
// arrIdx will be the index in the array (if an array)
//   or -1 if no array!
//-> n is element(!) index
const char * FrameMetaInfo::getName(int n, int *_arrIdx) const
{
  smileMutexLock(myMtx);

  long lN=Ne; 

  static long nn=0;
  static long ff=0;

  if ((n>=0)&&(n<lN)) {
    long f=0,tmp=0;

    if (n>nn) {f=ff; tmp=nn; } //// fast, but unsafe? TODO: check!
    //TODO: this code is NOT thread safe. solutions:
    // a) use a mutex... this actually should do the job...
    // b) use a lookup table for n->f which must be built earlier or here if it doesn exist (but then we must use mutexes again to ensure exclusive building and waiting of other threads for the built index)
    
    for(; f<N; f++) {
      nn=tmp; ff=f; ////
      tmp += field[f].N;
      if (tmp>n) break;
    }
    if (_arrIdx != NULL) {
      if (field[f].N > 1)
        *_arrIdx = n-(tmp-field[f].N) + field[f].arrNameOffset;
      else
        *_arrIdx = -1;
    }
    smileMutexUnlock(myMtx);
    return field[f].name;
  }
  smileMutexUnlock(myMtx);
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

/******* datatypes ************/

cVector::cVector(int lN, int _type, bool noTimeMeta) :
  N(0), dataF(NULL), dataI(NULL), fmeta(NULL), tmeta(NULL), ntmp(NULL), tmetaAlien(0), tmetaArr(0)
{
  if (lN>0) {
    switch (_type) {
      case DMEM_FLOAT:
        dataF = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*lN);
        if (dataF==NULL) OUT_OF_MEMORY;
        break;
      case DMEM_INT:
        dataI = (INT_DMEM*)calloc(1,sizeof(INT_DMEM)*lN);
        if (dataI==NULL) OUT_OF_MEMORY;
        break;
      default:
        COMP_ERR("cVector: unknown data type encountered in constructor! (%i)",_type);
    }
    N = lN;
    type = _type;
    if (!noTimeMeta) {
      tmeta = new TimeMetaInfo();
      if (tmeta == NULL) OUT_OF_MEMORY;
    }
  }
}

cVector::cVector(std::initializer_list<FLOAT_DMEM> elem) :
  cVector(elem.size(), DMEM_FLOAT)
{
  std::copy(elem.begin(), elem.end(), dataF);
}

cVector::cVector(std::initializer_list<INT_DMEM> elem) :
  cVector(elem.size(), DMEM_INT)
{
  std::copy(elem.begin(), elem.end(), dataI);
}

const char *cVector::name(int n, int *lN) const
{
  if ((fmeta!=NULL)&&(fmeta->field!=NULL)) {
    if (ntmp != NULL) free(ntmp);
    int llN=-1;
    const char *t = fmeta->getName(n,&llN);
    if (lN!=NULL) *lN = llN;
    if (llN>=0) {
      ntmp=myvprint("%s[%i]",t,llN);
      return ntmp;
    } else {
      ntmp=NULL;
      return t;
    }
  }
  return NULL;
}

cVector::~cVector() {
  if (dataF!=NULL) free(dataF);
  if (dataI!=NULL) free(dataI);
  if ((tmeta!=NULL)&&(!tmetaAlien)) {
    if (tmetaArr) delete[] tmeta;
    else delete tmeta;
  }
  if (ntmp!=NULL)  free(ntmp);
}

cMatrix::~cMatrix()
{
  if ((tmeta!=NULL)&&(!tmetaAlien)) {
    if (tmetaArr) delete[] tmeta;
    else delete tmeta;
  }
  tmeta = NULL;
}

cMatrix::cMatrix(int lN, int lnT, int ltype, bool noTimeMeta) :
  cVector(0), nT(0)
{
  if ((lN>0)&&(lnT>0)) {
    switch (ltype) {
      case DMEM_FLOAT:
        dataF = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*lN*lnT);
        if (dataF==NULL) OUT_OF_MEMORY;
        break;
      case DMEM_INT:
        dataI = (INT_DMEM*)calloc(1,sizeof(INT_DMEM)*lN*lnT);
        if (dataI==NULL) OUT_OF_MEMORY;
        break;
      default:
        COMP_ERR("cMatrix: unknown data type encountered in constructor! (%i)",ltype);
    }
    N = lN;
    nT = lnT;
    type = ltype;
    if (!noTimeMeta) {
      tmetaArr = 1;
      tmeta = new TimeMetaInfo[lnT];
      if (tmeta == NULL) OUT_OF_MEMORY;
    }
  }
}

cMatrix::cMatrix(std::initializer_list<std::initializer_list<FLOAT_DMEM>> elem) :
  cMatrix(elem.size(), elem.size() > 0 ? elem.begin()->size() : 0, DMEM_FLOAT)
{
  int r = 0;
  for (const std::initializer_list<FLOAT_DMEM> &row : elem) {
    int c = 0;
    if (row.size() != nT) {
      COMP_ERR("cMatrix: initializer list contains rows of non-uniform length");
    }
    for (FLOAT_DMEM v : row) {
      setF(r, c, v);
      c++;
    }
    r++;
  }
}

cMatrix::cMatrix(std::initializer_list<std::initializer_list<INT_DMEM>> elem) :
  cMatrix(elem.size(), elem.size() > 0 ? elem.begin()->size() : 0, DMEM_INT)
{
  int r = 0;
  for (const std::initializer_list<INT_DMEM> &row : elem) {
    int c = 0;
    if (row.size() != nT) {
      COMP_ERR("cMatrix: initializer list contains rows of non-uniform length");
    }
    for (INT_DMEM v : row) {
      setI(r, c, v);
      c++;
    }
    r++;
  }
}

/* helper function for transposing a matrix */
static void quickTranspose(void *src, void *dst, long R, long C, int elSize)
{
  int r,c;
  char *s=(char*)src, *d=(char*)dst;
  for (r=0; r<R; r++) {  // read each row
    for (c=0; c<C; c++) { // and copy it as column
      memcpy( d+(r*C+c)*elSize, s+(c*R+r)*elSize, elSize );
    }
  }
}

// transpose matrix
void cMatrix::transpose()
{
  FLOAT_DMEM *f = NULL;
  INT_DMEM *i = NULL;
  switch (type) {
    case DMEM_FLOAT:
      f = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*N*nT);
      if (f==NULL) OUT_OF_MEMORY;
      // transpose:
      quickTranspose(dataF,f,N,nT,sizeof(FLOAT_DMEM));
      free(dataF);
      dataF = f;
      break;
    case DMEM_INT:
      i = (INT_DMEM*)calloc(1,sizeof(INT_DMEM)*N*nT);
      if (i==NULL) OUT_OF_MEMORY;
      // transpose:
      quickTranspose(dataI,i,N,nT,sizeof(INT_DMEM));
      free(dataI);
      dataI = i;
      break;
    default:
      COMP_ERR("cMatrix::transpose: unknown data type (%i) encountered, cannot transpose this matrix!",type);
  }
  // swap dimensions:
  long tmp = N;
  N = nT;
  nT = tmp;
}

// reshape to given dimensions, perform sanity check...
void cMatrix::reshape(long R, long C)
{
  long len = N*nT;
  if (R*C > len) SMILE_ERR(1,"reshape: cannot reshape matrix, new size (%i*%i=%i) is greater than current size (%i*%i=%i)! this is not yet implemented! \n   (maybe there should be a resize method in the future...)",R,C,R*C,N,nT,len);
  if (R*C < len) SMILE_WRN(1,"reshape: new matrix will be smaller than current matrix, not all data will be accessible (maybe use resize() in future releases!");
  R=N;
  C=nT;
}

// convert matrix to long vector with all columns concatenated ...
// return a compatible cVector object
cVector * cMatrix::expand()
{
  N = N*nT;
  return (cVector*)this;
}


/******* dataMemory level class ************/


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
      void *tmp;
      if (data->type == DMEM_FLOAT) {
        tmp = calloc(sizeof(FLOAT_DMEM), lcfg.nT * data->N);            
      } else if (data->type == DMEM_INT) {
        tmp = calloc(sizeof(INT_DMEM), lcfg.nT * data->N);
      }
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
  FLOAT_DMEM *f = data->dataF + rIdx*lcfg.N;
  FLOAT_DMEM *end = f+lcfg.N;
  for ( ; f < end; f++) {
    *f = *_data;
    _data++;
  }
}

void cDataMemoryLevel::frameWr(long rIdx, const INT_DMEM *_data)
{
  INT_DMEM *f = data->dataI + rIdx*lcfg.N;
  INT_DMEM *end = f+lcfg.N;
  for ( ; f < end; f++) {
    *f = *_data;
    _data++;
  }
}

void cDataMemoryLevel::frameRd(long rIdx, FLOAT_DMEM *_data) const
{
  FLOAT_DMEM *f = data->dataF + rIdx*lcfg.N;
  FLOAT_DMEM *end = f+lcfg.N;
  for ( ; f < end; f++) {
    *_data = *f;
    _data++;
  }
}

void cDataMemoryLevel::frameRd(long rIdx, INT_DMEM *_data) const
{
  INT_DMEM *f = data->dataI + rIdx*lcfg.N;
  INT_DMEM *end = f+lcfg.N;
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
          const char * typeStr = "unknown";
          if (lcfg.type==DMEM_FLOAT) { typeStr="float"; }
          else if (lcfg.type==DMEM_INT) { typeStr="int"; }
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
  data = new cMatrix(lcfg.N,lcfg.nT,lcfg.type,lcfg.noTimeMeta);
  if (data==NULL) COMP_ERR("cannot allocate level of unknown type %i or out of memory!",lcfg.type);
  
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


int cDataMemoryLevel::setFrame(long vIdx, const cVector *vec, int special)  // id must already be resolved...!
{
  if (!lcfg.finalised) { COMP_ERR("cannot set frame in non-finalised level! call finalise() first!"); }
  if (vec == NULL) {
    SMILE_ERR(3,"cannot set frame in dataMemory from a NULL cVector object!");
    return 0;
  }

  if (lcfg.N != vec->N) { COMP_ERR("setFrame: cannot set frame in level '%s', framesize mismatch: %i != %i (expected)",getName(),vec->N,lcfg.N); }
  if (lcfg.type != vec->type) { COMP_ERR("setFrame: frame type mismtach between frame and level (frame=%i, level=%i)",vec->type,lcfg.type); }

//****** acquire write lock.... *******
  smileMutexLock(RWstatMtx);
  // set write request flag, incase the level is currently locked for reading
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
    if (lcfg.type == DMEM_FLOAT) frameWr(rIdx, vec->dataF);
    else if (lcfg.type == DMEM_INT)   frameWr(rIdx, vec->dataI);
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
  if (lcfg.type != mat->type) { COMP_ERR("setMatrix: frame type mismtach between frame and level (frame=%i, level=%i)",mat->type,lcfg.type); }

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

    if (lcfg.type == DMEM_FLOAT) for (i=0; i<mat->nT; i++) { 
      //if ((mat->tmeta + i)->smileTime == -1.0 ) (mat->tmeta + i)->smileTime = smileTm;
      frameWr((rIdx+i)%lcfg.nT, mat->dataF + i*lcfg.N);
      if (!lcfg.noTimeMeta) {
        setTimeMeta((rIdx+i)%lcfg.nT,vIdx+i,mat->tmeta + i); 
      }
    }
    else if (lcfg.type == DMEM_INT) for (i=0; i<mat->nT; i++) { 
      //(mat->tmeta + i)->smileTime = -1.0;
      //if ((mat->tmeta + i)->smileTime == -1.0 ) (mat->tmeta + i)->smileTime = smileTm;
      frameWr((rIdx+i)%lcfg.nT, mat->dataI + i*lcfg.N);
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
//    usleep(1000);
    smileYield();
    smileMutexLock(RWstatMtx);
  }
  if (nCurRdr == 0) {
    nCurRdr++;
    smileMutexUnlock(RWstatMtx);
    smileMutexLock(RWmtx); // no other readers, so lock mutex to exclude writes...
    smileMutexLock(RWstatMtx);
  } else {
    nCurRdr++;
  }
//      nCurRdr++;
  smileMutexUnlock(RWstatMtx);
//****************

  smileMutexLock(RWptrMtx);
  long rIdx = validateIdxR(&vIdx,special,rdId);
  smileMutexUnlock(RWptrMtx);

  cVector *vec=NULL;
  if (rIdx>=0) {
    vec = new cVector(lcfg.N,lcfg.type);
    if (vec == NULL) OUT_OF_MEMORY;
    if (lcfg.type == DMEM_FLOAT) frameRd(rIdx, vec->dataF);
    else if (lcfg.type == DMEM_INT) frameRd(rIdx, vec->dataI);
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
    nCurRdr++;
    smileMutexUnlock(RWstatMtx);
    smileMutexLock(RWmtx); // no other readers, so lock mutex to exclude writes...
    smileMutexLock(RWstatMtx);
  } else {
    nCurRdr++;
  }
  smileMutexUnlock(RWstatMtx);
//****************

  smileMutexLock(RWptrMtx);
  // TODO : if EOI state, then allow vIdxEnd out of range! pad frame...
  long rIdx = validateIdxRangeR(vIdxold, &vIdx, vIdxEnd, special, rdId, 0, &padEnd);
  smileMutexUnlock(RWptrMtx);

  cMatrix *mat=NULL;
  if (rIdx>=0) {
    if (vIdxold < 0) mat = new cMatrix(lcfg.N,vIdxEnd-vIdxold,lcfg.type);
    else mat = new cMatrix(lcfg.N,vIdxEnd-vIdx,lcfg.type);
    SMILE_DBG(4,"creating new data matrix (%s)  vIdxold=%i , vIdx=%i, vIdxEnd=%i, lcfg.N=%i",this->getName(),vIdxold,vIdx,vIdxEnd,lcfg.N)
    if (mat == NULL) OUT_OF_MEMORY;
    long i,j;
    if (vIdxold < 0) {
      long i0 = 0-vIdxold;
      if (lcfg.type == DMEM_FLOAT) {
        for (i=0; i<i0; i++) {
          if (special == DMEM_PAD_ZERO) for (j=0; j<mat->N; j++) mat->dataF[i*lcfg.N+j] = 0.0; // pad with value
          else frameRd((rIdx)%lcfg.nT, mat->dataF + (i*lcfg.N)); // fill with first frame
          getTimeMeta((rIdx)%lcfg.nT, vIdx + i, mat->tmeta + i);
        }
        for (i=0; i<vIdxEnd; i++) { 
          frameRd((rIdx+i)%lcfg.nT, mat->dataF + (i+i0)*lcfg.N);
          getTimeMeta((rIdx+i)%lcfg.nT, vIdx + i + i0, mat->tmeta + i +i0);
        }
      } else if (lcfg.type == DMEM_INT) {
        for (i=0; i<i0; i++) {
          if (special == DMEM_PAD_ZERO) for (j=0; j<mat->N; j++) mat->dataI[i*lcfg.N+j] = 0; // pad with value
          else frameRd((rIdx)%lcfg.nT, mat->dataI + (i*lcfg.N)); // fill with first frame
          getTimeMeta((rIdx)%lcfg.nT, vIdx + i, mat->tmeta + i);
        }
        for (i=0; i<vIdxEnd; i++) {
          frameRd((rIdx+i)%lcfg.nT, mat->dataI + (i+i0)*lcfg.N);
          getTimeMeta((rIdx+i)%lcfg.nT, vIdx + i + i0, mat->tmeta + i +i0);
        }
      }
    } else if (padEnd>0) {
      if (lcfg.type == DMEM_FLOAT) {
        for (i=0; i<(vIdxEnd-vIdx)-padEnd; i++) {
          frameRd((rIdx+i)%lcfg.nT, mat->dataF + (i*lcfg.N));
          getTimeMeta((rIdx+i)%lcfg.nT, vIdx + i, mat->tmeta + i);
        }
        long i0 = i-1;
        for (; i<(vIdxEnd-vIdx); i++) {
          if (special == DMEM_PAD_ZERO) for (j=0; j<mat->N; j++) mat->dataF[i*lcfg.N+j] = 0.0; // pad with value
          else frameRd((rIdx+i0)%lcfg.nT, mat->dataF + (i*lcfg.N)); // fill with last frame
          getTimeMeta((rIdx+i0)%lcfg.nT, vIdx + i, mat->tmeta + i);
        }
        // TODO: Test DMEM_PAD_NONE option to truncate the frame!!
        if (special == DMEM_PAD_NONE) {
          mat->nT = (vIdxEnd-vIdx)-padEnd;
        }
      } else if (lcfg.type == DMEM_INT) {
        for (i=0; i<(vIdxEnd-vIdx)-padEnd; i++) {
          frameRd((rIdx+i)%lcfg.nT, mat->dataI + (i*lcfg.N));
          getTimeMeta((rIdx+i)%lcfg.nT, vIdx + i, mat->tmeta + i);
        }
        long i0 = i-1;
        for (; i<(vIdxEnd-vIdx); i++) {
          if (special == DMEM_PAD_ZERO) for (j=0; j<mat->N; j++) mat->dataI[i*lcfg.N+j] = 0; // pad with value
          else frameRd((rIdx+i0)%lcfg.nT, mat->dataI + (i*lcfg.N)); // fill with last frame
          getTimeMeta((rIdx+i0)%lcfg.nT, vIdx + i, mat->tmeta + i);
        }
        // TODO: Test DMEM_PAD_NONE option to truncate the frame!!
        if (special == DMEM_PAD_NONE) {
          mat->nT = (vIdxEnd-vIdx)-padEnd;
        }
      }

    } else {
      if (lcfg.type == DMEM_FLOAT) {
        for (i=0; i<mat->nT; i++) {
          frameRd((rIdx+i)%lcfg.nT, mat->dataF + i*lcfg.N);
          getTimeMeta((rIdx+i)%lcfg.nT, vIdx + i, mat->tmeta + i);
        }
      }
      else if (lcfg.type == DMEM_INT) {
        for (i=0; i<mat->nT; i++) {
          frameRd((rIdx+i)%lcfg.nT, mat->dataI + i*lcfg.N);
          getTimeMeta((rIdx+i)%lcfg.nT, vIdx + i, mat->tmeta + i);
        }
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

/***************************** dataMemory ***********************************************/

void cDataMemory::_addLevel()
{
  nLevels++;
  if (nLevels>=nLevelsAlloc) {
    int lN = nLevels+LOOKAHEAD_ALLOC;
    cDataMemoryLevel **l = (cDataMemoryLevel **)realloc(level,sizeof(cDataMemoryLevel *)*lN);
    if (l==NULL) OUT_OF_MEMORY;
    // initialize newly allocated memory with NULL
    int i;
    for (i=nLevels; i<lN; i++) l[i] = NULL;
    level = l;
    nLevelsAlloc=lN;
  }
}

int cDataMemory::registerLevel(cDataMemoryLevel *l)
{
  if (l!=NULL) {
    _addLevel();
    level[nLevels] = l;
    return nLevels;
  } else { SMILE_WRN(1,"attempt to register NULL level with dataMemory!"); return -1; }
}

// get element id of level 'name', return -1 on failure (e.g. level not found)
int cDataMemory::findLevel(const char *name) const
{
  int i;
  if (name==NULL) return -1;
  
  for (i=0; i<=nLevels; i++) {
    if(!strcmp(name,level[i]->getName())) return i;
  }
  return -1;
}

void cDataMemory::registerReadRequest(const char *lvl, const char *componentInstName)
{
  if (lvl == NULL) return;

  // do nothing if this request was already registered!
  for (const auto &rq : rrq) {
    if (!strcmp(lvl,rq.levelName) && (componentInstName==NULL || !strcmp(componentInstName,rq.instanceName)))
      return;
  }

  // add request to list
  rrq.emplace_back(componentInstName, lvl);

  SMILE_DBG(2,"registerReadRequest: registered read request for level '%s' by component instance '%s'",lvl,componentInstName);
}

void cDataMemory::registerWriteRequest(const char *lvl, const char *componentInstName)
{
  if (lvl == NULL) return;

  // check if the same component attempts to register twice (which is ok), or if two components want to write to the same level!!
  for (const auto &rq : wrq) {
    if (!strcmp(lvl,rq.levelName)) {
      if (!strcmp(rq.instanceName,componentInstName)) {
        return;  // ignore duplicate request from the same component
      } else {
        // it's likely another component wanting to write to the same level...
        COMP_ERR("two components cannot write to the same level: '%s', component1='%s', component2='%s'",lvl,rq.instanceName,componentInstName);
      }
    }
  }

  // add request to list
  wrq.emplace_back(componentInstName, lvl);

  SMILE_DBG(2,"registerWriteRequest: registered write request for level '%s' by component instance '%s'",lvl,componentInstName);
}

/* add a new level with given _lcfg parameters */
int cDataMemory::addLevel(sDmLevelConfig *_lcfg, const char *_name)
{
  if (_lcfg == NULL) return 0;

  if (_name != NULL) {
    _lcfg->setName(_name);
  }

  cDataMemoryLevel *l = new cDataMemoryLevel(-1, *_lcfg );
  if (l==NULL) OUT_OF_MEMORY;
  l->setParent( (cDataMemory*)this );
  return registerLevel(l);
}

int cDataMemory::myRegisterInstance(int *runMe)
{
  // check if a write request exists for each level where there exists a read request
  int err=0;
  for (const auto &rq : rrq) {
    auto result = std::find_if(wrq.begin(), wrq.end(), [rq](const sDmLevelRWRequest &rq2) {
      return !strcmp(rq.levelName,rq2.levelName);
    });
    if (result == wrq.end()) {
      SMILE_ERR(1,"level '%s' was not found! component '%s' requires it for reading.\n     it seems that no dataWriter has registered this level! check your config!",rq.levelName,rq.instanceName);
      err=1;
    }
  }

  if (err) { COMP_ERR("there were unresolved read-requests, please check your config file for missing components / missing dataWriters or incorrect level names (typos, etc.)!"); }
  if (runMe != NULL) *runMe = 0;
  return !err;
}

int cDataMemory::myConfigureInstance() 
{
  // check blocksizeconfig and update buffersizes of levels accordingly
  if (nLevels>=0) {
    int i;
    for (i=0; i<=nLevels; i++) {
      // do this for each level...
      SMILE_IDBG(3,"configuring level %i ('%s') (checking buffersize and config)",i,level[i]->getName());
      int ret = level[i]->configureLevel();
      if (!ret) {
        SMILE_IERR(1,"level '%s' could not be configured!");
        return 0;
      }
    }
  } else {
    SMILE_ERR(1,"it makes no sense to configure a dataMemory without levels! cannot configure dataMemory '%s'!",getInstName());
    return 0;
  }

  return 1;
}

int cDataMemory::myFinaliseInstance()
{
  // now finalise the levels (allocate storage memory and finalise config):
  if (nLevels>=0) {
    int i;
    for (i=0; i<=nLevels; i++) {
      // actually finalise now
      SMILE_DBG(3,"finalising level %i (allocating buffer, etc.)",i);
      int ret = level[i]->finaliseLevel();
      if (!ret) {
        SMILE_IERR(1,"level '%s' could not be finalised!");
        return 0;
      }
    }
    // allocate reader config array
    SMILE_DBG(4,"allocating reader positions in %i level(s)",nLevels+1);
    for (i=0; i<=nLevels; i++) {
      level[i]->allocReaders();
    }
  } else {
    SMILE_ERR(1,"it makes no sense to finalise a dataMemory without levels! cannot finalise dataMemory '%s'!",getInstName());
    return 0;
  }
  return 1;
}

cDataMemory::~cDataMemory() {
  int i;
  if (level != NULL) {
    for (i=0; i<nLevelsAlloc; i++) {
      if(level[i] != NULL) delete level[i];
    }
    free(level);
  }
}

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

void datamemoryLogger(const char *name, const char*dir, long cnt, long vIdx0, long vIdx, long rIdx, long nT, int special, long curR, long curW, long EOI, int nReaders, long *curRr, cVector *vec)
{
#ifdef DM_DEBUG_LOGGER
  long i;
  fprintf(stderr,"xxDMemLOGxx:: level:%s dir:%s cnt:%i vIdx0:%i vIdx:%i rIdx:%i nT:%i special:%i EOI:%i curR:%i curW:%i :: nReaders:%i ",name,dir,cnt,vIdx0,vIdx,rIdx,nT,special,EOI,curR,curW,nReaders);
  for (i=0; i<nReaders; i++) {
    fprintf(stderr,"curRr%i:%i ",i,curRr[i]);
  }
  fprintf(stderr,"::: DATA(%i) ::: ",vec->N);
  for (i=0; i<vec->N; i++) {
    fprintf(stderr,"f%i:%i ",i,vec->dataF[i]);
  }
  fprintf(stderr,"::END\n");
#endif
}

void datamemoryLogger(const char *name, long vIdx0, long vIdx, long rIdx, long nT, int special, long curR, long curW, long EOI, int nReaders, long *curRr, cMatrix *mat)
{
#ifdef DM_DEBUG_LOGGER
  long i,j;
  fprintf(stderr,"xxDMemLOGxx:: level:%s dir:%s cnt:%i vIdx0:%i vIdx:%i rIdx:%i nT:%i special:%i EOI:%i curR:%i curW:%i :: nReaders:%i ",name,dir,cnt,vIdx0,vIdx,rIdx,nT,special,EOI,curR,curW,nReaders);
  for (i=0; i<nReaders; i++) {
    fprintf(stderr,"curRr%i:%i ",i,curRr[i]);
  }
  fprintf(stderr,"::: DATA[%ix%i] ::: ",vec->N,vec->nT); // row x column
  for (j=0; j<vec->nT; j++) {
    for (i=0; i<vec->N; i++) {
      fprintf(stderr,"f%i,%i:%i ",i,j,vec->dataF[i+j*vec->N]);  // row, column
    }
    if (j<vec->nT-1) fprintf(stderr,":: ");
  }
  fprintf(stderr,"::END\n");
#endif
}
