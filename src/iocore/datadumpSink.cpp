/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: datadumpSink

dump data in raw binary format (float)
the data can easily be loaded into matlab

The first float value will contain the vecsize
The second float value will contain the number of vectors
Then the matrix data follows float by float

*/



#include <iocore/datadumpSink.hpp>

#define MODULE "cDatadumpSink"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cDataSink::registerComponent(_confman);
}
*/

SMILECOMPONENT_STATICS(cDatadumpSink)

//sComponentInfo * cDatadumpSink::registerComponent(cConfigManager *_confman)
SMILECOMPONENT_REGCOMP(cDatadumpSink)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CDATADUMPSINK;
  sdescription = COMPONENT_DESCRIPTION_CDATADUMPSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("filename","The filename of the output file (if it doesn't exist it will be created).","datadump.dat");
    ct->setField("lag","output data <lag> frames behind",0,0,0);
    ct->setField("append","1 = append to an existing file, or create a new file; 0 = overwrite an existing file, or create a new file",0);
  )

  SMILECOMPONENT_MAKEINFO(cDatadumpSink);
}

SMILECOMPONENT_CREATE(cDatadumpSink)

//-----

cDatadumpSink::cDatadumpSink(const char *_name) :
  cDataSink(_name),
  filehandle(NULL),
  filename(NULL),
  nVec(0),
  vecSize(0)
{
}

void cDatadumpSink::myFetchConfig()
{
  cDataSink::myFetchConfig();
  
  filename = getStr("filename");
  SMILE_IDBG(3,"filename = '%s'",filename);

  lag = getInt("lag");
  SMILE_IDBG(3,"lag = %i",lag);

  append = getInt("append");
  if (append) { SMILE_IDBG(3,"append to file is enabled"); }
}

/*
int cDatadumpSink::myConfigureInstance()
{
  int ret=1;
  ret *= cDataSink::myConfigureInstance();
  // ....
  //return ret;
}
*/


int cDatadumpSink::myFinaliseInstance()
{
  int ap=0;
  float tmp=0;
  
  int ret = cDataSink::myFinaliseInstance();
  if (ret==0) return 0;
  
  if (append) {
    // check if file exists:
    filehandle = fopen(filename, "rb");
    if (filehandle != NULL) {
      // load vecsize, to see if it matches!
      if (fread(&tmp,sizeof(float),1,filehandle)) vecSize=(long)tmp;
      else vecSize = 0;
      // load initial nVec
      if (fread(&tmp,sizeof(float),1,filehandle)) nVec=(long)tmp;
      else nVec = 0;
      fclose(filehandle);
      filehandle = fopen(filename, "ab");
      ap=1;
    } else {
      filehandle = fopen(filename, "wb");
    }
  } else {
    filehandle = fopen(filename, "wb");
  }
  if (filehandle == NULL) {
    COMP_ERR("Error opening binary file '%s' for writing (component instance '%s', type '%s')",filename, getInstName(), getTypeName());
  }
  
  if (vecSize == 0) vecSize = reader_->getLevelN();

  if (!ap) {
    // write mini dummy header ....
    writeHeader();
  }
  
  return ret;
}


eTickResult cDatadumpSink::myTick(long long t)
{
  if (filehandle == NULL) return TICK_INACTIVE;
  
  SMILE_IDBG(4,"tick # %i, writing value vector (lag=%i):",t,lag);
  cVector *vec= reader_->getFrameRel(lag);  //new cVector(nValues+1);
  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;

  // now print the vector:
  int i; float *tmp = (float*)malloc(sizeof(float)*vec->N);
  if (tmp==NULL) OUT_OF_MEMORY;
  
  for (i=0; i<vec->N; i++) {
    tmp[i] = (float)(vec->data[i]);
  }

  int ret=1;
  if (!fwrite(tmp,sizeof(float),vec->N,filehandle)) {
    SMILE_IERR(1,"Error writing to raw feature file '%s'!",filename);
    ret = 0;
  } else {
    //reader->nextFrame();
    nVec++;
  }

  free(tmp);
  
  nWritten_++;

  return ret ? TICK_SUCCESS : TICK_INACTIVE;
}

// WARNING: write header changes file write pointer to beginning of file (after header)
void cDatadumpSink::writeHeader()
{
  // seek to beginning of file:
  fseek( filehandle, 0, SEEK_SET );
  // write header:
  float tmp;
  tmp = (float)vecSize;
  fwrite(&tmp, sizeof(float), 1, filehandle);
  tmp = (float)nVec;
  fwrite(&tmp, sizeof(float), 1, filehandle);
}

cDatadumpSink::~cDatadumpSink()
{
  // write final header 
  writeHeader();
  // close output file
  fclose(filehandle);
}

