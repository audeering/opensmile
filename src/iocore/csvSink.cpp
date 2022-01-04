/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Comma Separated Value file output (CSV)

*/



#include <iocore/csvSink.hpp>

#define MODULE "cCsvSink"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cDataSink::registerComponent(_confman);
}
*/

SMILECOMPONENT_STATICS(cCsvSink)

//sComponentInfo * cCsvSink::registerComponent(cConfigManager *_confman)
SMILECOMPONENT_REGCOMP(cCsvSink)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CCSVSINK;
  sdescription = COMPONENT_DESCRIPTION_CCSVSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")

  SMILECOMPONENT_IFNOTREGAGAIN_BEGIN
    ct->setField("filename","The CSV file to write to. An empty filename (or '?' as the filename) disables this sink component.","smileoutput.csv");
    ct->setField("delimChar","The column delimiter character to use (usually ',' or ';') (NOTE: use '<space>' or '<tab>' for these special characters respectively)",';');
    ct->setField("lag","output data <lag> frames behind",0,0,0);
    ct->setField("append","1 = append to an existing file, or create a new file; 0 = overwrite an existing file, or create a new file",0);
    // for compatibility with arffSink
    // for compatibility with arffSink
    ct->setField("frameIndex", "(same as 'number') 1 = print an instance number (= frameIndex) attribute for each output frame (1/0 = yes/no)", 1);
    ct->setField("number", "1 = print an instance number (= frameIndex) attribute for each output frame (1/0 = yes/no)", 1);
    ct->setField("frameLength","1 = print a frame length attribute (1/0 = yes/no).",0);
    ct->setField("frameTime","(same as 'timestamp') 1 = print a timestamp attribute for each output frame (1/0 = yes/no)", 1);
    ct->setField("timestamp","1 = print a timestamp attribute for each output frame (1/0 = yes/no)", 1);
    ct->setField("printHeader","1 = print a header line as the first line in the CSV file. This line contains the attribute names separated by the delimiter character.",1);
    ct->setField("flush", "1 = flush data to file after every line written (might give low performance for small lines!).", 0);
    ct->setField("instanceBase", "if not empty, print instance name attribute <instanceBase_Nr>", (const char*)NULL);
    ct->setField("instanceName", "if not empty, print instance name attribute <instanceName>", (const char*)NULL);

  SMILECOMPONENT_IFNOTREGAGAIN_END

  SMILECOMPONENT_MAKEINFO(cCsvSink);
}

SMILECOMPONENT_CREATE(cCsvSink)

//-----

cCsvSink::cCsvSink(const char *_name) :
  cDataSink(_name),
  filehandle(NULL),
  filename(NULL),
  printHeader_(0),
  delimChar_(';'),
  instanceName(NULL),
  instanceBase(NULL),
  prname_(0),
  disabledSink_(false)
{
}

void cCsvSink::myFetchConfig()
{
  cDataSink::myFetchConfig();
  filename = getStr("filename");
  if (filename == NULL || *filename == 0 || (*filename == '?' && *(filename+1) == 0)) {
    SMILE_IMSG(2, "No filename given, disabling this sink component.");
    disabledSink_ = true;
    errorOnNoOutput_ = 0;
  }
  delimChar_ = getChar("delimChar");
  lag = getInt("lag");
  append_ = getInt("append");
  if (append_) { SMILE_IDBG(3,"append to file is enabled"); }
  printHeader_ = getInt("printHeader");
  if (printHeader_) { SMILE_IDBG(3,"printing header with feature names"); }

  frameLength_ = (getInt("frameLength") == 1);
  number_ = (getInt("number") == 1);
  if (isSet("frameIndex")) {
    number_ = (getInt("frameIndex") == 1);
  }
  timestamp_ = (getInt("timestamp") == 1);
  if (isSet("frameTime")) {
    timestamp_ = (getInt("frameTime") == 1);
  }
  flush_ = getInt("flush");
  if (isSet("instanceBase")) {
    instanceBase = getStr("instanceBase");
    prname_ = 2;
  }
  if (isSet("instanceName")) {  // instance name overrides instance base
    instanceName = getStr("instanceName");
    prname_ = 1;
  }
}

