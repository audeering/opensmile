/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: cDataProcessor 

This is an abstract base class for all components which read data from the data memory and write new data to the data memory.

*/


#ifndef __DATA_PROCESSOR_HPP
#define __DATA_PROCESSOR_HPP

#include <core/smileCommon.hpp>
#include <core/dataWriter.hpp>
#include <core/dataReader.hpp>

#define COMPONENT_DESCRIPTION_CDATAPROCESSOR "This is an abstract base class for all components which read data from the data memory and write new data to the data memory."
#define COMPONENT_NAME_CDATAPROCESSOR "cDataProcessor"

class cDataProcessor : public cSmileComponent {
  private:
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    long nInput_, inputStart_;
    const char *inputName_;

    cDataWriter *writer_;
    cDataReader *reader_;
    int namesAreSet_;
    
    double buffersize_sec_; /* buffersize of write level, as requested by config file (in seconds)*/
    double blocksizeR_sec_, blocksizeW_sec_; /* blocksizes for block processing (in seconds)*/
    long buffersize_; /* buffersize of write level, as requested by config file (in frames)*/
    long blocksizeR_, blocksizeW_; /* blocksizes for block processing (in frames)*/
    
    int copyInputName_; /* 1=copy input name and append "nameAppend" (if != NULL) , 0 = do not copy and set name only to "nameAppend" */
    const char * nameAppend_; /* suffix to append to element names */
    void addNameAppendField(const char *base, const char *append, int N=1, int arrNameOffset=0);
    void addNameAppendFieldAuto(const char *base, const char *customFixed, int N=1, int arrNameOffset=0);

    // get the size of input frames in seconds
    double getFrameSizeSec();

    // get the period of the base level (e.g. sampling rate for wave input)
    double getBasePeriod();

    //
    // find a field in the input level by a part of its name or its full name
    // set the internal variables nInput, inputStart, and inputName
    //
    // nEl specifies the maximum number of input elements (for checking valid range of field index)
    // (optional) fullName = 1: match full field name instead of part of name
    long findInputField(const char *namePartial, int fullName=0, long nEl=-1);

    //
    // find a field in the input level by a part of its name or its full name
    //
    // nEl specifies the maximum number of input elements (for checking valid range of field index)
    // *N , optional, a pointer to variable (long) that will be filled with the number of elements in the field
    // **_fieldName : will be set to a pointer to the name of the field
    // (optional) fullName = 1: match full field name instead of part of name
    long findField(const char *namePartial, int fullName=0, long *N=NULL, const char **fieldName=NULL, long nEl=-1, int *more=NULL, int *fieldIdx=NULL);

    //
    // search for an element by its partial name (if multiple matches are found, only the first is returned)
    // if fullName==1 , then do only look for exact name matches
    // the return value will be the element(!) index , i.e. the index of the element in the data vector
    //
    long findElement(const char *namePartial, int fullName, long *N, const char **fieldName, int *more, int *fieldIdx=NULL);

    // get data from input field (previously found by findInputField())
    // stores nInput elements in **_in, the memory is allocated if necessary (i.e. *_in == NULL)
    int getInputFieldData(const FLOAT_DMEM *src, long Nsrc, FLOAT_DMEM **in);

    virtual void myFetchConfig() override;
    virtual void mySetEnvironment() override;

    virtual int myRegisterInstance(int *runMe=NULL) override;
    virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    
    //---- *runMe return value for component manager : 0, don't call my tick of this component, 1, call myTick
    virtual int runMeConfig() { return 1; }

    //---- HOOKs during configure (for setting level parameters, buffersize/type, blocksize requirements)
    // overwrite this, to configure the writer AFTER the reader config is available
    // possible return values (others will be ignored): 
    // -1 : configureWriter has overwritten c->nT value for the buffersize, myConfigureInstance will not touch nT !
    virtual int configureWriter(sDmLevelConfig &c) { return 1; }
    
    // configure reader, i.e. setup matrix reading, blocksize requests etc...
    // c: currently existing reader config
    virtual int configureReader(const sDmLevelConfig &c);

    //---- HOOKs during finalise (for setting names)
    virtual int dataProcessorCustomFinalise() { return 1; }

    virtual int setupNewNames(long nEl);

    // clone the input field info of field 'sourceIdx' to target field 'targetIdx' (can be -1 for last field..)
    // force = 1: overwrite existing data (which was set by custom code, for example)
    virtual int cloneInputFieldInfo(int sourceFidx, int targetFidx=-1, int force = 0);

    virtual int setupNamesForField(int i, const char*name, long nEl);
    virtual void configureField(int idxi, long N, long nOut) {}

    virtual void setEOI() override {
      cSmileComponent::setEOI();
      if (reader_!=NULL) reader_->setEOI();
      if (writer_!=NULL) writer_->setEOI();
    }

    virtual void unsetEOI() override {
      cSmileComponent::unsetEOI();
      if (reader_!=NULL) reader_->unsetEOI();
      if (writer_!=NULL) writer_->unsetEOI();
    }

  public:
    SMILECOMPONENT_STATIC_DECL
 
    virtual int setEOIcounter(int cnt) override {
      if (reader_ != NULL) {
        reader_->setEOIcounter(cnt);
      }
      if (writer_ != NULL) {
        writer_->setEOIcounter(cnt);
      }
      return cSmileComponent::setEOIcounter(cnt);
    }
 
    cDataProcessor(const char *_name);
    virtual ~cDataProcessor();
};




#endif // __DATA_PROCESSOR_HPP
