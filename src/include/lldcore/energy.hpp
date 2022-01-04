/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

compute RMS and log frame energy

*/


#ifndef __CENERGY_HPP
#define __CENERGY_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CENERGY "This component computes logarithmic (log) and root-mean-square (rms) signal energy from PCM frames."
#define COMPONENT_NAME_CENERGY "cEnergy"

class cEnergy : public cVectorProcessor {
  private:
    int htkcompatible;
	  int erms, elog;
	  int energy2;
    FLOAT_DMEM escaleRms, escaleLog, escaleSquare;
    FLOAT_DMEM ebiasLog, ebiasRms, ebiasSquare;
    
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
    
    cEnergy(const char *_name);

    virtual ~cEnergy();
};




#endif // __CENERGY_HPP
