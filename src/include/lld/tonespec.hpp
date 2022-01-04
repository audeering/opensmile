/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

computes (or rather estimates) semi-tone spectrum from fft spectrum

*/


#ifndef __CTONESPEC_HPP
#define __CTONESPEC_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>
#include <math.h>

#define BUILD_COMPONENT_Tonespec
#define COMPONENT_DESCRIPTION_CTONESPEC "This component computes (or rather estimates) a semi-tone spectrum from an FFT magnitude spectrum."
#define COMPONENT_NAME_CTONESPEC "cTonespec"

class cTonespec : public cVectorProcessor {
  private:
    int nOctaves, nNotes;
    int usePower, dbA;
  #ifdef DEBUG
    int printBinMap, printFilterMap;
  #endif
  
    FLOAT_DMEM firstNote;  // frequency of first note
    FLOAT_DMEM lastNote;   // frequency of last note (will be computed)

    FLOAT_DMEM **pitchClassFreq;
    FLOAT_DMEM **distance2key;
    FLOAT_DMEM **filterMap;
    FLOAT_DMEM **db;
    
    int **binKey;
    int **pitchClassNbins;
    int **flBin;
    
    int filterType;

    void computeFilters( long blocksize, double frameSizeSec, int idxc );
    void setPitchclassFreq( int idxc );
    
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
    
    cTonespec(const char *_name);

    virtual ~cTonespec();
};




#endif // __CTONESPEC_HPP
