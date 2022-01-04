/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Average Magnitude Difference Function (AMDF)

*/


#ifndef __CAMDF_HPP
#define __CAMDF_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CAMDF "This component computes the Average Magnitude Difference Function (AMDF) for each input frame. Various methods for padding or warping at the border exist."
#define COMPONENT_NAME_CAMDF "cAmdf"


#define AMDF_LIMIT    0
#define AMDF_WARP     1
#define AMDF_ZEROPAD  2

class cAmdf : public cVectorProcessor {
  private:
    //int htkcompatible;
    int nLag;
    int method;
    int invert;
    
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
    
    cAmdf(const char *_name);

    virtual ~cAmdf();
};




#endif // __CAMDF_HPP
