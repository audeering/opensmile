/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Signal source. Generates various noise types and pre-defined signals.

*/


#include <dsp/signalGenerator.hpp>
#define MODULE "cSignalGenerator"

SMILECOMPONENT_STATICS(cSignalGenerator)

SMILECOMPONENT_REGCOMP(cSignalGenerator)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CSIGNALGENERATOR;
  sdescription = COMPONENT_DESCRIPTION_CSIGNALGENERATOR;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("nFields","The number of fields in the output vector, set to -1 to use the size of the 'nElements' array or the 'fieldNames' array, if no nElements array is given.",1);
    ct->setField("nElements","An array of number of values/elements for each field in the output vector (default is 1)",1,ARRAY_TYPE);
    ct->setField("fieldNames","An array of names of fields in the output vector (default for all: noiseN, where N is the field number).",(const char*)NULL,ARRAY_TYPE);
    ct->setField("signalType","The type of signal/noise to generate for ALL(!) output fields. If you want different types of signals for the individual fields, use multiple noise sources and combine the output vectors in a data reader.\n Currently implemented noise and signal types are:\n   'white' \t white gaussian noise, use parameters 'stddev' and 'mean' to tune.\n   'const' \t constant value output, use 'constant' parameter to set this value.\n   'sine' \t single sine wave (range -1 to +1), see 'signalPeriod' or 'frequency' option and 'phase'.\n   'rect' \t rectangular periodic signal (range -1 to +1), see 'signalPeriod' or 'frequency' option and 'phase'.","white");
    ct->setField("randSeed","Random seed, for pseudo random gaussian white noise",1.0);
    //ct->setField("stddev","Standard deviation for gaussian white noise [CURRENTLY NOT IMPLEMENTED]",1.0);
    //ct->setField("mean","Mean value of the gaussian white noise [CURRENTLY NOT IMPLEMENTED]",0.0);
    ct->setField("scale","A scaling factor by which the generated signal is multiplied by",1.0);
    ct->setField("const","The constant value for the 'constant' signal type",0.0);
    ct->setField("signalPeriod","The period T in seconds for periodic signals (1.0/frequency). Don't forget to set the sample period via writer.levelconf.T or the 'period' option. Don't confuse this option with the 'period' option!",1.0,0,0);
    ct->setField("frequency","The frequency in Hz of periodic signals (1.0/signalPeriod). This overrides the 'signalPeriod' option, if both are set.",1.0);
    ct->setField("phase","The initial phase of periodic signals in seconds.",0.0);
    ct->setField("length","The length of the signal to generate (in seconds), -1.0 for infinite",3.0);
    ct->setField("lengthFrames","The length of signal to generate (in frames), -1 for infinite (overwrites 'length', if set)",-1,0,0);
  )

  SMILECOMPONENT_MAKEINFO(cSignalGenerator);
}

SMILECOMPONENT_CREATE(cSignalGenerator)

//-----

cSignalGenerator::cSignalGenerator(const char *_name) :
  cDataSource(_name),
  nValues(0), nFields(0), noiseType(0), lengthFrames(-1),
  fieldNames(NULL), nElements(NULL), samplePeriod(1.0), lastP(0.0), val(0.0), scale(1.0), myt(0), curT(0)
{
  // ...
}

