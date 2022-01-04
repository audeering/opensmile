/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: dataReader */



#ifndef __DATA_READER_HPP
#define __DATA_READER_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <core/dataMemory.hpp>

#define COMPONENT_DESCRIPTION_CDATAREADER  "This is the dataMemory interface component that reads data as vector or matrix from dataMemory component. It is used internally by all dataProcessor, dataSource, and dataSink components. A cDataReader can read from one or more data memory levels. In the latter case a single vector is returned which consists of all individual vectors concattenated. Reading from multiple levels implies waiting for data on the 'slowest' level, since only completely concattenated frames are read."
#define COMPONENT_NAME_CDATAREADER  "cDataReader"

class cDataReader : public cSmileComponent {
  private:
    
    cDataMemory * dm;
    const char *dmInstName;
    
    int nLevels; /* number of levels this reader is configured to read from */
    const char **dmLevel; /* array of level names of the levels this reader is configured to read from */
    int *level; /* mapping of level names to level indicies in the data memory */
    int *rdId; /* data memory assigned reader id's of the registered readers */
    
    //int dtype;
    
    /* various config parameters */
    int forceAsyncMerge;
    int errorOnFullInputIncomplete;

    // current frame TO read
    long curR; 
    
    // temporary vector...
    cVector *V_;
    // temporary matrix...
    cMatrix *m;

    /* reader parameters for sequential matrix reading */
    long stepM, lengthM;  /* parameters in frames */
    int ignMisBegM;
    double stepM_sec, lengthM_sec;  /* parameters in seconds */
    double ignMisBegM_sec;
    
    /* various mappings for multiple read levels */
    int *Lf,*Le;
    int *fToL;  // field->level map
    int *eToL;  // element->level map

    /* input level config */
    //long N,Nf;
    //double T;
    FrameMetaInfo *myfmeta;
    sDmLevelConfig *myLcfg; //??

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    virtual int myRegisterInstance(int *runMe=NULL) override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override { return TICK_SUCCESS; } // tick is not implemented for the readers and writers

  public:
    SMILECOMPONENT_STATIC_DECL

    cDataReader(const char *_name);


    //TODO:(partially)  internal frame and matrix pointer (auto free mechanism, if next frame is read)
    // privateVec=1 : allocates a new vector, which can and must be freed by the calling code (instead of returning a pointer to an internally allocated object, which will be only valid till the next getXXXX() call to the same dataReader
    // absolute
    cVector * getFrame(long vIdx, int special=-1, int privateVec=0, int *result=NULL);
    cMatrix * getMatrix(long vIdx, long length, int special=-1, int privateVec=0); // vIdx: start index of matrix (absolute)
    
    // relative:
    cVector * getFrameRel(long vIdxRelE, int privateVec=0, int noInc=0, int *result=NULL);
    cMatrix * getMatrixRel(long vIdxRelE, long length, int privateVec=0);  // vIdxRelE: end of matrix relative to end of data

    // sequential
    void nextFrame() { curR++; }  // only increase frame counter
    void nextMatrix() { curR += stepM; }  // only increase frame counter
    cVector * getNextFrame(int privateVec=0, int *result=NULL);
    cMatrix * getNextMatrix(int privateVec=0, int readToEnd=0, int special=-1);  // readToEnd == 1 will read all remaining input, even if stepM was not set to 0 or the current read pointer is no longer at vIdx 0

    void catchupCurR(long _curR=-1); // set curR in dataMemory to curW or to user defined value (for all input levels)

    /* set matrix reading parameters in FRAMES */
    int setupSequentialMatrixReading(long step, long length, long ignoreMissingBegin=0);
    /* set matrix reading parameters in SECONDS */
    int setupSequentialMatrixReading(double step, double length, double ignoreMissingBegin=0.0);

    // these two functions must be used *before* reader->configure() is called!
    /* set the required blocksize for reads (the default is 1, if you don't read blocks this is ok. You should use 
       setupSequentialMatrixReading() instead, if you want to use block reading (you don't need to use setBlocksize then!)
    */
    int setBlocksize(long length) { 
      if (isConfigured()) {
        lengthM = length;
        return updateBlocksize(length);
      }
      if (length >= 0) { lengthM = length; lengthM_sec = -1.0; return 1; }
      return 0;
    }
    /* same as above, however, takes time in seconds as parameter */
    int setBlocksizeSec(double length) { 
      if (isConfigured()) {
        if (myLcfg->T != 0.0) {
          lengthM = (long)ceil( length / myLcfg->T );
        } else {
          lengthM = (long)ceil( length );
        }
        return updateBlocksize((long)lengthM);
      }
      if (length >= 0.0) { lengthM_sec = length; lengthM = -1; return 1; }
      return 0;
    }

