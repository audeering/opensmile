/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#include <core/dataMemory.hpp>
#include <core/componentManager.hpp>
#include <algorithm>

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

cDataMemory::cDataMemory() : cSmileComponent("dataMemory") {}

cDataMemory::cDataMemory(const char *_name) : cSmileComponent(_name) {}

int cDataMemory::registerLevel(cDataMemoryLevel *l)
{
  if (l!=NULL) {
    levels.emplace_back(l);
    return levels.size() - 1;
  } else { SMILE_WRN(1,"attempt to register NULL level with dataMemory!"); return -1; }
}

// get element id of level 'name', return -1 on failure (e.g. level not found)
int cDataMemory::findLevel(const char *name) const
{
  int i;
  if (name==NULL) return -1;
  
  for (i=0; i<levels.size(); i++) {
    if(!strcmp(name,levels[i]->getName())) return i;
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

void cDataMemory::setEOI() {
  // set end-of-input for all levels
  for (auto &level : levels) level->setEOI();
  // set end-of-input in this component
  cSmileComponent::setEOI();
}

void cDataMemory::unsetEOI() {
  // set end-of-input for all levels
  for (auto &level : levels) level->unsetEOI();
  // set end-of-input in this component
  cSmileComponent::unsetEOI();
}

int cDataMemory::setEOIcounter(int cnt) {
  // set end-of-input for all levels
    for (auto &level : levels) level->setEOIcounter(cnt);
  // set end-of-input in this component
  return cSmileComponent::setEOIcounter(cnt);
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
  if (!levels.empty()) {
    int i;
    for (i=0; i<levels.size(); i++) {
      // do this for each level...
      SMILE_IDBG(3,"configuring level %i ('%s') (checking buffersize and config)",i,levels[i]->getName());
      int ret = levels[i]->configureLevel();
      if (!ret) {
        SMILE_IERR(1,"level '%s' could not be configured!", levels[i]->getName());
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
  if (!levels.empty()) {
    int i;
    for (i=0; i<levels.size(); i++) {
      // actually finalise now
      SMILE_DBG(3,"finalising level %i (allocating buffer, etc.)",i);
      int ret = levels[i]->finaliseLevel();
      if (!ret) {
        SMILE_IERR(1,"level '%s' could not be finalised!");
        return 0;
      }
    }
    // allocate reader config array
    SMILE_DBG(4,"allocating reader positions in %i level(s)",levels.size());
    for (i=0; i<levels.size(); i++) {
      levels[i]->allocReaders();
    }
  } else {
    SMILE_ERR(1,"it makes no sense to finalise a dataMemory without levels! cannot finalise dataMemory '%s'!",getInstName());
    return 0;
  }
  return 1;
}

const sDmLevelConfig * cDataMemory::queryReadConfig(int llevel, long blocksizeReader)
{
  if ((llevel>=0)&&(llevel<levels.size()))
    return levels[llevel]->queryReadConfig(blocksizeReader);
  else
    return NULL;
}

const sDmLevelConfig * cDataMemory::queryReadConfig(int llevel, double blocksizeReaderSeconds)
{
  if ((llevel>=0)&&(llevel<levels.size()))
    return levels[llevel]->queryReadConfig(blocksizeReaderSeconds);
  else
    return NULL;
}

int cDataMemory::addField(int llevel, const char *lname, int lN, int arrNameOffset)
{  
  if ((llevel>=0)&&(llevel<levels.size())) 
    return levels[llevel]->addField(lname, lN, arrNameOffset);
  else
    return 0;
}

int cDataMemory::setFieldInfo(int _level, int i, int _dataType, void *_info, long _infoSize) {
  if ((_level>=0)&&(_level<levels.size()))
    return levels[_level]->setFieldInfo(i,_dataType,_info,_infoSize);
  else
    return 0;
}

int cDataMemory::setFieldInfo(int _level, const char *_name, int _dataType, void *_info, long _infoSize) {
  if ((_level>=0)&&(_level<levels.size()))
    return levels[_level]->setFieldInfo(_name,_dataType,_info,_infoSize);
  else
    return 0;
}

int cDataMemory::registerReader(int _level) {
  if ((_level>=0)&&(_level<levels.size()))
    return levels[_level]->registerReader();
  else
    return -1;
}

int cDataMemory::setFrame(int _level, long vIdx, const cVector *vec, int special/*, int *result*/)
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->setFrame(vIdx,vec,special/*, result*/); else return 0; }
int cDataMemory::setMatrix(int _level, long vIdx, const cMatrix *mat, int special/*, int *result*/)
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->setMatrix(vIdx,mat,special/*, result*/); else return 0; }

int cDataMemory::checkRead(int _level, long vIdx, int special, int rdId, long len, int *result)
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->checkRead(vIdx,special,rdId,len,result); else return -1; }

cVector * cDataMemory::getFrame(int _level, long vIdx, int special, int rdId, int *result)
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getFrame(vIdx,special,rdId,result); else return NULL; }
cMatrix * cDataMemory::getMatrix(int _level, long vIdx, long vIdxEnd, int special, int rdId, int *result)
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getMatrix(vIdx,vIdxEnd,special,rdId,result); else return NULL; }

