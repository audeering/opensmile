/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

compute gliding mean of input vectors and subtract it from the vectors
(used for cepstral mean subtraction (CMS), for example)

*/


#ifndef __CVECTORTRANSFORM_HPP
#define __CVECTORTRANSFORM_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>
#include <math.h>

#define COMPONENT_DESCRIPTION_CVECTORTRANSFORM "this is a base class for vector transforms which require history data or precomputed data (e.g. cepstral mean subtraction, etc.)"
#define COMPONENT_NAME_CVECTORTRANSFORM "cVectorTransform"

/* available mean update methods */
#define UPDMETHOD_NUL   0  /* only inital values */
#define UPDMETHOD_EXP   1  /* exponentially decaying mean */
#define UPDMETHOD_FIX   2  /* fixed buffer */
#define UPDMETHOD_AVG   3  /* weighted sum: incremental average and w. inital mean */
#define UPDMETHOD_AVGI  4  /* weighted sum: incremental average and w. inital mean (incl. mean update) */
#define UPDMETHOD_FIXI   6  /* fixed buffer */
#define UPDMETHOD_USR   100  /* user defined, set by child class */

/* operation modes */
#define MODE_ANALYSIS       1
#define MODE_TRANSFORMATION 2
#define MODE_INCREMENTAL    0   /* transformation and analysis (incremental updates) */

/* header of transform data file (see .cpp for more info on file formats) */
/*
C) advanced smile transformation format: Binary
   (note: this format is used interally)
  Header: 
    Magic     (int32)  0xEE 0x11 0x11 0x00
    N vectors (int32)
    N groups  (int32)
    N timeunits(int32)
    Vecsize   (int32)
    N userData(int32)
    TypeID    (int32)  [defined in the header file of the component implementing the transform]
    Reserved  (16 byte)
  User data : N userData (double)
  Matrix data: N vec x Vecsize (double, 8 byte)
*/
struct sTfHeader {
  unsigned char magic[4];
  int32_t nVec;
  int32_t nGroups;
  int64_t nTimeunits;  // e.g. for cummulative averages: number of values, etc.
  int32_t vecSize;
  int32_t nUserdata;  /* additional data besides parameter vectors */
  int32_t typeID; /* ID describing data loaded, e.g. MVN data, HEQ data, CMS, ... */
  unsigned char reserved[16]; /* reserved for future extension, should be zeroe'd */
  /* user data:  nUserdata x sizeof(double) */
  /* matrix data:  nVec x vecSize x sizeof(double) */
};

/* struct for storing transformation data */
struct sTfData {
  struct sTfHeader head;
  double * userData;  /* this can be NULL if no user data is present, i.e. when nUserdata = 0 */
  double * vectors;   /* this can be NULL if only userData is present and no vectors (nVec = 0) */
  /* NOTE: at least one of the two above pointers must be != NULL */
  void * user;  /* optional workspace data, if required by child classes */
};

const unsigned char smileMagic[] = {(unsigned char)0xEE,
    (unsigned char)0x11, (unsigned char)0x11, (unsigned char)0x00};

/* we define some transform type IDs here, other will be defined in child classes */
#define TRFTYPE_MNN     10    /* mean normalisation, mean vector only */
#define TRFTYPE_MVN     20    /* mean variance normalisation, mean vector + stddev vector */
#define TRFTYPE_MRN     21    /* mean and range normalisation, mean vector + min & max vector */
#define TRFTYPE_UNDEF    0    /* undefined, un-initialized */
#define TRFTYPE_USR      1    /* user-defined, reserved */

// #define TRFTYPE_HEQ   101  // --> defined in vectorHEQ.hpp

class cVectorTransform : public cVectorProcessor {
  private:
    const char * initFile;
    char * saveFile_;
    int saveFileInterval_;
    int saveFileCounter_;
    int err, flushed;

    int invertMVNdata;
    int zeroThIsLast; // flag that indicates whether last coeff in initFile is loaded into means[0]
    int skipNfirst;

    double fixedBuffer;
    double updateMaxSec_;
    long updateMaxFrames_;
    
    const char * turnStartMessage;  // name of turnStartMessage
    const char * turnEndMessage;    // name of turnEndMessage

    struct sTfData transform0;   // initial transform as loaded from file
    struct sTfData transform;    // current (computed/updated) transform

    int loadHTKCMNdata(const char *filename, struct sTfData  * tf);
    int loadMVNtextdata(const char *filename, struct sTfData  * tf);
    int loadMVNdata(const char *filename, struct sTfData  * tf);
    int loadSMILEtfData(const char *filename, struct sTfData  * tf);

