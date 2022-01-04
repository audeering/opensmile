/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

reads in windows and outputs vectors (frames)

*/


#include <core/vecToWinProcessor.hpp>

#define MODULE "cVecToWinProcessor"


SMILECOMPONENT_STATICS(cVecToWinProcessor)

SMILECOMPONENT_REGCOMP(cVecToWinProcessor)
{
  SMILECOMPONENT_REGCOMP_INIT
  
  scname = COMPONENT_NAME_CVECTOWINPROCESSOR;
  sdescription = COMPONENT_DESCRIPTION_CVECTOWINPROCESSOR;

  // we inherit cDataProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor");
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->disableField("blocksize");
    ct->disableField("blocksizeR");
    ct->disableField("blocksizeW");
    ct->disableField("blocksize_sec");
    ct->disableField("blocksizeR_sec");
    ct->disableField("blocksizeW_sec");

    ct->setField("forceSampleRate","Set a given sample rate for the output level. Typically the base period of the input level will be used for this purpose, but when reading frame-based data from feature files, for example, this information is not available. This option overwrites the input level base period, if it is set.",16000.0);

    ct->setField("normaliseAdd","1/0 (on/off) : normalise frames before adding to eliminate envelope fluctuation artefacts and scaling artefacts. When this is enabled the output should always be correctly scaled to the range -1 and +1. If this is deactivated perfect reconstruction can only be guaranteed with root-raised cosine windows and 50 percent overlap.",0);
    ct->setField("useWinAasWinB","1 = use window A as window B (e.g. if win A is a root of window x function, e.g. root raised cosine). The 'windowB' must be left blank, and NO windower must be present between the ifft (or other processing) and this component. This component will internally apply window function A before doing the overlap add. (NOT YET IMPLEMENTED)",0);
    ct->setField("gain","A gain to apply to the output samples.",1.0);

    ct->setField("windowA","Name of cWindower component applied before transforming into the spectral domain. Leave empty to use constant window (=1).",(const char*)NULL);
    ct->setField("windowB","Name of cWindower component applied after transforming back into the time domain. Leave empty to use constant window (=1).",(const char*)NULL);

    ct->setField("processArrayFields","If turned on (1), process array fields individually. If turned off (0), treat the input vector as a single field/frame.",1);
    ct->setField("noPostEOIprocessing","1 = do not process incomplete windows at end of input",1);
  )

  SMILECOMPONENT_MAKEINFO(cVecToWinProcessor);
}

SMILECOMPONENT_CREATE(cVecToWinProcessor)

//-----

cVecToWinProcessor::cVecToWinProcessor(const char *_name) :
  cDataProcessor(_name),
  ptrWinA(NULL), ptrWinB(NULL),
  mat(NULL), ola(NULL),
  hasOverlap(0)
{
}

void cVecToWinProcessor::myFetchConfig()
{
  cDataProcessor::myFetchConfig();

  normaliseAdd = getInt("normaliseAdd");
  gain = (FLOAT_DMEM)getDouble("gain");


  useWinAasWinB = getInt("useWinAasWinB");

  processArrayFields = getInt("processArrayFields");
}


int cVecToWinProcessor::configureWriter(sDmLevelConfig &c)
{
  //SMILE_IDBG(2,"reader period = %f",c.T);

  if (isSet("forceSampleRate")) {
    double sr = getDouble("forceSampleRate");
    if (sr > 0.0) {
      c.basePeriod = 1.0/sr;
    } else {
      c.basePeriod = 1.0;
      SMILE_IERR(1,"sample rate (forceSampleRate) must be > 0! (it is: %f)",sr);
    }
  }

  c.frameSizeSec = c.basePeriod;
  c.N = Nfi; // TODO: is N fi available already??
  c.blocksizeWriter = (long)round(c.T/c.basePeriod);
  c.T = c.basePeriod;
  

  // this is now handled by cDataProcessor myConfigureInstance
  //writer->setConfig( 1, len, frameStep, 0.0, frameSize, 0, DMEM_FLOAT);

  // we must return 1, in order to indicate configure success (0 indicates failure)
  return 1;
}




