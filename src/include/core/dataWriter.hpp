/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: dataWriter */

#ifndef __DATA_WRITER_HPP
#define __DATA_WRITER_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <core/dataMemory.hpp>

#define COMPONENT_DESCRIPTION_CDATAWRITER  "This is the dataMemory interface component that writes vector or matrix data to a dataMemory level. A writer can write only to a single level in the dataMemory, this level cannot be changed during the run-time phase."
#define COMPONENT_NAME_CDATAWRITER  "cDataWriter"

class cDataWriter : public cSmileComponent {
  private:
    cDataMemory * dm;
    const char *dmInstName;
    const char *dmLevel;
    int level;
    //int namesSet;
    sDmLevelConfig cfg;
    int override_nT;
    //long blocksize;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance(int isRb, long nT, double period, double lenSec, int dyn, int _type) override;
    virtual int myRegisterInstance(int *runMe=NULL) override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override { return TICK_SUCCESS; } // tick is not implemented for the readers and writers

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cDataWriter(const char *_name);
    
    void setBuffersize(long nT); // assume override = 1 when calling this method to set the Blocksize!
    long getBuffersize() { return cfg.nT; }
/*
    void setConfig(int isRb, long nT, double period, double lenSec, int dyn, int _type, int _override=1);
    void setConfig(int isRb, long nT, double period, double lenSec, double frameSizeSec, int dyn, int _type, int _override=1);
    */
    void setConfig(sDmLevelConfig &_cfg, int _override=1);

    /* data writer configuration process:
            # = called from parent's configureInstance
      #  configureInstance(...) to set the level timing and size parameters.
      #  addField(...) to add fields and thus set the vec. size in this level
      finaliseInstance() to finalise config.
    */
    int addField(const char * lname, int lN = 1, int arrNameOffset=0);
    
    /* set field info (by field index), dataType and custom data record. index -1 refers to the last added field */
    int setFieldInfo(int i, int _dataType, void *_info=NULL, long _infoSize=0) { // special value for i : -1 , last field that was added (N-1)
      if (i==-1) {
        /*const sDmLevelConfig *c = dm->getLevelConfig(level);
        if (c != NULL) {
          const FrameMetaInfo * fmeta = c->fmeta;*/
          const FrameMetaInfo * fmeta = getFrameMetaInfo();
          if (fmeta == NULL) {
            SMILE_IERR(2,"cannot read FrameMetaInfo from write level!\n");
            i=-1; // was: i==-1 ??
          } else {
            i = fmeta->N-1;
          }
        //}
      }
      return dm->setFieldInfo(level,i,_dataType, _info, _infoSize);
    }

    /* set field info (by field name), dataType and custom data record. */
    int setFieldInfo(const char* __name, int _dataType, void *_info=NULL, long _infoSize=0) {
      return dm->setFieldInfo(level,__name,_dataType, _info, _infoSize);
    }

    /* set field info (by field name), custom data record only, dataType is not changed. */
    int setFieldInfo(const char* __name, void *_info, long _infoSize) {
      return setFieldInfo(__name,0,_info,_infoSize);
    }

    /* set field info (by field index), custom data record only, dataType is not changed. index -1 refers to the last added field */
    int setFieldInfo(int i, void *_info, long _infoSize) {
      return setFieldInfo(i,0,_info,_infoSize);
    }

	  void setArrNameOffset(int arrNameOffset);
	
    // get pointer to meta data of the write level
    cVectorMeta * getLevelMetaDataPtr() {
      return dm->getLevelMetaDataPtr(level);
    }

    //void setBlocksize(long _bs) { blocksize = _bs; }

// absolute position write functions:
    int setFrame(long vIdx, const cVector *vec, int special=-1);
    int setMatrix(long vIdx, const cMatrix *mat, int special=-1);
// sequential write functions: write next frame(s)
    int setNextFrame(const cVector *vec);  // append at end in dataMemory level
    int setNextMatrix(const cMatrix *mat);  // append at end in dataMemory (no overlap)
// TODO: meta-data handling...

    int checkWrite(long len);
    int getNAvail() { return dm->getNAvail(level); }
    int getNFree() { return dm->getNFree(level); }

    void setFrameSizeSec(double _fss) { dm->setFrameSizeSec(level,_fss); }
    void setBlocksize(long _bsw) { dm->setBlocksizeWriter(level,_bsw); }

    // number of elements (arrays expaned = frameSize)
    int getLevelN() { return cfg.N; }
    // number of fields (arrays not expanded ! i.e. this is not the vecSize!)
    int getLevelNf() { return cfg.Nf; }
    
    // frame period (or 0.0 for unperiodic)
    double getLevelT() { return cfg.T; }

    // names of fields, etc.
    const FrameMetaInfo * getFrameMetaInfo() { 
      //OLD: //return cfg.fmeta; 
      // NEW:
      const sDmLevelConfig *c = dm->getLevelConfig(level);
      if (c != NULL) {
        return c->fmeta;
      } else {
        return NULL;
      }
    }

    // full level config
    const sDmLevelConfig * getLevelConfig() { 
      return dm->getLevelConfig(level);
      //return &cfg; 
    }
    
    // name of field n, optionally returns number of elements in field in *_N
    const char * getFieldName(int n, int *lN = NULL)
    { 
      return dm->getFieldName(level, n, lN); 
    }

    // full name of element n (incl. array index, etc.)
    const char * getElementName(int n)  
      { return dm->getElementName(level,n); }

    const char * getLevelName() { return dmLevel; }
    
    cDataMemory * getDmObj() const { return dm; }

    virtual ~cDataWriter() {}
};




#endif // __DATA_WRITER_HPP
