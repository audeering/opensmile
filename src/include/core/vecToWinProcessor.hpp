/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

reads in frames , outputs windows
*/


#ifndef __CVECTOWINPROCESSOR_HPP
#define __CVECTOWINPROCESSOR_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>
#include <dspcore/windower.hpp>

#define COMPONENT_DESCRIPTION_CVECTOWINPROCESSOR "Base class: reads in frames , outputs windows"
#define COMPONENT_NAME_CVECTOWINPROCESSOR "cVecToWinProcessor"

struct sVecToWinProcessorOla
{
  double * norm;
  FLOAT_DMEM * buffer;
  long bufferPtr;
  long bufferReadPtr;
  long buffersize;
  double overlap;
  long framelen;
};


class cVecToWinProcessor : public cDataProcessor {
  private:
    
    int   noPostEOIprocessing;
    int   processArrayFields;
    int   normaliseAdd;
    int   useWinAasWinB;
    FLOAT_DMEM gain;

    long Nfi;
    long No;

    cWindower * ptrWinA;
    cWindower * ptrWinB;

    long inputPeriodS;
    double inputPeriod;
    double samplePeriod;
    
    int hasOverlap;
    struct sVecToWinProcessorOla * ola;

    cMatrix *mat;

  protected:

    SMILECOMPONENT_STATIC_DECL_PR
    double getInputPeriod(){ return inputPeriod; }
    double getSamplePeriod(){ return samplePeriod; }

    long getNfi() { return Nfi; } // number of fields
    

    virtual void myFetchConfig() override;
    //virtual int myFinaliseInstance() override;
    //virtual int myConfigureInstance() override;
    virtual int dataProcessorCustomFinalise() override;
    virtual eTickResult myTick(long long t) override;

    virtual int configureWriter(sDmLevelConfig &c) override;
    //virtual int setupNamesForElement(int idxi, const char*name, long nEl) override;
    //virtual int setupNamesForField(int idxi, const char*name, long nEl) override;

    void initOla(long n, double _samplePeriod, double _inputPeriod, int idx);
    void computeOlaNorm(long n, int idx);
    double * getWindowfunction(cWindower *_ptrWin, long n, int *didAlloc = NULL);
    //void setOlaBuffer(long i, long j, FLOAT_DMEM val);
    void setOlaBufferNext(long idx, FLOAT_DMEM val);
    int flushOlaBuffer(cMatrix *mat);
    
    virtual int doProcess(int i, cMatrix *row, FLOAT_DMEM*x);
    virtual int doFlush(int i, FLOAT_DMEM*x);
    

    //virtual int processComponentMessage(cComponentMessage *_msg) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cVecToWinProcessor(const char *_name);

    virtual ~cVecToWinProcessor();
};




#endif // __CVECTOWINPROCESSOR_HPP
