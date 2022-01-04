/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

compute simplified intensity according to :
 Andreas Kie�ling - Extraktion und Klassifikation prosodischer Merkmale in der automatischen Sprachverarbeitung
 Pg. 156-157

*/


#ifndef __CINTENSITY_HPP
#define __CINTENSITY_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CINTENSITY "This component computes simplified frame intensity (narrow band approximation). IMPORTANT: It expects UNwindowed raw PCM frames as input!! A Hamming window is internally applied and the resulting signal is squared bevore applying loudness compression, etc."
#define COMPONENT_NAME_CINTENSITY "cIntensity"

class cIntensity : public cVectorProcessor {
  private:
    double I0;
    double *hamWin;
    long nWin;
    double winSum;

    int intensity, loudness;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    //virtual eTickResult myTick(long long t) override;

    //virtual int configureWriter(const sDmLevelConfig *c) override;

    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cIntensity(const char *_name);

    virtual ~cIntensity();
};




#endif // __CINTENSITY_HPP