    void checkDstFinite(FLOAT_DMEM *dst, long N) { // check for nan/inf in output
      int ret = checkVectorFinite(dst,N);
      if (ret==0) SMILE_IERR(2,"Non-finite #INF# or #NAN# value encountered in output (This value is now automatically set to 0, but somewhere there is a bug)!");
    }

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    int updateMethod;
    int mode;

    FLOAT_DMEM alpha, weight;
    long fixedBufferFrames;
    long nFrames;

    FLOAT_DMEM * buffer; /* ring-buffer for sums or values (depending on update method) */
    long *bufferNframes; /* ring-buffer of number of frames (update method 'avg'), same size as buffer, same r/w pointers */
    long nAvgBuffer; /* currently present number of elements in above ring buffer */
    long wPtr, rPtr; /* write and read pointers, real indicies */

    int turnOnlyNormalise, turnOnlyOutput;
    int invertTurn;
    int resetOnTurn;
    int turnOnlyUpdate;
    
    int isTurn, resetMeans;

    void prepareUnstandardise(struct sTfData * tf);

    /* loads data, the format (HTK, old MVN, smile native) is determined automatically */
    int loadTransformData(const char * filename, struct sTfData * tf);
    
    /* saves transform data in smile native format,
       other formats are not supported and will not be supported */
    int saveTransformData(const char * filename, struct sTfData * tf); 

    /* free the vectors and user data */
    void freeTransformData(struct sTfData * tf); 
    
    /* returns a pointer to the internal (private) transform data struct */
    struct sTfData * getTransformData() { return &transform; }

    /* returns a pointer to the internal (private) initial transform data struct */
    struct sTfData * getTransformInitData() { return &transform0; }

    virtual void initTransform(struct sTfData *tf, struct sTfData *tf0);

    /**** refactoring hooks ****/
    /* to modify or validate the initial transform directly aafter it has been loaded from a file,
       overwrite this method */
    virtual void modifyInitTransform(struct sTfData *tf0) {}
    /* allocate the memory for vectors and userData, initialise header
       the only field that has been set is head.vecSize ! 
       This function is always called and should always allocate memory, even if data was loaded */
    virtual void allocTransformData(struct sTfData *tf, int Ndst, int idxi) = 0;
    /* For mode == ANALYSIS or TRANSFORMATION, this functions allows for a final processing step
       at the end of input and just before the transformation data is saved to file */
    virtual void computeFinalTransformData(struct sTfData *tf, int idxi) {}
    /* this will be called BEFORE the transform will be reset to initial values (at turn beginning/end) 
       you may modify the initial values here or the new values, 
       if you return 1 then no further changes to tf will be done,
       if you return 0 then tf0 will be copied to tf after running this function */
    virtual int transformResetNotify(struct sTfData *tf, struct sTfData *tf0) { return 0; }

    /* this method is called after a transform (re-)init. Data from tf0 (if available) has already 
       been copied to tf once this function is called */
    virtual void transformInitDone(struct sTfData *tf, struct sTfData *tf0) {  }

    /* you may override this, if you want to modify the management of the ringbuffer! */
    virtual void updateRingBuffer(const FLOAT_DMEM *src, long Nsrc);

    /* Do the actual transformation (do NOT use this to calculate parameters!) 
       This function will only be called if not in ANALYSIS mode 
       Please return the number of output samples (0, if you haven't produced output) */
    virtual int transformData(const struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) = 0;

    /* Update transform parameters incrementally
       This function will only be called if not in TRANSFORMATIONs mode 
       *buf is a pointer to a buffer if updateMethod is fixedBuffer */
    // return value: 0: no update was performed , 1: successful update
    virtual int updateTransformExp(struct sTfData * tf, const FLOAT_DMEM *src, int idxi) { return 0; }
    virtual int updateTransformBuf(struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *buf, long Nbuf, long wrPtr, int idxi) { return 0; }
    virtual int updateTransformAvg(struct sTfData * tf, const FLOAT_DMEM *src, int idxi) { return 0; }
    virtual int updateTransformAvgI(struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *buf, long * _bufferNframes, long Nbuf, long wrPtr, int idxi) { return 0; }

    /* generic method, default version will select one of the above methods,
       overwrite to implement your own update strategy ('usr' option) */
    virtual int updateTransform(struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *buf, long * _bufferNframes, long Nbuf, long wrPtr, int idxi);

/////////////////////////////////////////////
    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    //virtual eTickResult myTick(long long t) override;

    virtual int configureWriter(sDmLevelConfig &c) override;

    virtual int processComponentMessage(cComponentMessage *_msg) override;
    //virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;
    virtual int flushVector(FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cVectorTransform(const char *_name);

    virtual ~cVectorTransform();
};




#endif // __CVECTORTRANSFORM_HPP
