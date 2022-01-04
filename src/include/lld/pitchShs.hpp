/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

This component computes pitch via the Harmonic Product Spectrum method.
As input it requires FFT magnitude data. 
Note that the type of input data is not checked, thus be careful when writing your configuration files!

*/


#ifndef __CPITCHSHS_HPP
#define __CPITCHSHS_HPP

#include <core/smileCommon.hpp>
#include <lldcore/pitchBase.hpp>

#define BUILD_COMPONENT_PitchShs
#define COMPONENT_DESCRIPTION_CPITCHSHS "This component computes the fundamental frequency via the Sub-Harmonic-Sampling (SHS) method (this is related to the Harmonic Product Spectrum method)."
#define COMPONENT_NAME_CPITCHSHS "cPitchShs"


class cPitchShs : public cPitchBase {
  private:
    int nHarmonics;
    int greedyPeakAlgo;
    FLOAT_DMEM Fmint, Fstept;
    FLOAT_DMEM nOctaves, nPointsPerOctave;
    FLOAT_DMEM *SS;
    FLOAT_DMEM *Fmap;
    FLOAT_DMEM compressionFactor;
    double base;
    double lfCut_;
    int shsSpectrumOutput;

    void addNameAppendFieldShs(const char*base, const char*append, int N, int arrNameOffset);
    int cloneInputFieldInfoShs(int sourceFidx, int targetFidx, int force);

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    cDataWriter * shsWriter_;
    cVector * shsVector_;

    virtual void myFetchConfig() override;
    virtual int setupNewNames(long nEl) override;
    
    virtual void mySetEnvironment() override;
    virtual int myRegisterInstance(int *runMe) override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;

    // to be overwritten by child class:
    virtual int pitchDetect(FLOAT_DMEM * inData, long N_, double _fsSec, double baseT, FLOAT_DMEM *f0cand, FLOAT_DMEM *candVoice, FLOAT_DMEM *candScore, long nCandidates) override;
    virtual int addCustomOutputs(FLOAT_DMEM *dstCur, long NdstLeft) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cPitchShs(const char *_name);
    
    virtual ~cPitchShs();
};




#endif // __CPITCHSHS_HPP
