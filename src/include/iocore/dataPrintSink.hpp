/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#ifndef __CDATAPRINTSINK_HPP
#define __CDATAPRINTSINK_HPP

#include <core/smileCommon.hpp>
#include <core/dataSink.hpp>

#define COMPONENT_NAME_CDATAPRINTSINK "cDataPrintSink"
#define COMPONENT_DESCRIPTION_CDATAPRINTSINK "This component prints data as text to stdout or log, optionally in a standard parseable result format."
#define BUILD_COMPONENT_DataPrintSink

class cDataPrintSink : public cDataSink {
  private:
    int parseable_;
    int useLog_;
    int printTimeMeta_;

    template<typename... Args>
    void print(const char *fmt, Args... args)
    {
      if (useLog_) {
        SMILE_PRINT(fmt, args...);
      } else {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
        printf(fmt, args...);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
      }
    }
        
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cDataPrintSink(const char *_name);
    virtual ~cDataPrintSink();
};

#endif // __CDATAPRINTSINK_HPP