double * cVecToWinProcessor::getWindowfunction(cWindower *_ptrWin, long n, int *didAlloc)
{
  double *w;
  if (_ptrWin != NULL) {
    sWindowerConfigParsed *c = _ptrWin->getWindowerConfigParsed();
    w = c->win;
    if (c->frameSizeFrames != n) {
      if (n > c->frameSizeFrames) {
        // TODO: alloc new frame, and zero pad 
        w = (double*)malloc(sizeof(double)*n);
        long i;
        for (i=0; i<c->frameSizeFrames; i++) w[i] = c->win[i];
        for (i=c->frameSizeFrames; i<n; i++) w[i] = 0.0;  // zero-pad window function
        if (didAlloc != NULL) *didAlloc = 1;
      } else {
        SMILE_IERR(1,"frameSizeFrames (%i) of cWindower component mismatches input frameSize in VecToWinProcessor (%i)!",c->frameSizeFrames,n);
      }
    }
    free(c);
  } else {
    w = (double*)malloc(sizeof(double)*n);
    if (didAlloc != NULL) *didAlloc = 1;
    long i;
    for (i=0; i<n; i++) w[i] = 1.0;
  }
  return w;
}

void cVecToWinProcessor::computeOlaNorm(long n, int idx)
{
  // get window functions
  cWindower * _ptrWinA = ptrWinA;
  cWindower * _ptrWinB = ptrWinB;

  long olaS = inputPeriodS; //(long) round( n * (1.0-ola[idx].overlap) );  // frame overlap in samples
  long I = (long)( floor((double)n / ((double)olaS))+4.0 );
  long i,j,a;
  int didAllocA=0, didAllocB=0;

  double *wA, *wB;
  wA = getWindowfunction(_ptrWinA,n,&didAllocA);
  if (useWinAasWinB) {
    wB = wA;
  } else {
    wB = getWindowfunction(_ptrWinB,n,&didAllocB);
    // problem: when zero-padding is used, the window function is shorter than n and should be zero outside its range
    // TODO: get the actual n from getWindowfunction
  }
  
  if (!normaliseAdd) {  // if we only want to scale with window B (=A, when useWinAasWinB is active)

    for (j=0; j<n; j++) {
      ola[idx].norm[j] = wB[j] * gain;
    }

  } else {

    double * tmp = (double*)calloc(1,sizeof(double)*(I*olaS+n));

    for (i=0; i<I; i++) {
      a=0;
      for (j=i*olaS; j<i*olaS+n; j++) {
        tmp[j] += wA[a]*wB[a];
        a++;
      }
    }

    a=0;
    for (j=(I/2+1)*olaS; j<(I/2+1)*olaS+n; j++) {
      if (tmp[j] > 0.0) {
        ola[idx].norm[a] = gain * 0.99 * ( 1.0/tmp[j] ); // if we should use winA as winB (winB NULL, and option set), then we must multiply winB into norm here...
        if (useWinAasWinB) ola[idx].norm[a] *= wB[a];
        //printf("xx: %f\n", ola[idx].norm[a]);
      } else {
        ola[idx].norm[a] = 1.0; // this should never happen... if it does audio will be broken/choppy, forcing the value of 1.0 will only reduce the damage and prevent applications from crashing
      }
      a++;
    }
    
    free(tmp);

  }

  // free wA and wB if they have been allocated. we only keep norm  (and wB in the future to save one extra windower component)
  if (didAllocA) free(wA);
  if (didAllocB) free(wB);
}



void cVecToWinProcessor::initOla(long n, double _samplePeriod, double _inputPeriod, int idx)
{
  ola[idx].framelen = n;
  // compute overlap 
  if (_inputPeriod <= 0 || _samplePeriod <= 0 || n <= 0) {
    ola[idx].overlap = 0.0;
  } else {
    ola[idx].overlap = 1.0 - _inputPeriod / ( (double)n * _samplePeriod );
  }


  if (ola[idx].overlap > 0.0) { 
    // init OLA buffer
    ola[idx].buffersize = 2*n;
    ola[idx].buffer = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*ola[idx].buffersize);
    

    // OLA normalisation curve
    if (normaliseAdd || useWinAasWinB) {
      ola[idx].norm = (double*)calloc(1,sizeof(double)*n);
      computeOlaNorm(n,idx);
    }
    hasOverlap = 1;
  } else {
    hasOverlap = 0;
  }
}


