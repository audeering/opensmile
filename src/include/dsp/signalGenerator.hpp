/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Signal source. Generates various noise types and pre-defined signals.

*/


#ifndef __CSIGNALGENERATOR_HPP
#define __CSIGNALGENERATOR_HPP

#include <core/smileCommon.hpp>
#include <core/dataSource.hpp>

#define BUILD_COMPONENT_SignalGenerator
#define COMPONENT_DESCRIPTION_CSIGNALGENERATOR "This component provides a signal source. This source generates various noise types and pre-defined signals and value patterns. See the configuration documentation for a list of currently implemented types."
#define COMPONENT_NAME_CSIGNALGENERATOR "cSignalGenerator"

#define NOISE_WHITE   0   // white gaussian noise 'white'
#define SIGNAL_SINE   1   // sinusodial signal (single frequency) 'sine'
#define SIGNAL_CONST  2   // constant value 'const'
#define SIGNAL_RECT   3   // rectangular periodic signal 'rect'
//...

class cSignalGenerator : public cDataSource {
  private:
    long nValues;
    int randSeed;
    FLOAT_DMEM stddev, mean;
    FLOAT_DMEM constant;
    double signalPeriod, phase;
    double myt ; // current time
    double samplePeriod;
    double lastP;
    double scale;
    double val;

    int * nElements;
    char ** fieldNames;
    int nFields;
    
    int noiseType;
    long lengthFrames;
    long curT;
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNewNames(long nEl) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cSignalGenerator(const char *_name);

    virtual ~cSignalGenerator();
};




#endif // __CSIGNALGENERATOR_HPP
