/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

experimental resampling by ideal fourier interpolation and band limiting
this component takes a complex (!) dft spectrum (generated from real values) as input

*/


#ifndef __CSPECRESAMPLE_HPP
#define __CSPECRESAMPLE_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define BUILD_COMPONENT_SpecResample
#define COMPONENT_DESCRIPTION_CSPECRESAMPLE "This component implements a spectral domain resampling component. Input frames are complex valued spectral domain data, which will be shifted and scaled by this component, and a modified DFT is performed to synthesize samples at the new rate."
#define COMPONENT_NAME_CSPECRESAMPLE "cSpecResample"

class cSpecResample : public cVectorProcessor {
  private:
    int antiAlias;
    long kMax;
    double sr;
    double fsSec;
    double targetFs;
    double resampleRatio;
    long _Nin, _Nout;
    FLOAT_DMEM *inData;
    const char * inputFieldPartial;

    sDftWork * dftWork;
    //FLOAT_DMEM *costable, *sintable;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
	  virtual int setupNewNames(long nEl) override;
    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cSpecResample(const char *_name);
    virtual ~cSpecResample();
};




#endif // __CSPECRESAMPLE_HPP
