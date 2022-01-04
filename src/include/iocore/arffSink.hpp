/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

ARFF file output (for WEKA)

*/


#ifndef __ARFF_SINK_HPP
#define __ARFF_SINK_HPP

#include <core/smileCommon.hpp>
#include <core/dataSink.hpp>
#include <string>
#include <vector>

#define COMPONENT_DESCRIPTION_CARFFSINK "This component writes dataMemory data to an ARFF file (WEKA). Depending on your config an instance name field, a frame index, and a frame time field can be added as well as multiple class/target attributes. See the config type documentation for more details."
#define COMPONENT_NAME_CARFFSINK "cArffSink"

class cArffSink : public cDataSink {
  private:
    FILE * filehandle;
    double frameTimeAdd;
    const char *filename;
    int lag;
    int append, timestamp, number, prname, frameLength;
    int instanceNameFromMetadata, useTargetsFromMetadata;
    const char *relation;
    const char *instanceBase, *instanceName;
    bool disabledSink_;

    int printDefaultClassDummyAttribute;    
    int nClasses; long nInst;
    long inr;
    std::vector<std::string> classname;
    std::vector<std::string> classtype;
    
    std::vector<std::string> targetall;
    std::vector<std::vector<std::string>> targetinst;
    
    static std::string escape(const char *str);
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;


  public:
    //static sComponentInfo * registerComponent(cConfigManager *_confman);
    //static cSmileComponent * create(const char *_instname);
    SMILECOMPONENT_STATIC_DECL
    
    cArffSink(const char *_name);

    virtual ~cArffSink();
};




#endif // __ARFF_SINK_HPP
