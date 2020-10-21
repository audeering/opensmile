/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

dataProcessor
write data to data memory...

*/


#include <core/dataProcessor.hpp>

#define MODULE "cDataProcessor"

SMILECOMPONENT_STATICS(cDataProcessor)

SMILECOMPONENT_REGCOMP(cDataProcessor)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CDATAPROCESSOR;
  sdescription = COMPONENT_DESCRIPTION_CDATAPROCESSOR;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  if (ct->setField("reader", "The configuration of the cDataReader subcomponent, which handles the dataMemory interface for data input",
                  _confman->getTypeObj("cDataReader"), NO_ARRAY, DONT_FREE) == -1) {
     rA=1; // if subtype not yet found, request , re-register in the next iteration
  }
  if (ct->setField("writer", "The configuration of the cDataWriter subcomponent, which handles the dataMemory interface for data output",
                  _confman->getTypeObj("cDataWriter"), NO_ARRAY, DONT_FREE) == -1) {
     rA=1; // if subtype not yet found, request , re-register in the next iteration
  }

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("buffersize", "The buffer size for the output level in frames (default [0] = same as input level), this option overwrites 'buffersize_sec'", 0,0,0);
    ct->setField("buffersize_sec", "The buffer size for the output level in seconds (default [0] = same as input level)", 0);
    ct->setField("blocksize", "The size of data blocks to process in frames (this sets both blocksizeR and blocksizeW, and overwrites blocksize_sec)", 0,0,0);
    ct->setField("blocksizeR", "The size of data blocks to read in frames (overwrites blocksize)", 0,0,0);
    ct->setField("blocksizeW", "The size of data blocks to write in frames (overwrites blocksize)", 0,0,0);
    ct->setField("blocksize_sec", "size of data blocks to process in seconds (this sets both blocksizeR_sec and blocksizeW_sec)", 0.0);
    ct->setField("blocksizeR_sec", "size of data blocks to read in seconds (overwrites blocksize_sec!)", 0.0,0,0);
    ct->setField("blocksizeW_sec", "size of data blocks to write in seconds (overwrites blocksize_sec!)", 0.0,0,0);

    ct->setField("nameAppend", "A string suffix to append to the input field names (default: empty)", (const char*)NULL);
    ct->setField("copyInputName", "1 = copy the input name (and optionally append a suffix, see 'nameAppend' option), 0 = discard the input name and use only the 'nameAppend' string as new name.", 1);
    ct->setField("EOIlevel", "set the EOI counter threshold at which to act in EOI mode (for full input processing). Required e.g. for multi-level EOI chains to avoid running full input functionals/windows on incomplete first EOI iteration data.", 0);
  )
  
  SMILECOMPONENT_MAKEINFO_ABSTRACT(cDataProcessor);
}

SMILECOMPONENT_CREATE_ABSTRACT(cDataProcessor)

//-----

cDataProcessor::cDataProcessor(const char *_name) :
  cSmileComponent(_name),
  writer_(NULL),
  reader_(NULL),
  namesAreSet_(0),
  buffersize_sec_(0.0),
  blocksizeR_sec_(0.0), blocksizeW_sec_(0.0),
  buffersize_(0),
  blocksizeR_(0), blocksizeW_(0),
  nameAppend_(NULL),
  copyInputName_(0),
  nInput_(0), inputStart_(0), inputName_(NULL)
{
  // create the child instances, reader and writer
  char *tmp = myvprint("%s.reader",getInstName());
  reader_ = (cDataReader *)(cDataReader::create(tmp));
  if (reader_ == NULL) {
    COMP_ERR("Error creating dataReader '%s'",tmp);
  }
  if (tmp!=NULL) free(tmp);

  tmp = myvprint("%s.writer",getInstName());
  writer_ = (cDataWriter *)(cDataWriter::create(tmp));
  if (writer_ == NULL) {
    COMP_ERR("Error creating dataWriter '%s'",tmp);
  }
  if (tmp!=NULL) free(tmp);

  //( TODO: configure dateWriter level according to (descending priority)
  //   1. dataReader level clone
  //   2. application code (based on info from dataReader level)
  //   3. config file )
}

