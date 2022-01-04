/*F***************************************************************************
 * This file is part of openSMILE.
 *
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Provides a callback function to external code that gets for each frame that
this sink component reads.

*/


#ifndef __CEXTERNALSINK_HPP
#define __CEXTERNALSINK_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <core/dataSink.hpp>

#define COMPONENT_DESCRIPTION_CEXTERNALSINK "This component allows external code to programmatically access the data read by this component."
#define COMPONENT_NAME_CEXTERNALSINK "cExternalSink"

typedef bool (*ExternalSinkCallback)(const FLOAT_DMEM *data, long vectorSize, void *param);

struct sExternalSinkMetaDataEx {
  long vIdx;
  double time;
  double period;
  double lengthSec;
};

typedef bool (*ExternalSinkCallbackEx)(const FLOAT_DMEM *data, long nT, long N, const sExternalSinkMetaDataEx *metaData, void *param);


class cExternalSink : public cDataSink {
  private:
    ExternalSinkCallback dataCallback;
	  void *callbackParam; // optional pointer to a custom object that will be passed to the callback function as a second parameter

    ExternalSinkCallbackEx dataCallbackEx;
    void *callbackParamEx; // optional pointer to a custom object that will be passed to the callback function as a second parameter
    long numElements;
    char **elementNames;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;
    virtual int configureReader() override;

  public:
    SMILECOMPONENT_STATIC_DECL

    cExternalSink(const char *_name);
    virtual ~cExternalSink();

    void setDataCallback(ExternalSinkCallback callback, void *param);
    void setDataCallbackEx(ExternalSinkCallbackEx callback, void *param);
    long getNumElements();
    const char *getElementName(long idx);

};


#endif // __CEXTERNALSINK_HPP
