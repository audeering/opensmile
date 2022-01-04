/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

dataFramer

*/


#ifndef __CFRAMER_HPP
#define __CFRAMER_HPP

#include <core/smileCommon.hpp>
#include <core/winToVecProcessor.hpp>

#define COMPONENT_DESCRIPTION_CFRAMER "This component creates frames from single dimensional input stream. It is possible to specify the frame step and frame size independently, thus allowing for overlapping frames or non continuous frames."
#define COMPONENT_NAME_CFRAMER "cFramer"

class cFramer : public cWinToVecProcessor {
  private:

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    //virtual void myFetchConfig() override;
    //virtual int myFinaliseInstance() override;
    //virtual eTickResult myTick(long long t) override;

    virtual int getMultiplier() override;
    //virtual int configureWriter(const sDmLevelConfig *c) override;
    //virtual int setupNamesForField(int idxi, const char*name, long nEl) override;
    virtual int doProcess(int i, cMatrix *row, FLOAT_DMEM*x) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFramer(const char *_name);

    virtual ~cFramer();
};




#endif // __CFRAMER_HPP