void cDataProcessor::myFetchConfig()
{
  reader_->fetchConfig();
  writer_->fetchConfig();

  buffersize_sec_ = getDouble("buffersize_sec");
  SMILE_IDBG(2,"buffersize (sec.) = %f",buffersize_sec_);
  buffersize_ = getInt("buffersize");
  SMILE_IDBG(2,"buffersize (frames.) = %i",buffersize_);
  double blocksize_sec = getDouble("blocksize_sec");
  blocksizeR_sec_ = blocksizeW_sec_ = blocksize_sec;
  if ( (blocksizeR_sec_ <= 0.0) || (isSet("blocksizeR_sec")) ) {
    blocksizeR_sec_ = getDouble("blocksizeR_sec");
  }
  if ( (blocksizeW_sec_ <= 0.0) || (isSet("blocksizeW_sec")) ) {
    blocksizeW_sec_ = getDouble("blocksizeW_sec");
  }
  SMILE_IDBG(2,"blocksizeR (sec.) = %f",blocksizeR_sec_);
  SMILE_IDBG(2,"blocksizeW (sec.) = %f",blocksizeW_sec_);
  long blocksize = getInt("blocksize");
  blocksizeR_ = blocksizeW_ = blocksize;
  if ( (blocksizeR_ <= 0) || (isSet("blocksizeR")) ) {
    blocksizeR_ = getInt("blocksizeR");
  }
  if ( (blocksizeW_ <= 0) || (isSet("blocksizeW")) ) {
    blocksizeW_ = getInt("blocksizeW");
  }
  SMILE_IDBG(2,"blocksizeR (frames, from config only) = %i",blocksizeR_);
  SMILE_IDBG(2,"blocksizeW (frames, from config only) = %i",blocksizeW_);
  nameAppend_ = getStr("nameAppend");
  if (nameAppend_ != NULL) { SMILE_IDBG(2,"nameAppend = '%s'",nameAppend_); }
  copyInputName_ = getInt("copyInputName");
  SMILE_IDBG(2,"copyInputName = %i",copyInputName_);
}

void cDataProcessor::mySetEnvironment()
{
  writer_->setComponentEnvironment(getCompMan(), -1, this);
  reader_->setComponentEnvironment(getCompMan(), -1, this);
}

/* register both read and write request, order is arbitrary */
// TODO: *runMe config... ? from config file... or from a custom hook for base classes
int cDataProcessor::myRegisterInstance(int *runMe)
{
  int ret = 1;
  ret *= reader_->registerInstance();
  ret *= writer_->registerInstance();
  if ((ret)&&(runMe!=NULL)) {
    // call runMe config hook
    *runMe = runMeConfig();
  }
  return ret;
}

int cDataProcessor::configureReader(const sDmLevelConfig &c)
{ 
  int EOIlevel = getInt("EOIlevel");
  setEOIlevel(EOIlevel);
  reader_->setEOIlevel(EOIlevel);
  writer_->setEOIlevel(EOIlevel);
  reader_->setBlocksize(blocksizeR_);
  return 1; 
}

