/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

LibSVM file output

*/


#ifndef __CLIBSVMSINK_HPP
#define __CLIBSVMSINK_HPP

#include <core/smileCommon.hpp>
#include <core/dataSink.hpp>

#define BUILD_COMPONENT_LibsvmSink
#define COMPONENT_DESCRIPTION_CLIBSVMSINK "This component writes data to a text file in LibSVM feature file format. For the 'on-the-fly' classification component see 'cLibsvmliveSink'."
#define COMPONENT_NAME_CLIBSVMSINK "cLibsvmSink"


class cLibsvmSink : public cDataSink {
  private:
    FILE * filehandle;
    const char *filename;
    int lag;
    int append, timestamp;
    const char *instanceBase, *instanceName;
    
    int targetNumAll;
    int nClasses;
    long nInst;
    long inr;
    char **classname;
    int *target;

    int getClassIndex(const char *_name)
    {
      int i;
      if (_name == NULL) return -1;
      if (classname == NULL) return -1;
      for (i=0; i<nClasses; i++) {
        if ((classname[i]!=NULL)&&(!strcmp(_name,classname[i]))) return i;
      }
      return -1;
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
    
    cLibsvmSink(const char *_name);

    virtual ~cLibsvmSink();
};




#endif // __CLIBSVMSINK_HPP
