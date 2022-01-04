/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

pre-emphasis per frame  (simplification, however, this is the way HTK does it... so for compatibility... here you go)
(use before window function is applied!)

*/


#ifndef __CVECTORPREEMPHASIS_HPP
#define __CVECTORPREEMPHASIS_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CVECTORPREEMPHASIS "This component performs per frame pre-emphasis without an inter-frame state memory. (This is the way HTK does pre-emphasis). Pre-emphasis: y(t) = x(t) - k*x(t-1) ; de-emphasis : y(t) = x(t) + k*x(t-1)"
#define COMPONENT_NAME_CVECTORPREEMPHASIS "cVectorPreemphasis"

class cVectorPreemphasis : public cVectorProcessor {
  private:
    FLOAT_DMEM k;
    double f;
    int de;
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    //virtual eTickResult myTick(long long t) override;

    //virtual int configureWriter(const sDmLevelConfig *c) override;

    virtual int dataProcessorCustomFinalise() override;
//    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cVectorPreemphasis(const char *_name);

    virtual ~cVectorPreemphasis();
};




#endif // __CVECTORPREEMPHASIS_HPP