// Configures both reader and writer.
// Here, the reader has to be configured first, in order to be able to auto configure the writer
//  based on the input level parameters.
int cDataProcessor::myConfigureInstance()
{
  if (!(reader_->configureInstance())) return 0;
  // finalise the reader first. this makes sure that the names in the input level are set up correctly
  if (!(reader_->finaliseInstance())) return 0;

  // allow derived class to configure the writer AFTER the reader config is available
  // if the derived class returns 1, we will continue
  const sDmLevelConfig *c = reader_->getConfig();
  if (c==NULL) COMP_ERR("myConfigureInstance: Error getting reader dmLevel config! returned sDmLevelConfig = NULL!");

  // now copy config so that we can safely modify it...
  sDmLevelConfig c2(*c);

  // convert blocksize options, so all options are accessible, if possible:
  // 1. blocksize values in frames override those in seconds:
  // 2. now do the inverse...
  if (blocksizeW_ > 0) {
    blocksizeW_sec_ = (double)blocksizeW_ * c2.T;
  } else if ((blocksizeW_sec_ > 0.0)&&(c2.T != 0.0)) {
    blocksizeW_ = (long) ceil (blocksizeW_sec_ / c2.T);
  }

  if (blocksizeR_ > 0) {
    blocksizeR_sec_ = (double)blocksizeR_ * c2.T;
  } else if ((blocksizeR_sec_ > 0.0)&&(c2.T != 0.0)) {
    blocksizeR_ = (long) ceil (blocksizeR_sec_ / c2.T);
  } else {
    SMILE_IDBG(3,"using fallback blocksizeR of 1, because blocksizeR or blocksize_sec was not set in config!");
    blocksizeR_ = 1;
  }

  // if only blocksizeR was set instead of blocksize, auto set blocksizeW to blocksizeR
  // NOTE: if blocksizeW != 0 this auto setting will not be performed...
  if (blocksizeW_ <= 0) {
    blocksizeW_ = blocksizeR_;
    blocksizeW_sec_ = blocksizeR_sec_;
  }
  
  // set writer blocksize from "blocksizeW" config option
  c2.blocksizeWriter = blocksizeW_;
  long oldBsw = blocksizeW_;

  // allow custom config of reader... 
  // main purpose: "communicating" blocksizeR to dataMemory !
  if (!configureReader(c2)) {
    // NOTE: reader config (setup matrix reading /blocksize / etc. may also be performed in configureWriter!! 
    SMILE_IERR(1,"configureReader() returned 0 (failure)!");
    return 0;  
  }

  // provide a hook, to allow a derived component to modify the writer config we have obtained from the parent level:
  int ret = configureWriter(c2);
  // possible return values (others will be ignored): 
  // -1 : configureWriter has overwritten c->nT value for the buffersize, myConfigureInstance will not touch nT !
  // 0 : failure, myConfigure must exit with code 0
  // 1 : default / success
  if (!ret) {
    SMILE_IERR(1,"configureWriter() returned 0 (failure)!");
    return 0;
  }

  // since configure writer may modify either c2.blocksizeWriter OR blocksizeW alone, we sync again if they differ:
  if (c2.blocksizeWriter != oldBsw) {
    blocksizeW_ = c2.blocksizeWriter;
  } else if (blocksizeW_ != oldBsw) {
    c2.blocksizeWriter = blocksizeW_;
  }

  if (ret!=-1) {
    if (buffersize_ > 0) {
      c2.nT = buffersize_;
    } else if (buffersize_sec_ > 0.0) {
      if (c2.T != 0.0) {
        c2.nT = (long)ceil(buffersize_sec_/c2.T);
      } else {
        c2.nT = (long)ceil(buffersize_sec_);
      } 
    } // else, don't modify c2.nT : buffersize will be the same as in input level or as set by configureWriter!
  }

  writer_->setConfig(c2,0);
  return writer_->configureInstance();
}

// Adds a field with given parameters, concat base name and append string (if not NULL).
void cDataProcessor::addNameAppendField(const char*base, const char*append, int N, int arrNameOffset)
{
  char *xx;

  if  ((append != NULL)&&(strlen(append)>0)) {
    if ((base != NULL)&&(strlen(base)>0)) {
      xx = myvprint("%s_%s",base,append);
      writer_->addField( xx, N, arrNameOffset );
      free(xx);
    } else {
      writer_->addField( append, N, arrNameOffset );
    }
  } else {
    if ((base != NULL)&&(strlen(base)>0)) {
      writer_->addField( base, N, arrNameOffset );
    } else {
      writer_->addField( "noname", N, arrNameOffset );
    }
  }
}