void cSignalGenerator::myFetchConfig()
{
  cDataSource::myFetchConfig();
  
  nFields = getInt("nFields");
  SMILE_IDBG(2,"nFields = %i",nFields);

  int nEl, nfN, minF=-1;
  nEl = getArraySize("nElements");
  nfN = getArraySize("fieldNames");

  if (nFields > 0) { minF = nFields; }
  
  if (nEl > 0) {
    if (minF < 0) { minF = nEl; }
    if (nEl != minF) {
      minF = MIN(nEl,minF);
      SMILE_IWRN(2,"nFields (%i) specified does not match size of nElements array (%i) ! Using minimum: nFields = %i.",nFields,nEl,minF);
    } 
  }
  if (nfN > 0) {
    if (minF < 0) { minF = nfN; }
    if (nfN != nFields) {
      minF = MIN(nfN,minF);
      SMILE_IWRN(2,"nFields (%i) specified does not match size of fieldNames array (%i) ! Using minimum: nFields = %i.",nFields,nfN,minF);
    }
  }

  if (minF <= 0) {
    SMILE_IERR(1,"The automatically determined number of fields is invalid: %i . [ nFields = %i ; # nElements = %i ; # fieldNames = %i ]",minF,nFields,nEl,nfN);
  } else {
    // allocate arrays
    nElements = (int*)malloc(sizeof(int)*minF);
    fieldNames = (char**)calloc(1,sizeof(char*)*minF);

    int i;
    for (i=0; i<minF; i++) {
      if (nEl > 0) {
        nElements[i] = getInt_f(myvprint("nElements[%i]",i));
      } else {
        nElements[i] = 1;
      }
      if (nfN > 0) {
        fieldNames[i] = strdup(getStr_f(myvprint("fieldNames[%i]",i)));
      } else {
        fieldNames[i] = myvprint("noise%i",i);
      }
    }
  }

  nFields = minF;

  // noise type:
  const char * nt = getStr("signalType");
  if (nt != NULL) {
    if ( (!strncasecmp(nt,"white",5)) || (!strncasecmp(nt,"gauss",5)) || (!strncasecmp(nt,"static",5)) ) {
      noiseType = NOISE_WHITE;
    } else
    if ( (!strncasecmp(nt,"sin",3)) ) {
      noiseType = SIGNAL_SINE;
    } else
    if ( (!strncasecmp(nt,"rect",4)) ) {
      noiseType = SIGNAL_RECT;
    } else
    if ( (!strncasecmp(nt,"const",5)) ) {
      noiseType = SIGNAL_CONST;
    }
  }

  randSeed = getInt("randSeed");
  SMILE_IDBG(2,"randSeed = %i",randSeed);

  // stddev and mean options currently not implemented
  //stddev = (FLOAT_DMEM)getDouble("stddev");
  //SMILE_IDBG(2,"stddev = %f",stddev);

  //mean = (FLOAT_DMEM)getDouble("mean");
  //SMILE_IDBG(2,"mean = %f",mean);

  scale = getDouble("scale");
  SMILE_IDBG(2,"scale = %f",scale);

  constant = (FLOAT_DMEM)getDouble("const");
  SMILE_IDBG(2,"const = %f",constant);

  signalPeriod = getDouble("signalPeriod");
  SMILE_IDBG(2,"signalPeriod = %f",signalPeriod);
  
  if (isSet("frequency")) {
    double frequency = getDouble("frequency");
    SMILE_IDBG(2,"frequency = %f",frequency);
    if (frequency > 0.0) {
      signalPeriod = 1.0/frequency;
    } else {
      SMILE_IERR(1,"invalid value for the 'frequency' option (%f). Must be > 0. Setting signalPeriod = 0.",frequency);
      signalPeriod = 0.0;
    }
    SMILE_IDBG(2,"signalPeriod = %f",signalPeriod);
  }

  phase = getDouble("phase");
  SMILE_IDBG(2,"phase = %f",phase);
  myt = phase;


  if (isSet("lengthFrames")) {
    lengthFrames = getInt("lengthFrames");
  } else {
    double _length = getDouble("length");
    if (_length < 0.0) lengthFrames = -1;
    else {

      if (period_ > 0.0) {
        lengthFrames = (long)ceil( _length / period_ );
      } else {
        lengthFrames = (long)_length;
      }
    }
  }
}


int cSignalGenerator::configureWriter(sDmLevelConfig &c)
{
  // configure your writer by setting values in &c
  samplePeriod = c.T;

  return 1;
}

// NOTE: nEl is always 0 for dataSources....
int cSignalGenerator::setupNewNames(long nEl)
{
  int i;
  nValues=0;
//  samplePeriod = writer->getLevelT();

  for (i=0; i<nFields; i++) {
    writer_->addField(fieldNames[i],nElements[i]);
    nValues += nElements[i];
  }

  // initialise random generator:
  srand(randSeed); 

  allocVec(nValues);
  return 1;
}

/*
int cSignalGenerator::myFinaliseInstance()
{
  return cDataSource::myFinaliseInstance();
}
*/


