/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example of a windowProcessor component

*/


#ifndef __CEXAMPLEWINDOWPROCESSOR_HPP
#define __CEXAMPLEWINDOWPROCESSOR_HPP

#include <core/smileCommon.hpp>
#include <core/windowProcessor.hpp>

#define COMPONENT_DESCRIPTION_CEXAMPLEWINDOWPROCESSOR "This is an example of a cWindowProcessor descendant. It has no meaningful function, this component is intended as a template for developers."
#define COMPONENT_NAME_CEXAMPLEWINDOWPROCESSOR "cExampleWindowProcessor"

class cExampleWindowProcessor : public cWindowProcessor {
  private:
    FLOAT_DMEM k;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR


    virtual void myFetchConfig() override;
/*
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;
*/

    //virtual int configureWriter(const sDmLevelConfig *c) override;

   // buffer must include all (# order) past samples
    virtual int processBuffer(cMatrix *_in, cMatrix *_out, int _pre, int _post) override;
    
/*
    virtual int setupNamesForField(int i, const char*name, long nEl) override;
*/
    
  public:
    SMILECOMPONENT_STATIC_DECL
    
    cExampleWindowProcessor(const char *_name);

    virtual ~cExampleWindowProcessor();
};




#endif // __CEXAMPLEWINDOWPROCESSOR_HPP
