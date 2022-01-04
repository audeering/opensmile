/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

on-line semi-tone filter bank

*/

// TODO: this should be a WinToVecProcessor and NOT a DataProcessor !


#include <lld/tonefilt.hpp>
//#include <math.h>

#define MODULE "cTonefilt"


SMILECOMPONENT_STATICS(cTonefilt)

SMILECOMPONENT_REGCOMP(cTonefilt)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CTONEFILT;
  sdescription = COMPONENT_DESCRIPTION_CTONEFILT;

  // we inherit cDataProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("nameAppend", NULL, "tonefilt");
    ct->setField("nNotes","The number of semi-tone filters in the bank",48);
    ct->setField("firstNote","The frequency of the first note in Hz",55.0);
    ct->setField("decayF0","The gliding average decay coefficient for the first note (lowest frequency)",0.9995);
    ct->setField("decayFN","The gliding average decay coefficient for the last note (highest frequency) (must be < decayF0!); decay coefficents for intermediate frequencies will be interpolated linearly from the start and end coefficients.",0.998);
    ct->setField("outputPeriod","Specifies the period at which to produce output frames in seconds",0.1);
    //ct->setField("outputBuffersize","size of output buffer in frames (0=determine automatically)",0);
  )

  SMILECOMPONENT_MAKEINFO(cTonefilt);
}

SMILECOMPONENT_CREATE(cTonefilt)

//-----

cTonefilt::cTonefilt(const char *_name) :
  cDataProcessor(_name),
  outputPeriod(0.0),
  //outputBuffersize(0),
  tmpFrame(NULL),
  tmpVec(NULL),
  freq(NULL),
  pos(NULL),
  corrC(NULL), corrS(NULL),
  decayF(NULL)
{

}

void cTonefilt::myFetchConfig()
{
  cDataProcessor::myFetchConfig();
  
  outputPeriod = getDouble("outputPeriod");
  if (outputPeriod <= 0.0) outputPeriod = 0.1;
  SMILE_IDBG(2,"outputPeriod = %f s",outputPeriod);

  decayFN = getDouble("decayFN");
  if (decayFN < 0.0) decayFN = 0.0;
  if (decayFN > 1.0) decayFN = 1.0;
  SMILE_IDBG(2,"decayFN = %f",decayFN);

  decayF0 = getDouble("decayF0");
  if (decayF0 < decayFN) decayF0 = decayFN;
  if (decayF0 < 0.0) decayF0 = 0.0;
  if (decayF0 > 1.0) decayF0 = 1.0;
  SMILE_IDBG(2,"decayF0 = %f",decayF0);

  firstNote = getDouble("firstNote");
  if (firstNote <= 0.0) firstNote = 1.0;
  SMILE_IDBG(2,"firstNote = %f Hz",firstNote);

  nNotes = getInt("nNotes");
  if (nNotes < 1) nNotes = 1;
  SMILE_IDBG(2,"nNotes = %i",nNotes);

  /*
  if (isSet("outputBuffersize")) {
    outputBuffersize = getInt("outputBuffersize");
    SMILE_IDBG(2,"outputBuffersize = %i frames",outputBuffersize);
  }
  */
}


int cTonefilt::configureWriter(sDmLevelConfig &c)
{
  SMILE_IDBG(3,"reader period = %f",c.T);

  // c.T is input period here...
  if (c.T != 0.0) {
    outputPeriodFrames = (long)round(outputPeriod / c.T);
  } else {
    outputPeriodFrames = (long)round(outputPeriod);
  }

/*
  if (!fsfGiven) frameSizeFrames = (long)round(frameSize / c->T);
  else frameSize = ((double)frameSizeFrames) * c->T;
  if (!fstfGiven) frameStepFrames = (long)round(frameStep / c->T);
  else frameStep = ((double)frameStepFrames) * c->T;
  SMILE_IDBG(4,"computed frameSizeFrames = %i",frameSizeFrames);
  SMILE_IDBG(4,"computed frameStepFrames = %i",frameStepFrames);
*/

  if (outputPeriod < c.T) {
    outputPeriod = c.T;
    outputPeriodFrames = 1;
  }
  if (outputPeriodFrames < 0) outputPeriodFrames = 0;

  c.T = outputPeriod;
  c.frameSizeSec = outputPeriod;
  
  reader_->setupSequentialMatrixReading(outputPeriodFrames, outputPeriodFrames);

  // you must return 1, in order to indicate configure success (0 indicated failure)
  return 1;
}


