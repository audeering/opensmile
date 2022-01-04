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


#ifndef __CPITCHBASEEXAMPLE_HPP
#define __CPITCHBASEEXAMPLE_HPP

#include <core/smileCommon.hpp>
#include <lldcore/pitchBase.hpp>

#define COMPONENT_DESCRIPTION_CPITCHBASEEXAMPLE "Base class for all pitch classes, no functionality on its own!"
#define COMPONENT_NAME_CPITCHBASEEXAMPLE "cPitchBaseExample"


class cPitchBaseExample : public cPitchBase {
  private:

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
/*	  virtual int setupNewNames(long nEl) override; */
    
    // to be overwritten by child class:
    virtual int pitchDetect(FLOAT_DMEM * _inData, long _N, double _fsSec, double _baseT, FLOAT_DMEM *_f0cand, FLOAT_DMEM *_candVoice, FLOAT_DMEM *_candScore, long _nCandidates) override;
    virtual int addCustomOutputs(FLOAT_DMEM *dstCur, long NdstLeft) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cPitchBaseExample(const char *_name);
    
    virtual ~cPitchBaseExample();
};




#endif // __CPITCHBASEEXAMPLE_HPP
