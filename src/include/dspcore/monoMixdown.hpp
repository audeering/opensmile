/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

simple mixer, which adds multiple channels (elements) to a single channel (element)

*/


#ifndef __CMONOMIXDOWN_HPP
#define __CMONOMIXDOWN_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>

#define COMPONENT_DESCRIPTION_CMONOMIXDOWN "This is a simple mixer, which adds multiple channels (elements) to a single channel (element)."
#define COMPONENT_NAME_CMONOMIXDOWN "cMonoMixdown"

class cMonoMixdown : public cDataProcessor {
  private:
    int normalise;
    long bufsize;
    cMatrix *matout;
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    // virtual int dataProcessorCustomFinalise() override;
    // virtual int setupNewNames(long nEl) override;
    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int configureWriter(sDmLevelConfig &c) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cMonoMixdown(const char *_name);

    virtual ~cMonoMixdown();
};




#endif // __CMONOMIXDOWN_HPP
