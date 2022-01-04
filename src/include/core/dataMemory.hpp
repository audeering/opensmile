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
#include <core/dataMemoryLevel.hpp>
#include <memory>
#include <vector>

// represents a request by a component instance to read or write from/to a level
struct sDmLevelRWRequest {
  const char *instanceName;
  const char *levelName;

  sDmLevelRWRequest(const char *_instanceName, const char *_levelName) :
    instanceName(_instanceName), levelName(_levelName) {}
};

#define COMPONENT_DESCRIPTION_CDATAMEMORY "central data memory component"
#define COMPONENT_NAME_CDATAMEMORY "cDataMemory"

class cDataMemory : public cSmileComponent {
  private:
    std::vector<std::unique_ptr<cDataMemoryLevel>> levels;

    std::vector<sDmLevelRWRequest> rrq;  // read requests of component instances to levels
    std::vector<sDmLevelRWRequest> wrq;  // write requests of component instances to levels

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override {}
    virtual int myRegisterInstance(int *runMe=NULL) override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override { return TICK_INACTIVE; }
    
  public:
    SMILECOMPONENT_STATIC_DECL

    cDataMemory();
    cDataMemory(const char *_name);

    /* register a read request (during "register" phase) */
    void registerReadRequest(const char *lvl, const char *componentInstName=NULL);
    void registerWriteRequest(const char *lvl, const char *componentInstName=NULL);

    /* register a new level, and check for uniqueness of name */
    int registerLevel(cDataMemoryLevel *l);

    /* add a new level with given _lcfg parameters */
    int addLevel(sDmLevelConfig *_lcfg, const char *_name=NULL);

    /* set end-of-input flag */
    virtual void setEOI() override;
    virtual void unsetEOI() override;
    virtual int setEOIcounter(int cnt) override;

    // get index of level 'name'
    int findLevel(const char *name) const;

    // get number of levels
    int getNlevels() const { return levels.size(); }

    /**** functions which will be forwarded to the corresponding level *****/

    /* check if level is ready for reading, get level config (WITHOUT NAMES!) and update blocksizeReader */
    const sDmLevelConfig * queryReadConfig(int llevel, long blocksizeReader = 1);
    const sDmLevelConfig * queryReadConfig(int llevel, double blocksizeReaderSeconds = 1.0);

    // adds a field to level <level>, _N is the size of an array field, set to 0 or 1 for scalar field
    // the name must be unique per level...
    int addField(int llevel, const char *lname, int lN, int arrNameOffset=0);

    // set info for field i (dataType and custom data pointer)
    int setFieldInfo(int _level, int i, int _dataType, void *_info, long _infoSize);
    
    // set info for field 'name' (dataType and custom data pointer). Do NOT include array indicies in field name!
    int setFieldInfo(int _level, const char *_name, int _dataType, void *_info, long _infoSize);

    // registers a reader for level _level, returns the reader id assigned to the new reader
    int registerReader(int _level);

    int setFrame(int _level, long vIdx, const cVector *vec, int special=-1/*, int *result=NULL*/);
    int setMatrix(int _level, long vIdx, const cMatrix *mat, int special=-1/*, int *result=NULL*/);

    /* check if a read will succeed */
    int checkRead(int _level, long vIdx, int special=-1, int rdId=-1, long len=1, int *result=NULL);

    // the memory pointed to by the return value must be freed via delete() by the calling code!!
    // TODO: optimise this... don't allocate a new frame everytime something is returned!! maybe provide an option
    //       for passing a buffer frame or so...
    cVector * getFrame(int _level, long vIdx, int special=-1, int rdId=-1, int *result=NULL);
    cMatrix * getMatrix(int _level, long vIdx, long vIdxEnd, int special=-1, int rdId=-1, int *result=NULL);

    // set current read index to current write index to prevent hangs, if the readers do not read data sequentially, or if the readers skip data
    void catchupCurR(int _level, int rdId=-1, long _curR=-1 /* if >= 0, value that curR[rdId] will be set to! */ );

    // methods to get info about current level fill status
    long getCurW(int _level) const; // current write index
    long getCurR(int _level, int rdId=-1) const;  // current read index
    long getNFree(int _level,int rdId=-1) const; // get number of free frames in current level (rb: nT-(curW-curR))
    long getNAvail(int _level,int rdId=-1) const; // get number of available frames in current level (curW-curR)
    long getMaxR(int _level) const;  // maximum readable index (index where data was written to) or -1 if level is empty
    long getMinR(int _level) const;  // minimum readable index or -1 if level is empty (relevant only for ringbuffers, otherwise it will always return -1 or 0)
    long getNreaders(int _level) const;  // number of registered readers

    int namesAreSet(int _level) const;
    void fixateLevel(int _level);

    const sDmLevelConfig * getLevelConfig(int _level) const;  
    const char * getLevelName(int _level) const;
    const char * getFieldName(int _level, int n, int *lN=NULL, int *_arrNameOffset=NULL) const;     
    char * getElementName(int _level, int n) const; // caller must free() returned string!!
    cVectorMeta * getLevelMetaDataPtr(int _level);

 	  void setArrNameOffset(int _level, int arrNameOffset);
    void setFrameSizeSec(int _level, double fss);
    void setBlocksizeWriter(int _level, long _bsw);

    /* print an overview over registered levels and their configuration */
    void printDmLevelStats(int detail=1);

    long secToVidx(int _level, double sec) const; // returns a vIdx
    double vIdxToSec(int _level, long vIdx) const; // returns a time in seconds as double
    
    virtual ~cDataMemory() {}
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