// automatically append "nameAppend", also allow for a fixed cutom part of the name
// base is automatically appended or not depending on the value of copyInputName
void cDataProcessor::addNameAppendFieldAuto(const char*base, const char *customFixed, int N, int arrNameOffset)
{
  char *xx;

  if  ((nameAppend_ != NULL)&&(strlen(nameAppend_)>0)) {
    if ((customFixed != NULL)&&(strlen(customFixed)>0)) {

      if ((copyInputName_)&&(base != NULL)&&(strlen(base)>0)) {
        xx = myvprint("%s_%s%s",base,customFixed,nameAppend_);
        writer_->addField( xx, N, arrNameOffset );
        free(xx);
      } else {
        xx = myvprint("%s%s",customFixed,nameAppend_);
        writer_->addField( xx, N, arrNameOffset );
        free(xx);
      }

    } else {

      if ((copyInputName_)&&(base != NULL)&&(strlen(base)>0)) {
        xx = myvprint("%s_%s",base,nameAppend_);
        writer_->addField( xx, N, arrNameOffset );
        free(xx);
      } else {
        writer_->addField( nameAppend_, N, arrNameOffset );
      }

    }
  } else {
    if ((customFixed != NULL)&&(strlen(customFixed)>0)) {

      if ((copyInputName_)&&(base != NULL)&&(strlen(base)>0)) {
        xx = myvprint("%s_%s",base,customFixed);
        writer_->addField( xx, N, arrNameOffset );
        free(xx);
      } else {
        writer_->addField( customFixed, N, arrNameOffset );
      }


    } else {

      if ((copyInputName_)&&(base != NULL)&&(strlen(base)>0)) {
        writer_->addField( base, N, arrNameOffset );
      } else {
        writer_->addField( "noname", N, arrNameOffset );
      }

    }

  }
}

// get the size of input frames in seconds
double cDataProcessor::getFrameSizeSec()
{
  const sDmLevelConfig *c = reader_->getLevelConfig();
  return (double)(c->frameSizeSec);
}

// get the period of the base level (e.g. sampling rate for wave input)
double cDataProcessor::getBasePeriod()
{
  const sDmLevelConfig *c = reader_->getLevelConfig();
  return (double)(c->basePeriod);
}

//
// find a field in the input level by a part of its name or its full name
// set the internal variables nInput, inputStart, and inputName
//
// nEl specifies the maximum number of input elements (for checking valid range of field index)
// (optional) fullName = 1: match full field name instead of part of name
long cDataProcessor::findInputField(const char *namePartial, int fullName, long nEl)
{
  int more = 0;
  inputStart_ = findField(namePartial, fullName, &nInput_, &inputName_, nEl, &more, NULL);
  return inputStart_;
  // TODO: make this function use findField!
  /*
  const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();
  int ri=0;
  long idx;
  if (fullName) {
    idx = fmeta->findField( namePartial , &ri );
  } else {
    idx = fmeta->findFieldByPartialName( namePartial , &ri );
  }
  if (nEl <= 0) nEl = reader_->getLevelN();
  if (idx < 0) {
    nInput_ = nEl;
    inputStart_ = 0;
    inputName_ = NULL;
    SMILE_IWRN(2,"Requested input field '*%s*' not found, defaulting to use full input vector!",namePartial);
  } else {
    inputStart_ = fmeta->fieldToElementIdx( idx ) + ri;;
    nInput_ = fmeta->field[idx].N;
    inputName_ = fmeta->field[idx].name;
  }

  if (nInput_+inputStart_ > nEl) nInput_ = nEl-inputStart_;
  if (inputStart_ < 0) inputStart_ = 0;
  return inputStart_;
  */
}

