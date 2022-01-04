/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

dataSink

*/


#ifndef __DATA_SINK_HPP
#define __DATA_SINK_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <core/dataReader.hpp>

#define COMPONENT_DESCRIPTION_CDATASINK "This is a base class for components reading from (and not writing to) the dataMemory and dumping/passing data to external entities."
#define COMPONENT_NAME_CDATASINK "cDataSink"

class cDataSink : public cSmileComponent {
  private:

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    int errorOnNoOutput_;
    long nWritten_;
    long blocksizeR_;
    double blocksizeR_sec_;
    cDataReader *reader_;

    virtual void myFetchConfig() override;
    virtual void mySetEnvironment() override;
    virtual int myRegisterInstance(int *runMe=NULL) override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;

    // *runMe return value for component manager : 0, don't call my tick of this component, 1, call myTick
    virtual int runMeConfig() { return 1; }

    // Configures the reader, i.e. set blocksize requests etc.
    virtual int configureReader();

    // Proper handling of EOI events, passing them to the reader.
    virtual int setEOIcounter(int cnt) override {
      int ret = cSmileComponent::setEOIcounter(cnt);
      if (reader_!=NULL) return reader_->setEOIcounter(cnt);
      return ret;
    }

    // Proper handling of EOI events, passing them to the reader.
    virtual void setEOI() override {
      cSmileComponent::setEOI();
      if (reader_!=NULL) reader_->setEOI();
    }

    // Proper handling of EOI events, passing them to the reader.
    virtual void unsetEOI() override {
      cSmileComponent::unsetEOI();
      if (reader_!=NULL) reader_->unsetEOI();
    }

  public:
    SMILECOMPONENT_STATIC_DECL

    cDataSink(const char *_name);
    virtual ~cDataSink();
};




#endif // __DATA_SINK_HPP
