/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Component to compute chroma features from a semi-tone spectrum. 

This component is based on original code from Moritz Dausinger (wave2chroma). 
The openSMILE chroma component was implemented by Christoph Kozielski.

*/


#ifndef __CCENS_HPP
#define __CCENS_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define BUILD_COMPONENT_Cens
#define COMPONENT_DESCRIPTION_CCENS "This component computes CENS (energy normalised and smoothed chroma features) from raw Chroma features generated by the 'cChroma' component."
#define COMPONENT_NAME_CCENS "cCens"

class cCens : public cVectorProcessor {
  private:
	  double **winf;
    FLOAT_DMEM **buffer;
    long *bptr, *dsidx;
    int downsampleRatio;

    int l2norm;
    int window;
    long winlength;

    void chromaDiscretise(const FLOAT_DMEM *in, FLOAT_DMEM *out, long N);

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    //virtual eTickResult myTick(long long t) override;
	
    virtual int configureWriter(sDmLevelConfig &c) override;
    //virtual void configureField(int idxi, long __N, long nOut) override;
    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cCens(const char *_name);

    virtual ~cCens();
};




#endif // __CCENS_HPP