//
// find a field in the input level by a part of its name or its full name
//
// return value: index of first element of field
//
// *namePartial gives the name/partial to search for
// nEl specifies the maximum number of input elements (for checking valid range of field index)
//     -1 will enable autodetection from reader_->getLevelN();
// *N , optional, a pointer to variable (long) that will be filled with the number of elements in the field
// **_fieldName : will be set to a pointer to the name of the field
// (optional) fullName = 1: match full field name instead of part of name
// *more: will be set to > 0 if more than one field matches the search expression
//        the index of the current field will be the value of *more
// *fieldIdx: index of field (not element!)
long cDataProcessor::findField(const char *namePartial, int fullName, long *N,
    const char **fieldName, long nEl, int *more, int *fieldIdx)
{
  const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();
  if (fmeta == NULL)
    return -1;
  int ri=0;
  long idx;
  if (fullName) {
    idx = fmeta->findField( namePartial , &ri, more );
  } else {
    idx = fmeta->findFieldByPartialName( namePartial , &ri, more );
  }
  long nInput, inputStart;
  const char *inputName;
  if (nEl <= 0)
    nEl = reader_->getLevelN();
  if (idx < 0) {
    nInput = nEl;
    inputStart = 0;
    inputName = NULL;
    if (fullName) {
      SMILE_IWRN(4,"Requested input field '%s' not found, check your config! Defaulting to use first field. Available fields:", namePartial);
    } else {
      SMILE_IWRN(4,"Requested input field matching pattern '*%s*' not found, check your config! Defaulting to use first field. Available fields:", namePartial);
    }
    if (SMILE_LOG_GLOBAL != NULL && SMILE_LOG_GLOBAL->getLogLevel_wrn() >= 4)
      fmeta->printFieldNames();
    //SMILE_IWRN(2,"Requested input field '*%s*' not found, defaulting to use full input vector!",namePartial);
  } else {
    inputStart = fmeta->fieldToElementIdx( idx ) + ri;
    nInput = fmeta->field[idx].N;
    inputName = fmeta->field[idx].name;
  }
  // FIXME: this function should return -1 if the field is not found and everyone using
  //  this function should check for a valid index!

  if ((nInput+inputStart > nEl)&&(nEl>0)) nInput = nEl-inputStart;
  if (inputStart < 0) inputStart = 0;

  if (fieldIdx != NULL) *fieldIdx = idx;
  if (N != NULL) *N = nInput;
  if (fieldName != NULL) *fieldName = inputName;
  return inputStart;
}

//
// search for an element by its partial name (if multiple matches are found, only the first is returned)
// if fullName==1 , then do only look for exact name matches
// the return value will be the element(!) index , i.e. the index of the element in the data vector
//
long cDataProcessor::findElement(const char *namePartial,
    int fullName, long *N, const char **fieldName, int *more, int *fieldIdx)
{
  const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();
  if (fmeta == NULL)
    return -1;
  int ri=0;
  long idx;
  if (fullName) {
    idx = fmeta->findField( namePartial , &ri, more );
  } else {
    idx = fmeta->findFieldByPartialName( namePartial , &ri, more );
  }
  long elIdx=-1;
  const char *inputName;
  long nInput;
  if (idx < 0) {
    inputName = NULL;
    nInput = 0;
    if (fullName) {
      SMILE_IWRN(4,"Requested input element '%s' not found, check your config! Available fields:", namePartial);
    } else {
      SMILE_IWRN(4,"Requested input element matching pattern '*%s*' not found, check your config! Available fields:", namePartial);
    }
    if (SMILE_LOG_GLOBAL != NULL && SMILE_LOG_GLOBAL->getLogLevel_wrn() >= 4)
      fmeta->printFieldNames();
  } else {
    elIdx = fmeta->fieldToElementIdx( idx ) + ri;
    nInput = fmeta->field[idx].N;
    inputName = fmeta->field[idx].name;
  }

  if (fieldIdx != NULL) *fieldIdx = idx;
  if (N != NULL) *N = nInput;
  if (fieldName != NULL) *fieldName = inputName;
  return elIdx;
}


// get data from input field (previously found by findInputField())
// stores nInput elements in **_in, the memory is allocated if necessary (i.e. *_in == NULL)
int cDataProcessor::getInputFieldData(const FLOAT_DMEM *src, long Nsrc, FLOAT_DMEM **in)
{
  if (nInput_ <= 0) return 0;
  if (in != NULL) {
    if (*in == NULL) *in = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*nInput_);
    long i;
    for (i=0; i<nInput_; i++) {
      long j = inputStart_ + i;
      if (j<Nsrc) (*in)[i] = src[j];
      else {
        SMILE_IERR(3,"out of range index in getInputFieldData (j=%i , inputStart=%i, Nsrc=%s)",j,inputStart_,Nsrc);
        (*in)[i] = 0.0;
      }
    }
    return 1;
  }
  return 0;
}


