/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

dataSink
write data to data memory...

*/
// TODO: derived class dataSinkChunk (receives turn start/end messages), -> dataSinkChunkFile (writes data to files)


#include <core/dataSink.hpp>

#define MODULE "cDataSink"

SMILECOMPONENT_STATICS(cDataSink)

SMILECOMPONENT_REGCOMP(cDataSink)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CDATASINK;
  sdescription = COMPONENT_DESCRIPTION_CDATASINK;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  if (ct->setField("reader", "The configuration of the cDataReader subcomponent, which handles the dataMemory interface for reading of input",
                  _confman->getTypeObj("cDataReader"), NO_ARRAY, DONT_FREE) == -1) {
     rA=1; // if subtype not yet found, request , re-register in the next iteration
  }
  
  ct->setField("blocksize", "The size of the data blocks to read at once, in frames (overwrites blocksize_sec, if set)", 0,0,0);
  ct->setField("blocksizeR", "The size of the data blocks to read at once, in frames (this overwrites blocksize and blocksize_sec!) (this option is provided for compatibility only... it is exactly the same as 'blocksize')", 0,0,0);
  ct->setField("blocksize_sec", "The size of the data blocks to read at once, in seconds", 0);
  ct->setField("blocksizeR_sec", "The size of the data blocks to read at once, in seconds (this overwrites blocksize_sec!) (this option is provided for compatibility only... it is exactly the same as 'blocksize')", 0,0,0);
  ct->setField("errorOnNoOutput","1 = show an error message if no output was written by this sink during this run.",0);
  SMILECOMPONENT_IFNOTREGAGAIN( {} )
  SMILECOMPONENT_MAKEINFO_ABSTRACT(cDataSink);
}

SMILECOMPONENT_CREATE_ABSTRACT(cDataSink)

//-----

cDataSink::cDataSink(const char *_name) :
  cSmileComponent(_name),
  reader_(NULL),
  blocksizeR_(1),
  blocksizeR_sec_(-1.0),
  nWritten_(0)
{
  char *tmp = myvprint("%s.reader",getInstName());
  reader_ = (cDataReader *)(cDataReader::create(tmp));
  if (reader_ == NULL) {
    COMP_ERR("Error creating dataReader '%s'",tmp);
  }
  if (tmp != NULL) free(tmp);
}

void cDataSink::mySetEnvironment()
{
  reader_->setComponentEnvironment(getCompMan(), -1, this);
}

void cDataSink::myFetchConfig()
{
  reader_->fetchConfig();
  
  blocksizeR_sec_ = getDouble("blocksize_sec");
  if ( (blocksizeR_sec_ <= 0.0) || (isSet("blocksizeR_sec")) ) {
    blocksizeR_sec_ = getDouble("blocksizeR_sec");
  }
  SMILE_IDBG(2,"blocksizeR (sec.) = %f",blocksizeR_sec_);
  blocksizeR_ = getInt("blocksize");
  if ( (blocksizeR_ <= 0) || (isSet("blocksizeR")) ) {
    blocksizeR_ = getInt("blocksizeR");
  }
  errorOnNoOutput_ = getInt("errorOnNoOutput");
}

int cDataSink::myRegisterInstance(int *runMe)
{
  int ret = reader_->registerInstance();
  if ((ret) && (runMe!=NULL)) {
    *runMe = runMeConfig();
  }
  return ret;
}

int cDataSink::configureReader() 
{ 
  reader_->setBlocksize(blocksizeR_);
  return 1; 
}

int cDataSink::myConfigureInstance()
{
  int ret = reader_->configureInstance();
  if (ret) {
    // convert blocksize options, so all options are accessible, if possible:
    // 1. blocksize values in frames override those in seconds:
    // 2. now do the inverse...
    double TT = reader_->getLevelT();
    if (blocksizeR_ > 0) {
      blocksizeR_sec_ = (double)blocksizeR_ * TT;
    } else if ((blocksizeR_sec_ > 0.0)&&(TT != 0.0)) {
      blocksizeR_ = (long) ceil (blocksizeR_sec_ / TT);
    } else {
      SMILE_IDBG(3,"using fallback blocksize of 1, because blocksize or blocksize_sec was not set in config!");
      blocksizeR_ = 1;
    }
    if (!configureReader()) {
      SMILE_IERR(1,"configureReader() returned 0 (failure)!");
      return 0;  
    }
  }
  return ret;
}

int cDataSink::myFinaliseInstance()
{
  return reader_->finaliseInstance();
}

cDataSink::~cDataSink()
{  
  if (errorOnNoOutput_ && nWritten_ == 0) {
    SMILE_IERR(1, "No output was written! (Maybe the input was too short to extract features from, or the config is broken?)");
  }
  if (reader_ != NULL) { delete reader_; }
}

