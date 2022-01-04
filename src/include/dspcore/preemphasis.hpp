/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

simple preemphasis : x(t) = x(t) - k*x(t-1)

*/


#ifndef __CPREEMPHASIS_HPP
#define __CPREEMPHASIS_HPP

#include <core/smileCommon.hpp>
#include <core/windowProcessor.hpp>

#define COMPONENT_DESCRIPTION_CPREEMPHASIS "This component performs pre- and de-emphasis of speech signals using a 1st order difference equation: y(t) = x(t) - k*x(t-1)  (de-emphasis: y(t) = x(t) + k*x(t-1))"
#define COMPONENT_NAME_CPREEMPHASIS "cPreemphasis"


class cPreemphasis : public cWindowProcessor {
  private:
    FLOAT_DMEM k;
    double f;
    int de;

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
    virtual int dataProcessorCustomFinalise() override;
/*
    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;
*/
    
  public:
    SMILECOMPONENT_STATIC_DECL
    
    cPreemphasis(const char *_name);

    virtual ~cPreemphasis();
};




#endif // __CPREEMPHASIS_HPP