int cDataProcessor::setupNewNames(long nEl)
{
	// if you overwrite this method, uncomment the following line in your own class'es code:
	// namesAreSet_ = 1;
   return 1;
}

int cDataProcessor::setupNamesForField(int i, const char*name, long nEl)
{
  if (copyInputName_) {
    addNameAppendField( name, nameAppend_, nEl );
  } else {
    addNameAppendField( NULL, nameAppend_, nEl );
  }
  return nEl;
}

int cDataProcessor::cloneInputFieldInfo(int sourceFidx, int targetFidx, int force)
{
  const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();
  if ((fmeta != NULL) && (sourceFidx < fmeta->N)) {
    const FrameMetaInfo * fmetaW = writer_->getFrameMetaInfo();
    int isset = 0;
    if (fmetaW != NULL) {
      if ((fmetaW->N > 0) && (fmetaW->field[fmetaW->N-1].infoSet)) isset = 1;
      if (!isset || force) {
        if (fmeta->field[sourceFidx].infoSize > 0) { // TODO: check why we had infoSize==0 here!! (valgrind leak check revealed this...)
          void * _info = malloc(fmeta->field[sourceFidx].infoSize);
          memcpy(_info, fmeta->field[sourceFidx].info, fmeta->field[sourceFidx].infoSize);
          writer_->setFieldInfo(targetFidx, fmeta->field[sourceFidx].dataType, _info , fmeta->field[sourceFidx].infoSize );
        }
      }
      return 1;
    }
  }
  return 0;
}

/* finalise reader an writer
   first finalise the reader, so we can read field/element names from it
   then set names for the writer
   then finalise the writer...
 */
int cDataProcessor::myFinaliseInstance()
{
  // an extra hook to allow custom configure of e.g. vectorProcessors, etc. without overwriting myFinaliseInstance!
  if (!dataProcessorCustomFinalise()) {
    SMILE_IERR(1,"dataProcessorCustomFinalise returned 0 (failure) !");
    return 0;
  }

  /* available hooks for setting data element names in the output level:
    1. setupNewNames() : you are free to set any names in the output level, independent of the names in the input level
       (configureField will not be called! you must do everything in setupNewNames!)

    2. setupNamesForField() : this will be called for every input field(!), use this if you only want to append a suffix or change the number of elements in each field!
           setupNamesFor filed must return the number of elements that were set for the current field
       + configureField() : custom configuration (e.g. allocation of buffers, etc.) that the component must perform shall be put into this method!

    a hook that actually sets names MUST set the variable "namesAreSet" to 1

    if no hook sets the names, the names will be copied from the input level, and the suffix "nameAppend" will be appended!
  */

  // hook 1.
  if (!namesAreSet_) {
    if (!setupNewNames(reader_->getLevelNf())) {
      SMILE_IERR(1,"setupNewNames() returned 0 (failure)!");
      return 0;
    }
  }

  // hook 2.
  if (!namesAreSet_) {
    int lN = reader_->getLevelNf();
    int i;
    for (i=0; i<lN; i++) {
      int llN=0;
      int arrNameOffset=0;
      const char *tmp = reader_->getFieldName(i,&llN,&arrNameOffset);
      long nOut = setupNamesForField(i,tmp,llN);
      if (nOut==llN) {
  		  writer_->setArrNameOffset(arrNameOffset);
	    }
      configureField(i,llN,nOut);
      cloneInputFieldInfo(i, -1, 0); //fallback... no overwrite
    }
    namesAreSet_ = 1;
  }
  
  return writer_->finaliseInstance();
}

cDataProcessor::~cDataProcessor()
{
  if (writer_ != NULL) { delete writer_; }
  if (reader_ != NULL) { delete reader_; }
}