int cVecToWinProcessor::dataProcessorCustomFinalise() 
{
  //No = Nfi;
  // window functions
  const char * windowA = getStr("windowA");
  const char * windowB = getStr("windowB");
  
  if (windowA != NULL) {
    const char * tpA = getComponentInstanceType(windowA);
    cSmileComponent * winA = getComponentInstance(windowA);

    if (winA != NULL && tpA != NULL) {
      if (strcmp(tpA,"cWindower")) {  // !!strcmp ...
        SMILE_IERR(1,"The component '%s' (specified via the 'windowA' option) is not of type cWindower (it is of type '%s')! Please check your config!",windowA,tpA);
      } else {
        ptrWinA = (cWindower *)winA;
      }
    } else {
      SMILE_IERR(1,"The component '%s' was not found in the current config!",windowA);
    }
  }

  if (windowB != NULL) {
    const char * tpB = getComponentInstanceType(windowB);
    cSmileComponent * winB = getComponentInstance(windowB);

    if (winB != NULL && tpB != NULL) {
      if (strcmp(tpB,"cWindower")) {  // !!strcmp ...
        SMILE_IERR(1,"The component '%s' (specified via the 'windowB' option) is not of type cWindower (it is of type '%s')! Please check your config!",windowB,tpB);
      } else {
        ptrWinB = (cWindower *)winB;
      }
    } else {
      SMILE_IERR(1,"The component '%s' was not found in the current config!",windowB);
    }
  }

  // before we do any actions, we must check if the windower configs have been finalised and the window function is available (if normaliseAdd is activated), otherwise we must delay our finalise
  if (normaliseAdd && (ptrWinA != NULL || ptrWinB != NULL)) {
    if ( (ptrWinA == NULL || ptrWinA->isFinalised()) && (ptrWinB == NULL || ptrWinB->isFinalised()) ) {

   } else {
      // at least one windower is not yet ready...
      return 0;
    }
  } else {
    normaliseAdd = 0;
  } 

  Nfi = reader_->getLevelNf();
  if (!processArrayFields) { Nfi = 1; }

  ola = (struct sVecToWinProcessorOla*)calloc(1,sizeof(struct sVecToWinProcessorOla)*Nfi);

  const FrameMetaInfo * fm = reader_->getFrameMetaInfo();
  const sDmLevelConfig *cfg = reader_->getLevelConfig();
  samplePeriod = cfg->basePeriod; // ???
  inputPeriod = cfg->T;
  inputPeriodS = (long)round(inputPeriod/samplePeriod);

  // foreach input field: compute output and overlap parameters and create buffers
  long i;
  long e=0;

  if (!processArrayFields) {
    // the whole input is treated as a single vector/frame
    long n = reader_->getLevelN();
    No = setupNamesForField(0,nameAppend_,1);

    // TODO.. overlap, ola buffer, etc...
    initOla(n,samplePeriod,inputPeriod,0);

  } else {

    No = 0;
    for (i=0; i<Nfi; i++) {
      int __N=0;
      const char *tmp = reader_->getFieldName(i,&__N);
      if (tmp == NULL) { SMILE_IERR(1,"reader->getFieldName(%i) failed (return value = NULL)!",i); return 0; }


      long n = fm->field[i].N;
      
      initOla(n,samplePeriod,inputPeriod,i);

      // set output field name
      if (__N > 1) { // we only consider input vectors... not single elements!
        No += setupNamesForField(e++, tmp, 1);
      } else {
        SMILE_IWRN(2,"ignoring field '%s' with single element",tmp);
      }
    }

  }

  namesAreSet_ = 1;

  if (useWinAasWinB) normaliseAdd = 1;

  //Mult = getMultiplier();
  //if (Mult*Ni != No) COMP_ERR("Mult not constant (as returned by setupNamesForField! This is not allowed! Mult*Ni=%i <> No=%i",Mult*Ni,No);
  //???
  //if (tmpFrameF==NULL) tmpFrameF=(FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*Mult);
  return 1;
}

