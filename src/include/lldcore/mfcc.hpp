/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

computes MFCC from Mel-spectrum (see cMelspec)

*/


#ifndef __CMFCC_HPP
#define __CMFCC_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>
#include <math.h>

#define COMPONENT_DESCRIPTION_CMFCC "This component computes Mel-frequency cepstral coefficients (MFCC) from a critical band spectrum (see 'cMelspec'). An I-DCT of type-II is used from transformation from the spectral to the cepstral domain. Liftering of cepstral coefficients is supported. HTK compatible values can be computed."
#define COMPONENT_NAME_CMFCC "cMfcc"

class cMfcc : public cVectorProcessor {
  private:
    int printDctBaseFunctions;
    int inverse;
    int nBands, htkcompatible, usePower;
    FLOAT_DMEM **costable;
    FLOAT_DMEM **sintable;
    int firstMfcc, lastMfcc, nMfcc;
    FLOAT_DMEM melfloor;
    FLOAT_DMEM cepLifter;
    int doLog_;
    
    int initTables( long blocksize, int idxc );
    
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
    
    cMfcc(const char *_name);

    virtual ~cMfcc();
};




#endif // __CMFCC_HPP
