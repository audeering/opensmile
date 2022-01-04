/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Comma Separated Value file output (CSV)

*/


#ifndef __CCSVSINK_HPP
#define __CCSVSINK_HPP

#include <core/smileCommon.hpp>
#include <core/dataSink.hpp>

#define COMPONENT_DESCRIPTION_CCSVSINK "This component exports data in CSV (comma-separated-value) format used in many spreadsheet applications. As the first line of the CSV file a header line may be printed, which contains a delimiter separated list of field names of the output values."
#define COMPONENT_NAME_CCSVSINK "cCsvSink"

class cCsvSink : public cDataSink {
  private:
    FILE * filehandle;
    const char *filename;
    const char *instanceName;
    const char *instanceBase;
    bool disabledSink_;
    char delimChar_;
    int lag;
    int flush_;
    int prname_;
    bool append_;
    bool timestamp_;
    bool number_;
    bool printHeader_;
    bool frameLength_;
    
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
    
    cCsvSink(const char *_name);

    virtual ~cCsvSink();
};




#endif // __CCSVSINK_HPP
