/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

ARFF file output (for WEKA)

*/


#ifndef __CDATADUMPSINK_HPP
#define __CDATADUMPSINK_HPP

#include <core/smileCommon.hpp>
#include <core/dataSink.hpp>

#define COMPONENT_DESCRIPTION_CDATADUMPSINK "This component writes dataMemory data to a raw binary file (e.g. for matlab import). The binary file consists of 32-bit float values representing the data values, concatenated frame by frame along the time axis. The first two float values in the file are the file header, indicating the dimension of the matrix (1: size of frames, 2: number of frames in file). The total file size in bytes is thus <size of frames>x<number of frames>x4 + 2."
#define COMPONENT_NAME_CDATADUMPSINK "cDatadumpSink"

class cDatadumpSink : public cDataSink {
  private:
    FILE * filehandle;
    const char *filename;
    int lag;
    int append;
    long nVec,vecSize;
    
    void writeHeader();
    
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
    
    cDatadumpSink(const char *_name);

    virtual ~cDatadumpSink();
};




#endif // __CDATADUMPSINK_HPP
