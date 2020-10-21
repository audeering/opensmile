/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: cHtkSource
-----------------------------------

HTK Source:

Reads data from an HTK parameter file.

-----------------------------------

11/2009 - Written by Florian Eyben
*/


#ifndef __CHTKSOURCE_HPP
#define __CHTKSOURCE_HPP

#include <core/smileCommon.hpp>
#include <core/dataSource.hpp>
//#include <htkSink.hpp>   // include this for the HTK header type and endianness functions

#define COMPONENT_DESCRIPTION_CHTKSOURCE "This component reads data from binary HTK parameter files."
#define COMPONENT_NAME_CHTKSOURCE "cHtkSource"

class cHtkSource : public cDataSource {
  private:
    sHTKheader head;
    int vax;
    const char * featureName;
    int N; // <-- sampleSize
    float *tmpvec;

    FILE *filehandle;
    const char *filename;
    int eof;
 
    void prepareHeader( sHTKheader *h )
    {
      smileHtk_prepareHeader(h);
    }

    int readHeader() 
    {
      return smileHtk_readHeader(filehandle,&head);
    }

  protected:
    SMILECOMPONENT_STATIC_DECL_PR


    virtual void myFetchConfig() override;
    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int setupNewNames(long nEl=0) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cHtkSource(const char *_name);

    virtual ~cHtkSource();
};




#endif // __CHTKSOURCE_HPP