void cDataMemory::catchupCurR(int _level, int rdId, long _curR /* if >= 0, value that curR[rdId] will be set to! */ ) 
  { if ((_level>=0)&&(_level<levels.size())) levels[_level]->catchupCurR(rdId,_curR); }

long cDataMemory::getCurW(int _level) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getCurW(); else return -1; }
long cDataMemory::getCurR(int _level, int rdId) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getCurR(rdId); else return -1; }
long cDataMemory::getNFree(int _level,int rdId) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getNFree(rdId); else return -1; }
long cDataMemory::getNAvail(int _level,int rdId) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getNAvail(rdId); else return -1; }
long cDataMemory::getMaxR(int _level) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getMaxR(); else return -1; }
long cDataMemory::getMinR(int _level) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getMinR(); else return -1; }
long cDataMemory::getNreaders(int _level) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getNreaders(); else return -1; }

int cDataMemory::namesAreSet(int _level) const
  { if ((_level>=0)&&(_level<levels.size())) { return levels[_level]->namesAreSet(); } else return 0; }
void cDataMemory::fixateLevel(int _level) 
  { if ((_level>=0)&&(_level<levels.size())) levels[_level]->fixateLevel(); } 

const sDmLevelConfig * cDataMemory::getLevelConfig(int _level) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getConfig(); else return NULL; }    
const char * cDataMemory::getLevelName(int _level) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getName(); else return NULL; }
const char * cDataMemory::getFieldName(int _level, int n, int *lN, int *_arrNameOffset) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getFieldName(n, lN, _arrNameOffset); else return NULL; }      
char * cDataMemory::getElementName(int _level, int n) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getElementName(n); else return NULL; }
cVectorMeta * cDataMemory::getLevelMetaDataPtr(int _level)
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->getLevelMetaDataPtr(); else return NULL; }

void cDataMemory::setArrNameOffset(int _level, int arrNameOffset)
  { if ((_level>=0)&&(_level<levels.size())) levels[_level]->setArrNameOffset(arrNameOffset); }
void cDataMemory::setFrameSizeSec(int _level, double fss)
  { if ((_level>=0)&&(_level<levels.size())) levels[_level]->setFrameSizeSec(fss);  }
void cDataMemory::setBlocksizeWriter(int _level, long _bsw)
  { if ((_level>=0)&&(_level<levels.size())) levels[_level]->setBlocksizeWriter(_bsw);  }

void cDataMemory::printDmLevelStats(int detail)
  { for (int i=0; i<levels.size(); i++) levels[i]->printLevelStats(detail); }

long cDataMemory::secToVidx(int _level, double sec) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->secToVidx(sec); else return -1; }
double cDataMemory::vIdxToSec(int _level, long vIdx) const
  { if ((_level>=0)&&(_level<levels.size())) return levels[_level]->vIdxToSec(vIdx); else return -1; }

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
    fprintf(stderr,"f%i:%i ",i,vec->data[i]);
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
      fprintf(stderr,"f%i,%i:%i ",i,j,vec->data[i+j*vec->N]);  // row, column
    }
    if (j<vec->nT-1) fprintf(stderr,":: ");
  }
  fprintf(stderr,"::END\n");
#endif
}
