/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

time signal properties: min/max sample value, mean and zero crossing rates

*/


#ifndef __CMZCR_HPP
#define __CMZCR_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CMZCR "This component computes time signal properties, zero-corssing rate, mean-crossing rate, dc offset, max/min value, and absolute maximum value of a PCM frame."
#define COMPONENT_NAME_CMZCR "cMZcr"


class cMZcr : public cVectorProcessor {
  private:
    int zcr, mcr, amax, maxmin, dc;
    
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
    
    cMZcr(const char *_name);

    virtual ~cMZcr();
};




#endif // __CMZCR_HPP