int cTonefilt::setupNewNames(long nEl)
{
  // get reader names, append tonefilt to them, and set writer names
  N = reader_->getLevelN();
  Nf = reader_->getLevelNf();
  inputPeriod = reader_->getLevelT();
  int i,n;
  char *xx;

  corrC = (double**)calloc(1,sizeof(double*)*N);
  corrS = (double**)calloc(1,sizeof(double*)*N);

  for (i=0; i<Nf; i++) {
    int __N=0;
    const char *tmp = reader_->getFieldName(i,&__N);
    if (tmp == NULL) { SMILE_IERR(1,"reader->getFieldName(%i) failed (return value = NULL)!",i); return 0; }
    if (__N > 1) {
      for (n=0; n<__N; n++) {
        if ((nameAppend_!=NULL)&&(strlen(nameAppend_)>0))
          xx = myvprint("%s%i_%s",tmp,n,nameAppend_);
        else
          xx = myvprint("%s%i",tmp,n);
//        char *xx = myvprint("%s%i_frame",tmp,n);  // TODO: append name of winFunc, if option in config file is set
        writer_->addField( xx, nNotes );
        free(xx);
      }
    } else {
      if ((nameAppend_!=NULL)&&(strlen(nameAppend_)>0))
        xx = myvprint("%s_%s",tmp,nameAppend_);
      else
        xx = myvprint("%s",tmp);
//      char *xx = myvprint("%s_frame",tmp);  // TODO: append name of winFunc, if option in config file is set
      writer_->addField( xx, nNotes );
      free(xx);
    }
  }
  
  for (i=0; i<N; i++) {
    // initialize buffers for each element we process...
    corrC[i] = (double*)calloc(1,sizeof(double)*nNotes);
    corrS[i] = (double*)calloc(1,sizeof(double)*nNotes);
  }
  // initialize global buffers:
  freq = (double*)malloc(sizeof(double)*nNotes);
  decayF = (double*)malloc(sizeof(double)*nNotes);  // decay coefficient for each note
  for (n=0; n<nNotes; n++) {
    freq[n] = firstNote * pow(2.0,(double)n / 12.0); // freq. for each note
  }
  for (n=0; n<nNotes; n++) {
    // TODO!!!!!!!!!!
    //decayF[n] = decayFN + (decayF0-decayFN) * ((double)nNotes/12.0) / pow(2.0, (double)n/12.0); // TODO
    decayF[n] = decayFN + (decayF0-decayFN) * (freq[n]-freq[0])/(freq[nNotes-1]);
  }
  pos = (long *)calloc(1,sizeof(long)*N);
  
  namesAreSet_ = 1;
  if (tmpFrame==NULL) tmpFrame=(FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*nNotes);
  return 1;
}

/*
int cTonefilt::myFinaliseInstance()
{
  return cDataProcessor::myFinaliseInstance();
}
*/

void cTonefilt::doFilter(int i, cMatrix *row, FLOAT_DMEM*y)
{
  long n,t;
  double *s = corrS[i]; // sine
  double *c = corrC[i]; // cosine
  // TOOD: check for type data
  for (t=0; t<nNotes; t++) {
    double f= freq[t];
    FLOAT_DMEM *x = row->data;
    long idx = pos[i];
    for (n=0; n<row->nT; n++) {
      // compute correlation with sin+cos
      double time = (double)(idx+n) * inputPeriod;
      //      s[t] += sin(2.0*M_PI*f*t) * x[n];
      //      c[t] += cos(2.0*M_PI*f*t) * x[n];
      s[t] = decayF[t] * s[t] + (1.0 - decayF[t]) * sin(2.0*M_PI*f*time) * (double)x[n];
      c[t] = decayF[t] * c[t] + (1.0 - decayF[t]) * cos(2.0*M_PI*f*time) * (double)x[n];
    }
    y[t] = (FLOAT_DMEM)sqrt(c[t]*c[t] + s[t]*s[t]);  // sqrt(re^2 + im^2)
    y[t] *= 10.0;
  }
  pos[i] += row->nT;
}

eTickResult cTonefilt::myTick(long long t)
{
  SMILE_IDBG(5,"tick # %i, cTonefilt ....",t);

  if (!(writer_->checkWrite(1))) return TICK_DEST_NO_SPACE;

  // get next frame from dataMemory
  cMatrix *mat = reader_->getNextMatrix();
  if (mat == NULL) { return TICK_SOURCE_NOT_AVAIL; } // currently no data available

  if (tmpVec==NULL) tmpVec = new cVector(nNotes*N);
//  printf("vs=%i Nf=%i nn=%i\n",tmpVec->N,Nf,nNotes);
  
  int i;
  for (i=0; i<N; i++) {
    cMatrix *r = mat->getRow(i);

    doFilter(i,r,tmpFrame); 
    
    // copy data into main vector
    memcpy( tmpVec->data+i*nNotes, tmpFrame, sizeof(FLOAT_DMEM)*nNotes );
  }

  // generate new tmeta from first and last tmeta
  mat->squashTimeMeta();
  tmpVec->setTimeMeta(mat->tmeta);
  
  // save to dataMemory
  writer_->setNextFrame(tmpVec);

  return TICK_SUCCESS;
}


cTonefilt::~cTonefilt()
{
  if (tmpFrame!=NULL) free(tmpFrame);
  if (tmpVec!=NULL) delete tmpVec;
  if (freq!=NULL) free(freq);
  if (pos!=NULL) free(pos);
  if (decayF !=NULL) free(decayF);
  int i;
  if (corrS !=NULL) {
    for (i=0; i<N; i++) { if (corrS[i] != NULL) free(corrS[i]); }
    free(corrS);
  }
  if (corrC !=NULL) {
    for (i=0; i<N; i++) { if (corrC[i] != NULL) free(corrC[i]); }
    free(corrC);
  }
}