/* WHITE NOISE SHIFT REGISTER:
float g_fScale = 2.0f / 0xffffffff;
int g_x1 = 0x67452301;
int g_x2 = 0xefcdab89;

void whitenoise(
  float* _fpDstBuffer, // Pointer to buffer
  unsigned int _uiBufferSize, // Size of buffer
  float _fLevel ) // Noiselevel (0.0 ... 1.0)
{
  _fLevel *= g_fScale;

  while( _uiBufferSize-- )
  {
    g_x1 ^= g_x2;
    *_fpDstBuffer++ = g_x2 * _fLevel;
    g_x2 += g_x1;
  }
}


GAUSSIAN:
#define PI 3.1415926536f

float R1 = (float) rand() / (float) RAND_MAX;
float R2 = (float) rand() / (float) RAND_MAX;

float X = (float) sqrt( -2.0f * log( R1 )) * cos( 2.0f * PI * R2 );



*/


#if 0 
GAUSSIAN:
/* Generate a new random seed from system time - do this once in your constructor */
srand(time(0));

/* Setup constants */
const static int q = 15;
const static float c1 = (1 << q) - 1;
const static float c2 = ((int)(c1 / 3)) + 1;
const static float c3 = 1.f / c1;

/* random number in range 0 - 1 not including 1 */
float random = 0.f;

/* the white noise */
float noise = 0.f;

for (int i = 0; i < numSamples; i++)
{
    random = ((float)rand() / (float)(RAND_MAX + 1));
    noise = (2.f * ((random * c2) + (random * c2) + (random * c2)) - 3.f * (c2 - 1.f)) * c3;
}
#endif

eTickResult cSignalGenerator::myTick(long long t)
{
  SMILE_IDBG(4,"tick # %i, writing value vector",t);

  if (!(nValues > 0 && (curT < lengthFrames || lengthFrames == -1))) 
    return TICK_INACTIVE;
  if (!writer_->checkWrite(1))
    return TICK_DEST_NO_SPACE;

  long i=0;
  const static int q = 15;
  const static float c1 = (1 << q) - 1;
  const static float c2 = ((int)(c1 / 3)) + 1; // ignore compiler warning here?? or do typecast?
  const static float c3 = 1.f / c1;
  
  FLOAT_DMEM v=0.0;
  switch(noiseType) {
    case NOISE_WHITE:
      float random;
      random = ((float)rand() / ((float)(RAND_MAX) + 1.0f));
      v = (FLOAT_DMEM)scale * (FLOAT_DMEM)( (2.f * ((random * c2) + (random * c2) + (random * c2)) - 3.f * (c2 - 1.f)) * c3 );
      for (i=0; i<nValues; i++) {
        vec_->data[i] = v;
      }
      break;
    case SIGNAL_CONST:
      for (i=0; i<nValues; i++) {
        vec_->data[i] = constant;
      }
      break;
    case SIGNAL_RECT:
      v = (FLOAT_DMEM)(val*scale);
      for (i=0; i<nValues; i++) {
        vec_->data[i] = v;
      }
      myt += samplePeriod;
      if ((signalPeriod > 0.0) && (floor(myt/(signalPeriod*0.5)) > lastP)) {
        if (val==1.0) val = -1.0;
        else val = 1.0;
      }
      lastP = floor(myt/(signalPeriod*0.5));
      break;
    case SIGNAL_SINE:
      val = sin(2.0*M_PI*(1.0/signalPeriod)*myt) ;
      v = (FLOAT_DMEM)( val * scale ); 
      for (i=0; i<nValues; i++) {
        vec_->data[i] = v;
      }
      myt += samplePeriod;
      break;

    // TODO: option for multiple additive sines (phase/frequency..)

    default:
      break;
  }

  writer_->setNextFrame(vec_);
  curT++;

  return TICK_SUCCESS;
}


cSignalGenerator::~cSignalGenerator()
{
  if (nFields > 0) {
    int i;
    if (fieldNames != NULL) {
      for (i=0; i<nFields; i++) {
        if (fieldNames[i] != NULL) free(fieldNames[i]);
      }
      free(fieldNames);
    }
  }
  if (nElements != NULL) {
    free(nElements);      
  }
}