/*
int cCsvSink::myConfigureInstance()
{
  int ret=1;
  ret *= cDataSink::myConfigureInstance();
  // ....
  //return ret;
}
*/


int cCsvSink::myFinaliseInstance()
{
  int ap=0;
  // we need to finalise data sink first, in
  // order to ensure that parameters are correctly
  // set up for reading, as we will read data, even
  // if the sink is disabled.
  int ret = cDataSink::myFinaliseInstance();
  if (ret == 0)
    return 0;
  if (disabledSink_) {
    filehandle = NULL;
    return ret;
  }

  if (append_) {
    // check if file exists:
    filehandle = fopen(filename, "r");
    if (filehandle != NULL) {
      fclose(filehandle);
      filehandle = fopen(filename, "a");
      ap=1;
    } else {
      filehandle = fopen(filename, "w");
    }
  } else {
    filehandle = fopen(filename, "w");
  }
  if (filehandle == NULL) {
    COMP_ERR("Error opening file '%s' for writing (component instance '%s', type '%s')",filename, getInstName(), getTypeName());
  }
  
  if ((!ap)&&(printHeader_)) {
    // write header ....
    if (prname_) {
      fprintf(filehandle, "name%c", delimChar_);
    }
    if (number_) {
      fprintf(filehandle, "frameIndex%c", delimChar_);
    }
    if (timestamp_) {
      fprintf(filehandle, "frameTime%c", delimChar_);
    }
    if (frameLength_) {
      fprintf(filehandle, "frameLength%c", delimChar_);
    }

    long N = reader_->getLevelN();
    long i;
    for(i=0; i<N-1; i++) {
      char *tmp = reader_->getElementName(i);
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-overflow"
#endif
      fprintf(filehandle, "%s%c",tmp,delimChar_);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
      free(tmp);
    }
    char *tmp = reader_->getElementName(i);
    fprintf(filehandle, "%s%s",tmp,NEWLINE);
    free(tmp);
  }
  
  return ret;
}


eTickResult cCsvSink::myTick(long long t)
{
  // NOTE: we must read the vector in any case to prevent blocking
  cVector *vec = reader_->getFrameRel(lag);
  if (vec == NULL)
    return TICK_SOURCE_NOT_AVAIL;
  if (filehandle == NULL)
    return TICK_INACTIVE;
  SMILE_IDBG(4, "tick # %i, writing to CSV file (lag=%i) (vec = %ld):", 
    t, lag, long(vec));
  long vi = vec->tmeta->vIdx;
  double tm = vec->tmeta->time;
  if (prname_ == 1) {
    fprintf(filehandle, "'%s'%c", instanceName, delimChar_);
  } else if (prname_ == 2) {
    fprintf(filehandle, "'%s_%ld'%c", instanceBase, vi, delimChar_);
  }

  if (number_) 
    fprintf(filehandle,"%ld%c", vi, delimChar_);
  if (timestamp_) 
    fprintf(filehandle,"%f%c", tm, delimChar_);
  if (frameLength_)
    fprintf(filehandle, "%f%c", vec->tmeta->lengthSec, delimChar_);

  // now print the vector:
  int i;
  for (i=0; i<vec->N-1; i++) {
    // print float as integer if its decimals are zero
    if (vec->data[i] == floor(vec->data[i])) {
      fprintf(filehandle,"%.0f%c",vec->data[i],delimChar_);
    } else {
      fprintf(filehandle,"%e%c",vec->data[i],delimChar_);
    }
  }
  if (vec->data[i] == floor(vec->data[i])) {
    fprintf(filehandle,"%0.f%s",vec->data[i],NEWLINE);
  } else {
    fprintf(filehandle,"%e%s",vec->data[i],NEWLINE);
  }
  if (flush_) {
    fflush(filehandle);
  }
  nWritten_++;

  return TICK_SUCCESS;
}


cCsvSink::~cCsvSink()
{
  if (filehandle != NULL) {
    fclose(filehandle);
    filehandle = NULL;
  }
}

