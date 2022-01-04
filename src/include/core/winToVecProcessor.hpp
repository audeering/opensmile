/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

reads in windows and outputs vectors (frames)

*/


#ifndef __CWINTOVECPROCESSOR_HPP
#define __CWINTOVECPROCESSOR_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>

#define COMPONENT_DESCRIPTION_CWINTOVECPROCESSOR "reads input windows, outputs frame(s)"
#define COMPONENT_NAME_CWINTOVECPROCESSOR "cWinToVecProcessor"

#define FRAME_MSG_QUE_SIZE 10
#define MATBUF_ALLOC_STEP  200

#define FRAMEMODE_FIXED 0 
#define FRAMEMODE_FULL  1
#define FRAMEMODE_VAR   2
#define FRAMEMODE_LIST  3
#define FRAMEMODE_META  4


class cWinToVecProcessor : public cDataProcessor {
  private:
    int   frameMode;
    int   fsfGiven;   // flag that indicates whether frameSizeFrame, etc. was specified directly (to override frameSize in seconds)
    int   fstfGiven;   // flag that indicates whether frameStepFrame, etc. was specified directly (to override frameStep in seconds)
    int   noPostEOIprocessing;
    int   nIntervals;  // number of intervals for frameMode = list
    double *ivSec; //interleaved array : start/end in seconds (frameList, frameMode = list)
    long *ivFrames; //interleaved array : start/end in frames (vIdx) (frameList, frameMode = list)
    long frameIdx; // next frame index, (frameMode== list)

    long Mult;
    long matBufN; // number of valid elements in matbuf
    long matBufNalloc; // backup of matbuf->nT
    char * lastText, *lastCustom;
    double inputPeriod;
    
    cMatrix *matBuf;
    cMatrix *tmpRow;
    cVector *tmpVec;
    FLOAT_DMEM *tmpFrameF;

    //mapping of field indices to config indices: (size of these array is maximum possible size: Nfi)
    int Nfconf;
    int *fconf, *fconfInv;
    long *confBs;  // blocksize for configurations

    int addFconf(long bs, int field); // return value is index of assigned configuration
    
    // message memory:
    int nQ; // number of frames queued
    double Qstart[FRAME_MSG_QUE_SIZE]; // start time array
    double Qend[FRAME_MSG_QUE_SIZE];   // end time array
    int Qflag[FRAME_MSG_QUE_SIZE];     // final frame flag (turn end..)
    int QID[FRAME_MSG_QUE_SIZE];     // final frame flag (turn end..)
    int getNextFrameData(double *start, double *end, int *flag, int *ID);
    int peekNextFrameData(double *start, double *end, int *flag, int *ID);
    int clearNextFrameData();
    int queNextFrameData(double start, double end, int flag, int ID);
    void addVecToBuf(cVector *ve);

  protected:
    long Ni, Nfi;
    long No; //, Nfo;
    int   allow_last_frame_incomplete_;
    int   wholeMatrixMode; // this is 0 by default, but child classes can overwrite it, if they implement the doProcessMatrix method instead of the doProcess method
    int   processFieldsInMatrixMode; // <- this variable is set from the config by the child class, the config option must also be populated by the child class; by default the value is 0

    double frameSize, frameStep, frameCenter;
    long  frameSizeFrames, frameStepFrames, frameCenterFrames, pre;

    SMILECOMPONENT_STATIC_DECL_PR
    double getInputPeriod(){return inputPeriod;}
    long getNi() { return Ni; } // number of elements
    long getNfi() { return Nfi; } // number of fields
    long getNf() { return Ni; } // return whatever will be processed (Ni: winToVec, Nfi, vecProc.)

    // converts a string time (framemode=list index list) to a float time value
    double stringToTimeval(char *x, int *isSec);

    // parses a frameList definition (comma separated list of start-end timestamps)
    // frees the memory allocated by *fl
    void parseFrameList(char * fl);

    // (input) field configuration, may be used in setupNamesForField
    int getFconf(int field) { return fconf[field]; } // caller must check for return value -1 (= no conf available for this field)
    void multiConfFree( void * x );
    void *multiConfAlloc() {
      return calloc(1,sizeof(void*)*getNf());
    }
    
    virtual void myFetchConfig() override;
    //virtual int myFinaliseInstance() override;
    //virtual int myConfigureInstance() override;
    virtual int dataProcessorCustomFinalise() override;
    virtual eTickResult myTick(long long t) override;

    // this must return the multiplier, i.e. the vector size returned for each input element (e.g. number of functionals, etc.)
    virtual int getMultiplier();
    
    // get the number of outputs given a number of inputs, 
    // this is used for wholeMatrixMode, where the quantitative relation between inputs and outputs may be non-linear
    // this replaces getMultiplier, which is used in the standard row-wise processing mode
    virtual long getNoutputs(long nIn);

    virtual int configureReader(const sDmLevelConfig &c) override;
    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNamesForElement(int idxi, const char*name, long nEl);
    //virtual int setupNamesForField(int idxi, const char*name, long nEl) override;
    virtual int doProcessMatrix(int i, cMatrix *in, FLOAT_DMEM *out, long nOut);
    virtual int doProcess(int i, cMatrix *row, FLOAT_DMEM*x);
    virtual int doFlush(int i, FLOAT_DMEM*x);

    virtual int processComponentMessage(cComponentMessage *_msg) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cWinToVecProcessor(const char *_name);

    virtual ~cWinToVecProcessor();
};




#endif // __CWINTOVECPROCESSOR_HPP