/*
int cVecToWinProcessor::myFinaliseInstance()
{

  // get reader names, append _win(_winf) to it (winf is optional)
  int ret=1;
  ret *= reader->finaliseInstance();
  if (!ret) return ret;

  Ni = reader->getLevelN();
  Nfi = reader->getLevelNf();
  No = 0;
//  Nfo = Ni;
  inputPeriod = reader->getLevelT();

  int i,n;

  long e=0;
  for (i=0; i<Nfi; i++) {
    int __N=0;
    const char *tmp = reader->getFieldName(i,&__N);
    if (tmp == NULL) { SMILE_IERR(1,"reader->getFieldName(%i) failed (return value = NULL)!",i); return 0; }
    if (__N > 1) {

      for (n=0; n<__N; n++) {
		    char *na = reader->getElementName(e);
        No += setupNamesForField(e++, na, 1);
        free(na);
      }

    } else {
      No += setupNamesForField(e++, tmp, 1);
    }
  }

  namesAreSet = 1;
  Mult = getMultiplier();
  if (Mult*Ni != No) COMP_ERR("Mult not constant (as returned by setupNamesForField! This is not allowed! Mult*Ni=%i <> No=%i",Mult*Ni,No);
  if (tmpFrameF==NULL) tmpFrameF=(FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM)*Mult);

  if (frameMode != FRAMEMODE_VAR) 
    reader->setupSequentialMatrixReading(frameStepFrames, frameSizeFrames, pre);

  return cDataProcessor::myFinaliseInstance();
  
}
*/

// idxi is index of input element
// row is the input row
// y is the output vector (part) for the input row
int cVecToWinProcessor::doProcess(int idxi, cMatrix *row, FLOAT_DMEM*y)
{
   /* int i;
    for (i=0;i<row->nT; i++) {
      printf("[%i] %f\n",i,row->data[i]);
    }*/
  SMILE_IERR(1,"dataType FLOAT_DMEM not yet supported!");
  //return getMultiplier();
  return 0;
  // return the number of actually set components in y!!
  // return 0 on failue
  // return -1 if you don't want to set a new output frame...
}

int cVecToWinProcessor::doFlush(int idxi, FLOAT_DMEM*y)
{
  //SMILE_IERR(1,"dataType FLOAT_DMEM not yet supported!");
  //return getMultiplier();
  return 0;
  // return the number of actually set components in y!!
  // return 0 on failue
  // return -1 if you don't want to set a new output frame...
}

/*
CONCEPT:

* inverse window function is handled seperately by the windower component on the vectors

* metadata read from vectors:
 - period of vectors, 
 - vector size
 - samplePeriod
* automatically compute overlap
SPECIAL CASE: if T=0 we have asynchronous data, in this case assume no overlap, then just concattenate data in the output level and don't do OLA

* configurable window functions a and b
* used for pre computing the normalisation function

- myTick reads single vectors and adds them in an internal buffer (auto sized)
- the fully overlapped parts of the buffer are flushed to the output level

--
naming of output elements:
- ideally input element without array name suffix (single element)
- but: manually overwriteable (i.e. processArrayFields = 0, and define 1 output name!)

*/

int cVecToWinProcessor::flushOlaBuffer(cMatrix *mat) 
{
  long i,j;
  for (i=0; i<Nfi; i++) {
    long ptr = ola[i].bufferReadPtr;
    long bs = ola[i].buffersize;
    FLOAT_DMEM * buf = ola[i].buffer;
    FLOAT_DMEM * df = mat->data+i;
    for (j=0; j<inputPeriodS; j++) {
      df[j*Nfi] = buf[ptr]; //(float)rand()/(float)RAND_MAX; 
      buf[ptr++] = 0.0;
      ptr %= bs;
    }
    ola[i].bufferReadPtr = ptr;
  }
  return 1;
}

/*
void cVecToWinProcessor::setOlaBufferRel(long idx, long j, FLOAT_DMEM val) 
{
  long x = (j+ola[idx].bufferPtr[idx])%ola[idx].buffersize;
  ola[idx].buffer[x] += val;
}
*/

