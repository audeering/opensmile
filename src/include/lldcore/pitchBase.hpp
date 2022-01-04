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


#ifndef __CPITCHBASE_HPP
#define __CPITCHBASE_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CPITCHBASE "Base class for all pitch classes, no functionality on its own!"
#define COMPONENT_NAME_CPITCHBASE "cPitchBase"


class cPitchBase : public cVectorProcessor {
  private:
    const char *inputFieldPartial;
    int nCandidates_;
    int scores, voicing;
    int F0C1, voicingC1;
    int F0raw;
    int voicingClip;

    double maxPitch_;
    double minPitch_;

    FLOAT_DMEM *inData_;
    FLOAT_DMEM *f0cand_, *candVoice_, *candScore_;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    FLOAT_DMEM voicingCutoff;
    int octaveCorrection;

    virtual void myFetchConfig() override;
	  virtual int setupNewNames(long nEl) override;
    
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

    // to be overwritten by child class:
    virtual int pitchDetect(FLOAT_DMEM * inData, long N_, double _fsSec, double baseT, FLOAT_DMEM *f0cand, FLOAT_DMEM *candVoice, FLOAT_DMEM *candScore, long nCandidates);
    virtual int addCustomOutputs(FLOAT_DMEM *dstCur, long NdstLeft);

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cPitchBase(const char *_name);
    
    virtual ~cPitchBase();
};




#endif // __CPITCHBASE_HPP
