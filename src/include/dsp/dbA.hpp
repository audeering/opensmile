/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: dbA

applies dbX weighting to fft magnitudes

*/


#ifndef __CDBA_HPP
#define __CDBA_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>
#include <math.h>

#define BUILD_COMPONENT_DbA
#define COMPONENT_DESCRIPTION_CDBA "This component performs dbX (dbA,dbB,dbC,...) equal loudness weighting of FFT bin magnitudes. Currently only dbA weighting is implemented."
#define COMPONENT_NAME_CDBA "cDbA"

#define CURVE_DBA 0
#define CURVE_DBB 1
#define CURVE_DBC 2

extern void computeDBA(FLOAT_DMEM *x, long blocksize, FLOAT_DMEM F0);

class cDbA : public cVectorProcessor {
  private:
    int curve;
    int usePower;
    FLOAT_DMEM **filterCoeffs;
    
    int computeFilters( long blocksize, double frameSizeSec, int idxc );

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    //virtual eTickResult myTick(long long t) override;

    //virtual int configureWriter(const sDmLevelConfig *c) override;

    virtual int dataProcessorCustomFinalise() override;
    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cDbA(const char *_name);

    virtual ~cDbA();
};




#endif // __CDBA_HPP
