/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:


*/


#ifndef __DATA_SOURCE_HPP
#define __DATA_SOURCE_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <core/dataWriter.hpp>

#define COMPONENT_DESCRIPTION_CDATASOURCE "This is a base class for components, which write data to dataMemory, but do not read from it."
#define COMPONENT_NAME_CDATASOURCE "cDataSource"

class cDataSource : public cSmileComponent {
  private:

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    cDataWriter *writer_;
    cVector     *vec_;    // current vector to be written
    cMatrix     *mat_;    // current matrix to be written
    double buffersize_sec_; /* buffersize of write level, as requested by config file (in seconds)*/
    double blocksizeW_sec; /* blocksizes for block processing (in seconds)*/
    long buffersize_; /* buffersize of write level, as requested by config file (in frames)*/
    long blocksizeW_; /* blocksizes for block processing (in frames)*/
    double period_; /* period, as set by config */
    double basePeriod_; 
    int namesAreSet_;

    virtual void myFetchConfig() override;
    virtual void mySetEnvironment() override;
    virtual int myRegisterInstance(int *runMe=NULL) override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    void allocVec(int n);
    void allocMat(int n, int t);

    // *runMe return value for component manager : 0, don't call my tick of this component, 1, call myTick
    virtual int runMeConfig() { return 1; }
    virtual int configureWriter(sDmLevelConfig &c) { return 1; }
    virtual int setupNewNames(long nEl=0) { return 1; }

    virtual int setEOIcounter(int cnt) override {
      int ret = cSmileComponent::setEOIcounter(cnt);
      if (writer_!=NULL) return writer_->setEOIcounter(cnt);
      return ret;
    }

    virtual void setEOI() override {
      cSmileComponent::setEOI();
      if (writer_!=NULL) writer_->setEOI();
    }

    virtual void unsetEOI() override {
      cSmileComponent::unsetEOI();
      if (writer_!=NULL) writer_->unsetEOI();
    }

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cDataSource(const char *_name);
    virtual ~cDataSource();
};




#endif // __DATA_SOURCE_HPP