void cVecToWinProcessor::setOlaBufferNext(long idx, FLOAT_DMEM val) 
{
  ola[idx].buffer[ola[idx].bufferPtr] += val;
  ola[idx].bufferPtr++;
  if (ola[idx].bufferPtr >= ola[idx].buffersize) {
    ola[idx].bufferPtr = 0;
  }
}

eTickResult cVecToWinProcessor::myTick(long long t)
{
  /*
   get a vector, for each field call a processFrame method to modify data
   then overlap add data to buffer and flush buffer pieces (of size n-olaS) to output level

   NOTE: the input period is always the same, this is the amount of data which will be written to the output in every tick. 
         only the frameSize and thus the percentage of overlap can change

  */

  SMILE_IDBG(4,"tick # %i, running vecToWinProcessor ....",t);
  
  // check for space in output level
  if (!(writer_->checkWrite(inputPeriodS))) return TICK_DEST_NO_SPACE;

  // get next frame from dataMemory
  cVector *vec;
  vec = reader_->getNextFrame();
  if (vec==NULL) { return TICK_SOURCE_NOT_AVAIL; } // currently no data available

  if (mat == NULL) mat = new cMatrix(Nfi, inputPeriodS);
  

  long i,j;
  if (hasOverlap) {

    for (i=0; i<Nfi; i++) {
      long el = vec->fmeta->fieldToElementIdx(i);
      long ptr1 = ola[i].bufferPtr;
      long ptr = ola[i].bufferPtr;
      FLOAT_DMEM * buf = ola[i].buffer;
      FLOAT_DMEM * df = vec->data+el;
      long bs = ola[i].buffersize;
      double * norm = ola[i].norm;

      if (normaliseAdd)  {
        // multiply input with norm function & add to buffer
        for (j=0; j<ola[i].framelen; j++) {
          //setOlaBufferNext(i, (FLOAT_DMEM)( vec->data[el+j] * ola[i].norm[j] ));
          buf[ptr1++] += df[j] * (FLOAT_DMEM)norm[j];
          if (ptr1 >= bs) ptr1 = 0;
        }
      } else {
        if (gain != 1.0) {
          // add to buffer, multiply with gain
          for (j=0; j<ola[i].framelen; j++) {
            //setOlaBufferNext(i, vec->data[el+j] * gain); 
            buf[ptr1++] += df[j] * gain;
            if (ptr1 >= bs) ptr1 = 0;
          }
        } else {
          // add to buffer
          for (j=0; j<ola[i].framelen; j++) {
            //setOlaBufferNext(i, vec->data[el+j]); 
            buf[ptr1++] += df[j];
            if (ptr1 >= bs) ptr1 = 0;
          }
        }
      }

      ola[i].bufferPtr = ptr + inputPeriodS;
      ola[i].bufferPtr %= ola[i].buffersize;
    }

    // flush buffer (always size of inputPeriod)
    if (flushOlaBuffer(mat)) {
      writer_->setNextMatrix(mat);
    } else {
      return TICK_INACTIVE;
    }

  } else {

    // flush directly to matrix object and write output
    for (i=0; i<Nfi; i++) {
      long el = vec->fmeta->fieldToElementIdx(i);
      if (gain != 1.0) {
        for (j=0; j<ola[i].framelen; j++) {
          mat->data[j*Nfi+i] = vec->data[el+j] * gain;
        }
      } else {
        for (j=0; j<ola[i].framelen; j++) {
          mat->data[j*Nfi+i] = vec->data[el+j];
        }
      }
    }
    writer_->setNextMatrix(mat);

  }

  return TICK_SUCCESS;
}


cVecToWinProcessor::~cVecToWinProcessor()
{
  // free overlap add buffers
  if (ola != NULL) {
    long i;
    for (i=0; i<Nfi; i++) {
      if (ola[i].norm != NULL) {
        free(ola[i].norm);
      }
      if (ola[i].buffer != NULL) {
        free(ola[i].buffer);
      }
    }
    free(ola);
  }

  // free output buffer matrix object
  if (mat != NULL) delete(mat);
}

