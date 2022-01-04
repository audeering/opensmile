/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

vector selector based on threshold of value <idx>

*/


#ifndef __CVALBASEDSELECTOR_HPP
#define __CVALBASEDSELECTOR_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>

#define COMPONENT_DESCRIPTION_CVALBASEDSELECTOR "This component copies only those frames from the input to the output that match a certain threshold criterion, i.e. where a specified value N exceeds a certain threshold."
#define COMPONENT_NAME_CVALBASEDSELECTOR "cValbasedSelector"

class cValbasedSelector : public cDataProcessor {
  private:
    long idx;
    int removeIdx, invert, allowEqual, zerovec;
    int adaptiveThreshold_;
    int debugAdaptiveThreshold_;
    FLOAT_DMEM outputVal;
    FLOAT_DMEM threshold;
    cVector *myVec;
    long elI;
    FLOAT_DMEM *valBuf_;
    FLOAT_DMEM valBufSum_;
    FLOAT_DMEM valBufN_;
    int valBufSize_, valBufIdx_;
    int valBufRefreshCnt_;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;
    virtual int setupNamesForField(int i, const char*name, long nEl) override;

    // virtual int dataProcessorCustomFinalise() override;
    // virtual int setupNewNames(long nEl) override;
    //virtual int setupNamesForField(int i, const char*name, long nEl) override;
    //virtual int configureWriter(sDmLevelConfig &c) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cValbasedSelector(const char *_name);

    virtual ~cValbasedSelector();
};




#endif // __CVALBASEDSELECTOR_HPP
