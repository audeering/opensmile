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

#include <iocore/htkSource.hpp>
#define MODULE "cHtkSource"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cDataSource::registerComponent(_confman);
}
*/
#define N_ALLOC_BLOCK 50

SMILECOMPONENT_STATICS(cHtkSource)

SMILECOMPONENT_REGCOMP(cHtkSource)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CHTKSOURCE;
  sdescription = COMPONENT_DESCRIPTION_CHTKSOURCE;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("filename","HTK parameter file to read","input.htk");
    ct->setField("featureName","The name of the array-field which is to be created in the data memory output level for the data array read from the HTK file","htkpara");
    ct->setField("featureFrameSize","The size of the feature frames in seconds.",0.0);
    ct->setField("forceSampleRate","Set a given sample rate for the output level. Typically the base period of the input level will be used for this purpose, but when reading frame-based data from feature files, for example, this information is not available. This option overwrites the input level base period, if it is set.",16000.0);
    ct->setField("blocksize", "The size of data blocks to write at once (to data memory) in frames", 10);
//    ct->setField("featureNames","array of feature names to apply (must match the vector size in the HTK parameter file!)","htkpara",ARRAY_TYPE);
  )

  SMILECOMPONENT_MAKEINFO(cHtkSource);
}

SMILECOMPONENT_CREATE(cHtkSource)

//-----

cHtkSource::cHtkSource(const char *_name) :
  cDataSource(_name),
  eof(0),
  featureName(NULL),
  tmpvec(NULL)
{
  vax = smileHtk_IsVAXOrder();
}

void cHtkSource::myFetchConfig()
{
  cDataSource::myFetchConfig();
  
  filename = getStr("filename");
  SMILE_IDBG(2,"filename = '%s'",filename);
  featureName = getStr("featureName");
  SMILE_IDBG(2,"featureName = '%s'",featureName);
}


int cHtkSource::myConfigureInstance()
{
  int ret = 1;
  filehandle = fopen(filename, "rb");
  if (filehandle == NULL) {
    COMP_ERR("Error opening file '%s' for reading (component instance '%s', type '%s')",filename, getInstName(), getTypeName());
  }

  if (!readHeader()) {
    ret = 0;
  } else {
    ret *= cDataSource::myConfigureInstance();
  }

  if (ret == 0) {
    fclose(filehandle); filehandle = NULL;
  }
  return ret;
}

/*
int cHtkSource::readHeader()
{
  if (filehandle==NULL) return 0;
  if (!fread(&head, sizeof(sHTKheader), 1, filehandle)) {
    SMILE_IERR(1,"error reading header from file '%s'",filename);
    return 0;
  }
  prepareHeader(&head); // convert to host byte order
  return 1;
}
*/

int cHtkSource::configureWriter(sDmLevelConfig &c)
{
  c.T = ( (double)head.samplePeriod ) * 0.000000100;  // convert HTK 100ns units..

  if (isSet("forceSampleRate")) {
    double sr = getDouble("forceSampleRate");
    if (sr > 0.0) {
      c.basePeriod = 1.0/sr;
    } else {
      c.basePeriod = 1.0;
      SMILE_IERR(1,"sample rate (forceSampleRate) must be > 0! (it is: %f)",sr);
    }
  }
  
  if (isSet("featureFrameSize")) {
    c.frameSizeSec = getDouble("featureFrameSize");
    c.lastFrameSizeSec = c.frameSizeSec;
  } 

  c.noTimeMeta = true;

  return 1;
}

int cHtkSource::setupNewNames(long nEl)
{
  N = head.sampleSize/sizeof(float);
  writer_->addField(featureName,N);

  allocVec(N);
  tmpvec = (float *)malloc(sizeof(float)*N);

  namesAreSet_=1;
  return 1;
}

int cHtkSource::myFinaliseInstance()
{
  int ret = cDataSource::myFinaliseInstance();
  return ret;
}


eTickResult cHtkSource::myTick(long long t)
{
  if (isEOI()) return TICK_INACTIVE;

  SMILE_IDBG(4,"tick # %i, reading value vector from HTK parameter file",t);
  if (eof) {
    SMILE_IDBG(4,"EOF, no more data to read");
    return TICK_INACTIVE;
  }

  long n;
  for (n=0; n<blocksizeW_; n++) {

    // check if there is enough space in the data memory
    if (!(writer_->checkWrite(1))) return TICK_DEST_NO_SPACE;


    if (fread(tmpvec, head.sampleSize, 1, filehandle)) {
      long i;
      if (vax) {
        for (i=0; i<vec_->N; i++) {
          smileHtk_SwapFloat ( (tmpvec+i) );
          vec_->data[i] = (FLOAT_DMEM)tmpvec[i];
        }
      } else {
        for (i=0; i<vec_->N; i++) {
          vec_->data[i] = (FLOAT_DMEM)tmpvec[i];
        }
      }
    } else {
      // EOF ??
      eof = 1;
    } 


    if (!eof) {
      writer_->setNextFrame(vec_);
      return TICK_SUCCESS;
    } else {
      return TICK_INACTIVE;
    }
  }
  return TICK_INACTIVE;
}


cHtkSource::~cHtkSource()
{
  if (filehandle!=NULL) fclose(filehandle);
  if (tmpvec != NULL) free(tmpvec);
}