    // these updateBlocksize functions may be used *after* reader->configure, but *before* finalise()
    int updateBlocksize(long length) { 
      int i;
      for (i=0; i<nLevels; i++) {
        dm->queryReadConfig(level[i], length);
      }
      return 1;
    }

    /* same as above, however, takes time in seconds as parameter */
    int updateBlocksizeSec(double length) { 
      if (myLcfg != NULL) {
        long _bs = 1;
        if (myLcfg->T != 0.0) {
          _bs = (long)ceil( length / myLcfg->T );
        } else {
          _bs = (long)ceil( length );
        }
        updateBlocksize((long)_bs);
        return 1;
      }
      return 0;
    }

    // set the current read index (negative values are also allowed!)
    void setCurR(long _curR) { curR = _curR; }
    long getCurR() { return curR; }

    // get pointer to meta data of the read level, (levelIdx-th input level)
    // WARN: we cannot merge metadata from multiple levels, thus we just use the first by default (the caller can change this via _levelIdx). This may not be correct in all cases!
    cVectorMeta * getLevelMetaDataPtr(int _levelIdx=0) {
      if ((_levelIdx < nLevels)&&(_levelIdx>=0)) {
        return dm->getLevelMetaDataPtr(level[_levelIdx]); 
      } else {
        return NULL;
      }
    }

    // return number in input levels
    int getNLevels() { return nLevels; }

    // number of elements
    int getLevelN() {  return myLcfg->N; }
    // number of fields
    int getLevelNf() {  return myLcfg->Nf; }

    // frame period (or 0.0 for unperiodic)
    double getLevelT() { return myLcfg->T; }
    // frame size in seconds 
    double getFrameSizeSec() 
    {
       // XXX: TODO: check if frameSizeSec is unique for all inputs... else return min/max/mean ??
       return myLcfg->frameSizeSec;
    }

    // names of fields, etc.
    const FrameMetaInfo * getFrameMetaInfo() { return myfmeta; }

    // get full config of input level, get this config directly from the DM..
    const sDmLevelConfig * getLevelConfig() 
      { return dm->getLevelConfig(level[0]);
             // XXX TODO: merge level config from all input levels

      }

    // get full config of input level, return locally stored myLcfg
    // OBSOLETE, don't use
    const sDmLevelConfig * getConfig() { return myLcfg; }

    long getMinR();

    long getNFree();
    long getNAvail();
    
    int isNextMatrixReadOk() { 
      int r = 1; int i;
      int nL = nLevels;
      if (nL < 1) nL=1;
      for (i=0; i<nL; i++) {
        r &= dm->checkRead(level[i],curR,-1,rdId[i],lengthM);
      }
      return r;
    }

    int isNextFrameReadOk(int lag=0) { 
      int r = 1; int i;
      int nL = nLevels;
      if (nL < 1) nL=1;
      for (i=0; i<nL; i++) {
        r &= dm->checkRead(level[i],curR,-1,rdId[i]);
      }
      return r;
    }

    const char * getFieldName(int n, int *N=NULL, int *arrNameOffset=NULL)    // name of field n
    { 
        if ((n >= 0) && (n < myLcfg->Nf)) {
          return dm->getFieldName(level[fToL[n]], n - Lf[fToL[n]], N, arrNameOffset);
        } else { 
          return NULL; 
        }
    }
    
    char * getElementName(int n)    // name of element n , // caller must free() returned string!!
    { 
        if ((n>=0)&&(n<myLcfg->N)) {
          return dm->getElementName(level[eToL[n]],n-Le[eToL[n]]);
        } else { return NULL; }
    }

    std::string getLevelName(int i=-1) {
      if (i < -1 || i >= nLevels) {
        throw std::invalid_argument("index out of range");
      }
      if (i != -1) { 
        return dmLevel[i];
      } else {        
        // concatenate the names of all levels, separated by semicolons
        std::string names;
        for (int j = 0; j < nLevels; j++) {
          if (j != 0) {
            names += ";";
          }
          names += dmLevel[j];          
        }
        return names;
      }
    }
    
    const cDataMemory * getDmObj() const { return dm; }
    
    virtual ~cDataReader();
};




#endif // __DATA_READER_HPP

