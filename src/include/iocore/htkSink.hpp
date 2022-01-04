/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

ARFF file output (for WEKA)

*/


#ifndef __CHTKSINK_HPP
#define __CHTKSINK_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <core/dataSink.hpp>

#define COMPONENT_DESCRIPTION_CHTKSINK "This component writes dataMemory data to a binary HTK parameter file."
#define COMPONENT_NAME_CHTKSINK "cHtkSink"


class cHtkSink : public cDataSink {
  private:
    FILE * filehandle;
    int vax;
    const char *filename;
    int lag;
    int append;
    uint16_t parmKind;
    uint32_t vecSize;
    uint32_t nVec;
    double period;
    double forcePeriod_;
    sHTKheader header;
    bool disabledSink_;

    void prepareHeader( sHTKheader *h )
    {
      smileHtk_prepareHeader(h);
    }

    int writeHeader();
    
    int readHeader() 
    {
      return smileHtk_readHeader(filehandle,&header);
    }
    
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
    
    cHtkSink(const char *_name);

    virtual ~cHtkSink();
};




#endif // __CHTKSINK_HPP
