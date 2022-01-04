/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

filter :  (abstract class only)
       linear N-th order filter for single value data streams
       this class processed every element of a frame independently
       derived classes only need to implement the filter algorithm

*/


#ifndef __CWINDOWPROCESSOR_HPP
#define __CWINDOWPROCESSOR_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>

#define COMPONENT_DESCRIPTION_CWINDOWPROCESSOR "filter dataProcessor, filters each element in a dataMemory level independently"
#define COMPONENT_NAME_CWINDOWPROCESSOR "cWindowProcessor"

class cWindowProcessor : public cDataProcessor {
  private:
    //int blocksize;         // block size for filter (speed up purpose only)
    long Ni;
    int isFirstFrame;
    int pre, post, winsize;
    int noPostEOIprocessing;
    
    cMatrix * matnew;
    cMatrix * rowout;
    cMatrix * rowsout;
    cMatrix * row;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    int multiplier;

    void setWindow(int _pre, int _post);
    int firstFrame() { return isFirstFrame; }

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int configureWriter(sDmLevelConfig &c) override;


   // buffer must include all (# order) past samples
    virtual int processBuffer(cMatrix *_in, cMatrix *_out,  int _pre, int _post );
    virtual int processBuffer(cMatrix *_in, cMatrix *_out,  int _pre, int _post, int rowGlobal );
    virtual int dataProcessorCustomFinalise() override;

/*
    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;
*/
    
  public:
    SMILECOMPONENT_STATIC_DECL
    
    cWindowProcessor(const char *_name, int _pre=0, int _post=0);

    virtual ~cWindowProcessor();
};




#endif // __CWINDOWPROCESSOR_HPP
