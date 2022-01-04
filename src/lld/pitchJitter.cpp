/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

// TODO: some pitch periods in opensmile.wav are twice, others are missing! FIX!

/*  openSMILE component:

compute jitter from waveform and F0 estimate

This component has 2 readers (!)

*/


#include <lld/pitchJitter.hpp>

#define MODULE "cPitchJitter"

SMILECOMPONENT_STATICS(cPitchJitter)

SMILECOMPONENT_REGCOMP(cPitchJitter)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CPITCHJITTER;
  sdescription = COMPONENT_DESCRIPTION_CPITCHJITTER;

  // we inherit cDataProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")
  if (ct->setField("F0reader", "Configuration of the dataMemory reader sub-component which is used to read the F0 estimate from a pitch component output (e.g. cPitchShs).",
                  _confman->getTypeObj("cDataReader"), NO_ARRAY, DONT_FREE) == -1) {
     rA=1; // if subtype not yet found, request , re-register in the next iteration
  }
/*
if (ct->setField("exWriter", "dataMemory writer for pcm voice excitation signal",
                  _confman->getTypeObj("cDataReader"), NO_ARRAY, DONT_FREE) == -1) {
     rA=1; // if subtype not yet found, request , re-register in the next iteration
  }
  */
  SMILECOMPONENT_IFNOTREGAGAIN(
    //ct->setField("F0dmLevel","data memory level to read f0 estimate from","pitch");
    ct->setField("F0field","The name of the field in 'F0reader.dmLevel' containing the F0 estimate (in Hz) (usually F0final or F0raw) - full name, exact match!","F0final");
    ct->setField("searchRangeRel","The relative search range for period deviations (Jitter): maxT0, minT0 = (1.0 +/- searchRangeRel)*T0",0.10);
    ct->setField("minNumPeriods", "Minimum number of F0 periods to compute jitter/shimmer over. The frame size must be large enough to hold that number of periods for the extraction to be stable!", 2);
    ct->setField("minCC", "Cross correlation threshold below which the periods will be rejected.", 0.5);
    ct->setField("jitterLocal","1 = enable computation of F0 jitter (period length variations). jitterLocal = the average absolute difference between consecutive periods, divided by the average period length of all periods in the frame",0);
    ct->setField("jitterDDP","1 = enable computation of F0 jitter (period length variations). jitterDDP = the average absolute difference between consecutive differences between consecutive periods, divided by the average period length of all periods in the frame",0);
    ct->setField("jitterLocalEnv","1 = compute envelope of jitterLocal (i.e. fill jitter values in unvoiced frames with value of last voiced segment). Use this in conjunction with statistical functionals such as means.",0);
    ct->setField("jitterDDPEnv","1 = compute envelope of jitterDDP (i.e. fill jitter values in unvoiced frames with value of last voiced segment). Use this in conjunction with statistical functionals such as means.",0);
    
    ct->setField("shimmerLocal","1 = enable computation of F0 shimmer (amplitude variations). shimmerLocal = the average absolute difference between the interpolated peak amplitudes of consecutive periods, divided by the average peak amplitude of all periods in the frame", 0);
    ct->setField("shimmerLocalDB","1 = enable computation of F0 shimmer (amplitude variations) in decibel (dB). shimmerLocal = the average absolute difference between the interpolated peak amplitudes of consecutive periods, divided by the average peak amplitude of all periods in the frame", 0);
    ct->setField("shimmerLocalEnv","1 = compute envelope of shimmerLocal (i.e. fill shimmer values in unvoiced frames with value of last voiced segment). Use this in conjunction with statistical functionals such as means.", 0);
    ct->setField("shimmerLocalDBEnv","1 = compute envelope of shimmerLocalDB (i.e. fill shimmer values in unvoiced frames with value of last voiced segment). Use this in conjunction with statistical functionals such as means.", 0);
    ct->setField("shimmerUseRmsAmplitude", "1 = use average rms amplitude instead of peak amplitude.", 0);

    ct->setField("harmonicERMS","1 = output of harmonic component RMS energy (energy average period waveform).",0);
    ct->setField("noiseERMS","1 = output of noise component RMS energy (energy of difference signal between repeated average period waveform and actual signal).",0);
    ct->setField("linearHNR","1 = output of harmonics to noise ratio computed from waveform signal (= harmonicERMS/noiseERMS)",0);
    ct->setField("logHNR","1 = output of logarithmic harmonics to noise ratio computed from waveform signal, using natural logarithm (base e) (logHNR = log(harmonicERMS/noiseERMS) )",0);
    ct->setField("lgHNRfloor","minimal value logHNR can be, to avoid very large negative numbers for small harmonic energies.",-100.0);
    // TODO: this option is not fully supported by the functionals component, for example.
    // winToVecProcessor cannot deal with variable frame rate, i.e. does not yet support collecting frames by looking at tmeta fields...
    ct->setField("onlyVoiced","1 = produce output only for voiced frames. I.e. do not output 0 jitter/shimmer values for unvoiced frames. WARNING: this option is not fully supported by the functionals component, yet.",0);
    ct->setField("refinedF0", "1 = output refined F0 in a field named after the 'F0field' option.", 0);
    ct->setField("sourceQualityMean", "1 = compute larynx source quality mean per frame (similarity of pitch periods)", 0);
    ct->setField("sourceQualityRange", "1 = compute larynx source quality range per frame (max - min similarity of pitch periods)", 0);
    ct->setField("usePeakToPeakPeriodLength", "1 = use peak to peak period length instead of correlation peak position (should roughly be the same - the old version used the correlation peak pos., which is the default)", 0);
    ct->setField("periodOutputFile", "Dump period start/end/amplitude/length to file if this option is set to a filename.", (const char*)NULL);
    //ct->setField("periodLengths","1 = enable output of individual period lengths",0);
    //ct->setField("periodStarts","1 = enable output of individual period start times",0);
    ct->makeMandatory(ct->setField("inputMaxDelaySec", "The maximum possible delay of the F0 input wrt. to the waveform in seconds. This occurs mainly for viterbi smoothing, for example. IT IS IMPORTANT that you set this parameter with care (summing up all delays like bufferLength of the viterbi smoother, etc.), otherwise the processing will hang or abort before the actual end of the input!", 2.0));
    ct->setField("useBrokenJitterThresh", "1 = enable compatibility with 2.2 and earlier versions with broken Jitter computation. Please specify this manually in all new configs (and update old configs to use value 0), as the default might change from 1 to 0 in future builds.", 1);
  )
  
  SMILECOMPONENT_MAKEINFO(cPitchJitter);
}


SMILECOMPONENT_CREATE(cPitchJitter)

//-----

cPitchJitter::cPitchJitter(const char *_name) :
  cDataProcessor(_name), minF0(25.0), // <- TODO: read this from pitch field metadata
    lastIdx(0), lastMis(0), out(NULL), F0reader(NULL), lastT0(0.0), lastDiff(0.0),
    Nout(0), lastJitterLocal(0.0), lastJitterDDP(0.0), lastShimmerLocal(0.0),
    lastJitterLocal_b(0.0), lastJitterDDP_b(0.0), lastShimmerLocal_b(0.0),
    filehandle(NULL), threshCC_(0.5), minNumPeriods(2),
    useBrokenJitterThresh_(1)
{
  char *tmp = myvprint("%s.F0reader",getInstName());
  F0reader = (cDataReader *)(cDataReader::create(tmp));
  if (F0reader == NULL) {
    COMP_ERR("Error creating dataReader '%s'",tmp);
  }
  if (tmp!=NULL) free(tmp);
}

void cPitchJitter::myFetchConfig()
{
  cDataProcessor::myFetchConfig();
  F0reader->fetchConfig();

  // load all configuration parameters you will later require fast and easy access to here:

  F0field = getStr("F0field");
  SMILE_IDBG(2,"F0field = '%s'",F0field);
  
  searchRangeRel = getDouble("searchRangeRel");
  SMILE_IDBG(2,"searchRangeRel = %f",searchRangeRel);

  onlyVoiced = getInt("onlyVoiced");
  SMILE_IDBG(2,"onlyVoiced = %i",onlyVoiced);

  jitterLocal = getInt("jitterLocal");
  SMILE_IDBG(2,"jitterLocal = %i",jitterLocal);
  jitterDDP = getInt("jitterDDP");
  SMILE_IDBG(2,"jitterDDP = %i",jitterDDP);

  jitterLocalEnv = getInt("jitterLocalEnv");
  SMILE_IDBG(2,"jitterLocalEnv = %i",jitterLocalEnv);
  jitterDDPEnv = getInt("jitterDDPEnv");
  SMILE_IDBG(2,"jitterDDPEnv = %i",jitterDDPEnv);

  shimmerLocalDB = getInt("shimmerLocalDB");
  shimmerLocal = getInt("shimmerLocal");
  SMILE_IDBG(2,"shimmerLocal = %i",shimmerLocal);
  shimmerLocalDBEnv = getInt("shimmerLocalDBEnv");
  shimmerLocalEnv = getInt("shimmerLocalEnv");
  SMILE_IDBG(2,"shimmerLocalEnv = %i",shimmerLocalEnv);

  harmonicERMS = getInt("harmonicERMS");
  noiseERMS = getInt("noiseERMS");
  linearHNR = getInt("linearHNR");
  logHNR = getInt("logHNR");
  lgHNRfloor = (FLOAT_DMEM)getDouble("lgHNRfloor");
  input_max_delay_ = getInt("inputMaxDelaySec");
  shimmerUseRmsAmplitude = getInt("shimmerUseRmsAmplitude");

  minNumPeriods = getInt("minNumPeriods");
  if (minNumPeriods < 2) {
    SMILE_IWRN(2, "minNumPeriods must be >= 2. Setting to 2.");
    minNumPeriods = 2;
  }
  threshCC_ = (FLOAT_DMEM)getDouble("minCC");
  if (threshCC_ < (FLOAT_DMEM)0.01) {
    SMILE_IWRN(2, "minCC must be > 0.01 and < 0.99! Setting to 0.01.");
    threshCC_ = (FLOAT_DMEM)0.01;
  }
  if (threshCC_ > (FLOAT_DMEM)0.99) {
    SMILE_IWRN(2, "minCC must be > 0.01 and < 0.99! Setting to 0.99.");
    threshCC_ = (FLOAT_DMEM)0.99;
  }
  refinedF0 = getInt("refinedF0");
  sourceQualityRange = getInt("sourceQualityRange");
  sourceQualityMean = getInt("sourceQualityMean");
  usePeakToPeakPeriodLength_ = getInt("usePeakToPeakPeriodLength");
  useBrokenJitterThresh_ = getInt("useBrokenJitterThresh");
}




int cPitchJitter::configureReader(const sDmLevelConfig &c)
{
  // check if F0 level period and pcm input period match
  double Tf0 = F0reader->getLevelT();
  //double T = reader_->getLevelT();
  if (c.T == Tf0) {
    SMILE_IERR(1,"pcm level frame period must be << F0 level frame period! pcm data should be stream data and not frame data!");
  }

  // set blocksize of wave reader to 5*max pitch period (2.0/minF0)
  blocksizeR_sec_ = (long)ceil((3.0 + (double)minNumPeriods)/(minF0)) + input_max_delay_;
  if (c.T > 0.0) {
    blocksizeR_ = (long) ceil (blocksizeR_sec_ / c.T);
  } else {
    SMILE_IERR(1, "Cannot properly set the reader blocksize in frames from blocksize in seconds, as the input level is not periodic (lcfg.T==0!). Non-periodic waveform input levels are not supported for pitchJitter!");
    return 0;
  }
  // FIXME: when the F0 is delayed due to post-smoothing (viterbi, etc.)
  //        then data in the wave level might have expired, or reading
  //        will block the processing.
  //        Thus, we must signal the wave reader the max possible delay
  //        so that the wave buffersize can be set correctly.
  //        We don't know the delay, as there is no standard mechanism
  //        we can use to read it from the pitch components.
  //        Therefore we provide a mandatory config option and let the user
  //        set it correctly!
  int ret = cDataProcessor::configureReader(c);
  F0reader->setBlocksize(1);
  return ret;
}

void cPitchJitter::mySetEnvironment()
{
  cDataProcessor::mySetEnvironment();
  F0reader->setComponentEnvironment(getCompMan(), -1, this);
}

int cPitchJitter::myRegisterInstance(int *runMe)
{
  int ret = cDataProcessor::myRegisterInstance(runMe);
  ret *= F0reader->registerInstance();
  return ret;
}

int cPitchJitter::myConfigureInstance()
{
  if (!(F0reader->configureInstance())) return 0;
  if (!(F0reader->finaliseInstance())) return 0;

  int ret = cDataProcessor::myConfigureInstance();
  return ret;
}

int cPitchJitter::configureWriter(sDmLevelConfig &c) 
{
  // check that we have mono input!!
  if (c.fmeta->Ne > 1) {
    SMILE_IERR(1,"this component must read mono (1 channel) wave input (your input currently has %i channels)! Use the monomixdown option in the wave-source!",c.fmeta->Ne);
    return 0;
  }

  const sDmLevelConfig *cfg = F0reader->getLevelConfig();
  c.T = cfg->T;
  c.frameSizeSec = cfg->frameSizeSec;
  c.basePeriod = cfg->basePeriod;
  c.growDyn = cfg->growDyn;
  c.isRb = cfg->isRb;
  c.lastFrameSizeSec = cfg->frameSizeSec;
  c.lenSec = cfg->lenSec;
  return 1; /* success */
}

/* You shouldn't need to touch this....
int cPitchJitter::myConfigureInstance()
{
  int ret = cDataProcessor::myConfigureInstance();
  return ret;
}
*/

/*
  Do what you like here... this is called after the input names and number of input elements have become available, 
  so you may use them here.
*/
/*
int cPitchJitter::dataProcessorCustomFinalise()
{
  
  return 1;
}
*/


/* 
  Use setupNewNames() to freely set the data elements and their names in the output level
  The input names are available at this point, you can get them via reader->getFrameMeta()
  Please set "namesAreSet" to 1, when you do set names
*/

int cPitchJitter::setupNewNames(long nEl) 
{
  // find pitch input field:
  const FrameMetaInfo * fmeta = F0reader->getFrameMetaInfo();
  int ri=0;
  long idx = fmeta->findField( F0field , &ri );
  if (nEl <= 0) nEl = reader_->getLevelN();
  if (idx < 0) {
    F0fieldIdx = 0;
    SMILE_IWRN(2,"Requested input field '*%s*' not found, defaulting to use 0th field! Available field names are listed below:", F0field);
    fmeta->printFieldNames();
  } else {
    F0fieldIdx = fmeta->fieldToElementIdx( idx ) + ri;
  }
  
  int n=0;
  if (jitterLocal) { writer_->addField("jitterLocal",1); n++; }
  if (jitterDDP) { writer_->addField("jitterDDP",1); n++; }
  if (jitterLocalEnv) { writer_->addField("jitterLocEnv",1); n++; }
  if (jitterDDPEnv) { writer_->addField("jitterDEnv",1); n++; }
  if (shimmerLocal) { writer_->addField("shimmerLocal",1); n++; }
  if (shimmerLocalDB) { writer_->addField("shimmerLocalDB",1); n++; }
  if (shimmerLocalEnv) { writer_->addField("shimmerLocEnv",1); n++; }
  if (shimmerLocalDBEnv) { writer_->addField("shimmerLocDBEnv",1); n++; }
  if (harmonicERMS) { writer_->addField("harmonicERMS",1); n++; }
  if (noiseERMS) { writer_->addField("noiseERMS",1); n++; }
  if (linearHNR) { writer_->addField("linearHNR",1); n++; }
  if (logHNR) { writer_->addField("logHNR",1); n++; }
  if (refinedF0) {
    if (F0field != NULL) {
      writer_->addField(F0field, 1);
    } else {
      writer_->addField("F0final", 1);
    }
    n++;
  }
  if (sourceQualityMean) {
    writer_->addField("sourceQualityMean", 1);
    n++;
  }
  if (sourceQualityRange) {
    writer_->addField("sourceQualityRange", 1);
    n++;
  }
  namesAreSet_ = 1;
  Nout = n;
  return n;
}

/*
  If you don't use setupNewNames() you may set the names for each input field by overwriting the following method:
*/
/*
int cPitchJitter::setupNamesForField( TODO )
{
  // DOC TODO...
}
*/

int cPitchJitter::myFinaliseInstance()
{
  int ret = cDataProcessor::myFinaliseInstance();
  if (ret) {
    const char *periodOutputFile = getStr("periodOutputFile");
    if (periodOutputFile != NULL) {
      filehandle = fopen(periodOutputFile, "w");
    }
  }
  savedMaxDebugPeriod = 0;
  return ret;
}

double cPitchJitter::crossCorr(FLOAT_DMEM * x, long Nx, FLOAT_DMEM * y, long Ny)
{
  long N = MIN(Nx,Ny);
  long i;
  double cc = 0.0;
  double mx = 0.0;
  double my = 0.0;
  double nx = 0;
  double ny = 0;
  for (i=0; i<N; i++) {
    mx += x[i];
    my += y[i];
  }
  mx /= (double)N;
  my /= (double)N;
  for (i=0; i<N; i++) {
/*
// TODO: low-pass filter in here...
// x[i] is symm moving avg of +- 5 samples?
double x_avg = 0.0;
double x_n = 0;
double y_avg = 0.0;
for (int j = 0; j < 6; j++) {
  if (i - j > 0) {
    x_avg += x[i - j];
    y_avg += y[i - j];
    x_n += 1.0;
  }
  if (i + j < N && j != 0) {
    x_avg += x[i - j];
    y_avg += y[i - j];
    x_n += 1.0;
  }
}
if (x_n > 0.0) {
  x_avg /= x_n;
  y_avg /= x_n;
}
    cc += (x_avg-mx)*(y_avg-my);
    nx += (x_avg-mx)*(x_avg-mx);
    ny += (y_avg-my)*(y_avg-my);*/
    cc += (x[i]-mx)*(y[i]-my);
    nx += (x[i]-mx)*(x[i]-mx);
    ny += (y[i]-my)*(y[i]-my);
  }
  /*
  FLOAT_DMEM tmpX[4];
  FLOAT_DMEM tmpY[4];
  FLOAT_DMEM tmp2[4];
  int j;
  for (i=0; i<N; i+=4) {
    tmpX[0] = x[i+0]-mx;
    tmpX[1] = x[i+1]-mx;
    tmpX[2] = x[i+2]-mx;
    tmpX[3] = x[i+3]-mx;
    tmpY[0] = y[i+0]-my;
    tmpY[1] = y[i+1]-my;
    tmpY[2] = y[i+2]-my;
    tmpY[3] = y[i+3]-my;
    tmp2[0] = tmpX[0]*tmpY[0];
    tmp2[1] = tmpX[1]*tmpY[1];
    tmp2[2] = tmpX[2]*tmpY[2];
    tmp2[3] = tmpX[3]*tmpY[3];
    cc += tmp2[0] + tmp2[1] + tmp2[2] + tmp2[3];
    tmp2[0] = (x[i+0]-mx) * tmpX[0];
    tmp2[1] = (x[i+1]-mx) * tmpX[1];
    tmp2[2] = (x[i+2]-mx) * tmpX[2];
    tmp2[3] = (x[i+3]-mx) * tmpX[3];
    nx += tmp2[0] + tmp2[1] + tmp2[2] + tmp2[3];
    tmp2[0] = (y[i+0]-mx) * tmpY[0];
    tmp2[1] = (y[i+1]-mx) * tmpY[1];
    tmp2[2] = (y[i+2]-mx) * tmpY[2];
    tmp2[3] = (y[i+3]-mx) * tmpY[3];
    ny += tmp2[0] + tmp2[1] + tmp2[2] + tmp2[3];
  }*/
  cc /= sqrt(nx)*sqrt(ny);
  return cc;
}

// get difference of peak amplitude in the two given frames
// save interpolated index of max peak in first frame in *maxI0 if != NULL
// save interpolated index of max peak in second frame in *maxI1 if != NULL
FLOAT_DMEM cPitchJitter::amplitudeDiff(FLOAT_DMEM *x, long Nx, FLOAT_DMEM *y, long Ny, double *maxI0, double *maxI1, FLOAT_DMEM *_A0, FLOAT_DMEM *_A1)
{
  long i;
  long N = MIN(Nx,Ny);
  double A0=1.0, A1=1.0;
  // analyse first frame:
  long mI=1;
  FLOAT_DMEM max0=x[1];
  FLOAT_DMEM min0=x[1];
  for (i=1; i<Nx-1; i++) {
    if (x[i] > max0) { max0=x[i]; mI = i; }
    if (x[i] < min0) { min0=x[i]; }
  }
  double mi = smileMath_quadFrom3pts((double)(mI-1),x[mI-1],(double)(mI),x[mI],(double)(mI+1),x[mI+1],&A0,NULL);
  //printf("A0 %f  x[%i] %f  Nx=%i\n",A0,mI,x[mI],Nx);
  if (maxI0 != NULL) *maxI0 = mi;
  // analyse second frame:
  mI = 1;
  FLOAT_DMEM max1 = y[1];
  FLOAT_DMEM min1 = y[1];
  for (i=1; i<Ny-1; i++) {
    if (y[i] > max1) { max1=y[i]; mI = i; }
    if (y[i] < min1) { min1=y[i]; }
  }
  mi = smileMath_quadFrom3pts((double)(mI-1),y[mI-1],(double)(mI),y[mI],(double)(mI+1),y[mI+1],&A1,NULL);
  if (maxI1 != NULL) *maxI1 = mi;
  //printf("A1 %f  x[%i] %f  Ny=%i\n",A1,mI,y[mI],Ny);
  
  // save min to max amplitudes
  //if (_A0 != NULL) *_A0 = (FLOAT_DMEM)A0;
  //if (_A1 != NULL) *_A1 = (FLOAT_DMEM)A1;
  if (_A0 != NULL) *_A0 = max0 - min0;
  if (_A1 != NULL) *_A1 = max1 - min1;

  // compute relative min to max amplitude difference:
  return (FLOAT_DMEM)fabs((max0 - min0) - (max1 - min1));
}

// get difference of low-pass smoothed rms amplitudes in the two given frames
// save interpolated index of max peak in first frame in *maxI0 if != NULL  (rms amplitudes in A0, A1..! NOT peak)
// save interpolated index of max peak in second frame in *maxI1 if != NULL
FLOAT_DMEM cPitchJitter::rmsAmplitudeDiff(FLOAT_DMEM *x, long Nx, FLOAT_DMEM *y, long Ny, double *maxI0, double *maxI1, FLOAT_DMEM *_A0, FLOAT_DMEM *_A1)
{
  long i;
  long N = MIN(Nx,Ny);
  double A0 = 1.0;
  double A1 = 1.0;
  // analyse first frame:
  long mI = 1;
  FLOAT_DMEM max = x[1];
  FLOAT_DMEM rmsX = x[0] * x[0];
  for (i=1; i < Nx - 1; i++) {
    if (x[i] > max) {
      max=x[i];
      mI = i;
    }
    rmsX += x[i] * x[i];
  }
  rmsX = sqrt((rmsX + x[i] * x[i]) / (FLOAT_DMEM)Nx);
  double mi = smileMath_quadFrom3pts((double)(mI-1),x[mI-1],(double)(mI),x[mI],(double)(mI+1),x[mI+1],&A0,NULL);
  if (maxI0 != NULL) {
    *maxI0 = mi;
  }
  // analyse second frame:
  mI = 1;
  max = y[1];
  FLOAT_DMEM rmsY = y[0] * y[0];
  for (i = 1; i < Ny - 1; i++) {
    if (y[i] > max) {
      max=y[i];
      mI = i;
    }
    rmsY += y[i] * y[i];
  }
  rmsY = sqrt((rmsY + y[i] * y[i]) / (FLOAT_DMEM)Ny);
  //rmsY = (rmsY + y[i]) / (FLOAT_DMEM)Ny;
  mi = smileMath_quadFrom3pts((double)(mI-1),y[mI-1],(double)(mI),y[mI],(double)(mI+1),y[mI+1],&A1,NULL);
  if (maxI1 != NULL) {
    *maxI1 = mi;
  }
  // save rms amplitudes
  if (_A0 != NULL) {
    *_A0 = (FLOAT_DMEM)rmsX;
  }
  if (_A1 != NULL) {
    *_A1 = (FLOAT_DMEM)rmsY;
  }
  // compute relative rms amplitude difference:
  return (FLOAT_DMEM)fabs(rmsX - rmsY);
}

void cPitchJitter::saveDebugPeriod(long sample, double sampleInterp)
{
  if (filehandle != NULL) {
    if (sample > savedMaxDebugPeriod + 1) {
      savedMaxDebugPeriod = sample;
      fprintf(filehandle, "%ld,%f\n", sample, sampleInterp);
    }
  }
}


#if 0
eTickResult cPitchJitter::myTick2(long long t)
{
  if (isEOI()) return TICK_INACTIVE; // TODO: check if we need to flush jitter data OR we can still read from input....?

  // get next F0 frame:
  cVector *fvec = F0reader->getNextFrame();
  if (fvec == NULL) {
    return TICK_SOURCE_NOT_AVAIL;
  }
  FLOAT_DMEM F0 = 0.0;
  if (F0fieldIdx < fvec->N) {
    F0 = fvec->data[F0fieldIdx];
  }

  // get wave data belonging to this F0 frame..
  double T = reader_->getLevelT();  // wave sample period
  long startVidxS = (long)round(fvec->tmeta->time / T);  // start of F0 frame in samples
  long lenF0frameS = fvec->tmeta->lengthFrames;  // the length of the F0 frame in wavesamples
  double F0period = fvec->tmeta->period;  // pitch frame period
  long stepF0frameS = (long)ceil(F0period/T);      // step size in samples of F0 frame
  long ovlF0frameS = lenF0frameS - stepF0frameS;  // pitch frame overlap (in samples)

  long toRead = stepF0frameS;
  if (firstF0frame) {
    toRead = lenF0frameS;
    firstF0frame = false;
  }
  cMatrix *mat = reader_->getMatrix(startVidxS, toRead /* = pitch frame length/step */);
  // save in ringbuffer
  waveBuffer->setNext(mat->data, mat->nT);
  if (F0 > 0.0) {
    // figure out which data to process from wave buffer wrPtr_ backwards
    long wp = waveBuffer->getWritePointer();
    wp -= lenF0frameS;
    // at least 2 periods...

    // not smaller than 0...

    FLOAT_DMEM *buf = waveBuffer->get(wp, lenF0frameS);

    // do wave period matching

  // get data from ringbuffer...:
  /* from lastIndex to end of buffer (only full periods)
   * at least 2 estimated periods,
   * cc match to refine period lengths
   * save last buffer index
   */
  } else {
    // reset some pointers

  }
}
#endif

/*
 * Ideas for NEW jitter component:
 *
 * read wave data sync. to pitch without delay
 * store in internal ringbuffer of sufficient size
 * - cross correlation computation
 * - analyse amplitudes and correlations in double and half the pitch period
 * - fix octave errors?
 * - compute jitter / shimmer / hnr
 * - compute other features from pitch region... make available to dataMemory?
 */
eTickResult cPitchJitter::myTick(long long t)
{
  if (isEOI()) return TICK_INACTIVE; // TODO: check if we need to flush jitter data OR we can still read from input....?

  if (!writer_->checkWrite(1))
    return TICK_DEST_NO_SPACE;

  long i;

  // get next pitch frame:
  cVector *fvec = F0reader->getNextFrame();
  if (fvec == NULL) 
    return TICK_SOURCE_NOT_AVAIL;

  FLOAT_DMEM F0 = 0.0;
  if (F0fieldIdx < fvec->N) {
    F0 = fvec->data[F0fieldIdx];  // F0 estimate
  }
  long lenF = (long)ceil(fvec->tmeta->lengthSec / fvec->tmeta->framePeriod);

  double T = reader_->getLevelT(); // sample period
  long startVidx = (long)round(fvec->tmeta->time / T);  // start in samples
  double pitchT = fvec->tmeta->period; // pitch frame period


  long ppLen = (long)ceil(pitchT/T);  // step size in samples of pitch frames
  long ovl = lenF - ppLen;  // pitch frame overlap (in samples)


  // New method to determine toRead samples:
  //  We need to read at least ppLen+lastMix, otherwise we fall behind the frame rate
  //  We need at least two pitch periods + serach range tolerance
  //  We must read at max. lenF+lastMis
  long toRead0 = ppLen+lastMis;  // number of samples we should read?
  long toRead = toRead0;
  double T0 = 0.0; // f0 period (seconds)
  double Tf = 0.0; // f0 period in samples, floating point
  long T0f = 0; // f0 period in samples as integer
  // upper / lower T0 bounds:
  double T0min = 0.0;  // lower bound of f0 period search range
  double T0max = 0.0;  // upper bound of f0 period search range
  long T0minF = 0;  // bounds as integer (samples)
  long T0maxF = 0;   // bounds as integer (samples)
  long two_pp = 0;
  if (F0 > 0.0) {
    T0 = 1.0/F0;
    Tf = T0/T;
    T0f = (long)round(Tf);
    T0min = (1.0-searchRangeRel)*Tf;
    T0max = (1.0+searchRangeRel)*Tf;
    T0minF = (long)floor(T0min);
    T0maxF = (long)ceil(T0max);
    two_pp = minNumPeriods * T0maxF + minNumPeriods;
    if (toRead < two_pp) {
      toRead = two_pp;
    }
  }
  long maxRead = lastMis+lenF;  // maxmimum number of samples we may read...
  //SMILE_IMSG(2,"Input indices (F0 = %f) (startV-lm = %i) != (last idx = %i)  (startv: %i, lastm: %i, tord %i, maxrd %i ; ppLen %i, 2pp %i).", F0, startVidx - lastMis, lastIdx, startVidx, lastMis, toRead, maxRead, ppLen, two_pp);

  if (toRead > maxRead) {
    SMILE_IWRN(2, "It appears that we need to read more wave samples than the length of the input F0 frame to read two full pitch periods at F0=%f with tolerance (searchRangeRel = %f) : toRead = %i, maxRead = %i. Limiting toRead=maxRead. Reduce the value of the minNumPeriods (=%i) option or increase the F0 framesize!\n", F0, searchRangeRel, toRead, maxRead, minNumPeriods);
    toRead = maxRead;
  }

  // check start idx:
  if (startVidx - lastMis != lastIdx) {
    SMILE_IWRN(3,"Discontinuity in input indices... %i != %i  (v: %i, lastm: %i, tr %i, mr %i - ppLen %i). This warning is ok, if you encounter it at the beginning and a pitchSmoother component is causing a delay of the pitch frames.", startVidx - lastMis, lastIdx, startVidx, lastMis, toRead, maxRead, ppLen);
    lastIdx = startVidx;
    if (toRead > lenF) { toRead = lenF; } 
    if (maxRead > lenF) { maxRead = lenF; } 
  }
  
  // read wave data for the current pitch frame
  cMatrix *mat = reader_->getMatrix(lastIdx, toRead /* = pitch frame length */);

  if (mat == NULL && !isEOI()) {
    // TODO: print this error message only if lastIdx is well behind the current F0 frame vIdx equivalent!
    SMILE_IERR(2, "no pcm data read!  lastIdx %ld  toRead %ld. Check inpuMaxDelaySec option! It needs to be increased (unless you get this error message close to the end of processing, current tick = %lld).", lastIdx, toRead, t);
    lastIdx += toRead0;
    return TICK_SOURCE_NOT_AVAIL;
  }
  if (maxRead < 1 || mat->data==NULL) {
    SMILE_IERR(1,"maxRead < 1 or mat->data==NULL, something is wrong (probably tmeta info on the pitch input level is not set correctly, thus the pitch frame length is read as 0; please debug the component that produces the pitch data this component reads!)");
    return TICK_INACTIVE;
  }

  // jitter computation variables:
  FLOAT_DMEM nPeriodsLocal = 0;
  FLOAT_DMEM nPeriodsDDP = 0;
  FLOAT_DMEM nPeriods = 0;
  FLOAT_DMEM avgPeriod = 0.0;
  FLOAT_DMEM JitterDDP=0.0;  // Praat: ddp
  FLOAT_DMEM JitterLocal=0.0;
  FLOAT_DMEM JitterPPQ=0.0;
  // shimmer computation variables:
  FLOAT_DMEM avgAmp=0.0;
  FLOAT_DMEM avgAmpDiff=0.0;
  // HNR computation variables:
  FLOAT_DMEM eH = 0.0;
  FLOAT_DMEM eN = 0.0;
  FLOAT_DMEM HNR = 0.0;
  FLOAT_DMEM lgHNR = 0.0;
  FLOAT_DMEM sumCC = 0.0;
  FLOAT_DMEM maxCC = -2.0;
  FLOAT_DMEM minCC = -2.0;
  int nCC = 0;
  
  // start the waveform matching for voiced segments
  long start = 0;
  long lastPeriod = 0;
  if (F0 > 0.0) { // voiced frame

    // create a buffer for storing pitch period boundaries (required for second pass HNR computation)
    int numPeriods = 0;
    long * periodBuffer;
    if (T0f > 0) periodBuffer = (long *)calloc(1,sizeof(long)*(maxRead/T0minF+3));
    else periodBuffer = (long *)calloc(1,sizeof(long)*(maxRead+2));

    // buffer for computing average period waveform
    FLOAT_DMEM * avgPeriodWf = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM) * (T0f+1));

    //// waveform matching  (TODO: sync to amplitude max.)
    double *cc = (double*)calloc(1,sizeof(double)*((int)(T0maxF-T0minF)+1));

    long os = start;
    long pp = 0;
    // TODO: check what happens, if the while loop below does not run at all!
    //       if pp is uninitialised, then valgrind will complain in the line:
    // for (j=periodBuffer[i]; j<MIN(periodBuffer[i+1],periodBuffer[i]+T0f); j++) {
    //printf("read N %i  2*T0maxF %i\n",mat->nT,2*T0maxF);
    // TODO: multi-channel (mat->N > 1) support!
    //printf("  start %i  toRead %i  T0maxF %i  mat-nt %i\n",start, toRead, 2*T0maxF, mat->nT);
    //while (( start< mat->nT - 2*T0maxF ) && (start < toRead)) {
    //while (( start < toRead0 - T0maxF - 1 ) && (start < toRead - 2 * T0maxF - 1)) {
    while ((start < mat->nT - 2 * T0maxF - 1)) {
      long _Tf;
      for (_Tf = T0minF; _Tf <= T0maxF; _Tf++) {
        long mid = start+_Tf;
        cc[_Tf-T0minF] = crossCorr(mat->data+start,_Tf,mat->data+mid,_Tf);
// FIXME: cross correlation with previous and next period, then average to get better estimate for current period length?
// FIXME: re-evaluate period length and position after band-pass comb filtering (phase shift??) and peak detection for boundaries!
// FIXME: sync correlation to max amplitude boundaries once found  (via lastMis variable..)
      }

      // peak picking
      double max=cc[T0f-T0minF];
      long maxI=-1;
      //printf("T0maxF %i T0minF %i T0 %f, F0 %f, T0min %f , T0max %f T %f\n",T0maxF,T0minF,T0,F0,T0min,T0max,T);
      for (i=1; i<T0maxF-T0minF-1; i++) {
        //printf("cc[%i] = %f\n",i,cc[i]);
        if ( (cc[i-1] < cc[i]) && (cc[i] > cc[i+1]) ) { // peak
          if (maxI == -1) {
            maxI = i; max = cc[i];
          } else {
            if (cc[i] > max) {
              maxI = i; max = cc[i];
            }
          }
        }
      }
      if (maxI == -1) {
        // no peak found, we assign the expected period length, for HNR computation
        pp = T0f;
      } else {
        pp = T0minF + maxI;
      }

      // assign new start
      os = start;

      // compute Jitter and Shimmer
      //printf("  jitter: %i  pp=%i (maxI: %i)\n",pp-T0f,pp,maxI);
      if (maxI >= 0) {
        start += pp;
        // get amplitude differences  for shimmer computation:
        double max0 = 0.0;
        double max1 = 0.0;
        FLOAT_DMEM _a0=0.0;
        FLOAT_DMEM _a1=0.0;
        FLOAT_DMEM ad=0.0;
// FIXME: compute shimmer with a lag of one period... only if we know the end position of the next period, compute shimmer! 
// FIXME: smooth the period waveforms with a narrow band-pass (comb!?) before computing amplitude to improve noise robustness!
//        --> better:  generate a sine (not dirac pulse... because f0 alone is sinusoidal) and correlate that with a single period to obtain period phase and magnitude...?
        if (shimmerUseRmsAmplitude) {
          ad = rmsAmplitudeDiff(mat->data+os, pp, mat->data+start, pp, &max0, &max1, &_a0, &_a1);
        } else {
          ad = amplitudeDiff(mat->data+os, pp, mat->data+start, pp, &max0, &max1, &_a0, &_a1);
        }
        periodBuffer[numPeriods++] = os;
        // compute averaged pitch period waveform for HNR
        for (i=0; i<T0f; i++) {
          avgPeriodWf[i] += *(mat->data+(os+i));
        }

        // parabolic interpolation of maxI
        double conf = 0.0;
        double ccI = 0.0;
        double maxId = fabs(((double)T0minF + smileMath_quadFrom3pts((double)(maxI-1),cc[maxI-1],(double)(maxI),cc[maxI],(double)(maxI+1),cc[maxI+1],&ccI,&conf) ) ) * T;
        // TODO: save instantaneous periods to extra file... with proper timestamps (e.g. for sonic visualiser).

        // store CC stats for laryngalisation
        sumCC += (FLOAT_DMEM)ccI;
        if (minCC == (FLOAT_DMEM)-2.0 || minCC > (FLOAT_DMEM)ccI) {
          minCC = (FLOAT_DMEM)ccI;
        }
        if (maxCC == (FLOAT_DMEM)-2.0 || maxCC < (FLOAT_DMEM)ccI) {
          maxCC = (FLOAT_DMEM)ccI;
        }

        FLOAT_DMEM thresh = threshCC_;
        if (useBrokenJitterThresh_) {
          // due to an earlier variable clash when introducing CC stats
          // we have to support the broken version in order to be compatible
          // to all models trained before 09.06.2016 which include Jitter!
          threshCC_ = minCC;
        }
        //printf("%f - %f\n", ccI, threshCC_);
        if (ccI > threshCC_) { 
// TODO: what happens the the lastPeriod, average etc. when a pitch period is discarded!? DEBUG THIS IN DETAIL!
          FLOAT_DMEM period = 0.0;
          if (usePeakToPeakPeriodLength_) {
            period = (FLOAT_DMEM)(((double)start + max1 - (double)os - max0) * T);
          } else {
            period = (FLOAT_DMEM)maxId;
          }
          avgPeriod += period;
          nPeriods += 1.0;
          if (lastT0 > 0.0) {
            FLOAT_DMEM diff = (FLOAT_DMEM)fabs(lastT0 - period);
            JitterLocal += diff;
            nPeriodsLocal += 1.0;
            if (lastDiff > 0.0) {
              JitterDDP += fabs(lastDiff - diff);
              nPeriodsDDP += 1.0;
            }
            lastDiff = diff;
          }
          lastT0 = period;
          // debug output
          // (TODO: save wave data properly to a pcm level, and save period start/end times..
          // TODO: save average waveform to output level...
          //saveFloatDmemVectorWlen_bin("dataJ.dat",mat->data+os,start-os,1);
          //saveDebugPeriod(lastIdx + start, (double)lastIdx + maxId);
          saveDebugPeriod(lastIdx + os + (int)round(max0), (double)lastIdx + os + max0);
          // Shimmer:
          avgAmp += (_a0 + _a1) / (FLOAT_DMEM)2.0;
          avgAmpDiff += ad;
        }
// TODO: send messages with pitch periods for pitch sync features!

        //SMILE_IMSG(2, "old start = %i ; new start = %i  (EV: %i - cc@max %f [%i %f] [T0f = %i])", os, start, lastIdx + start, cc[maxI-1], T0minF + maxI, maxId / T, T0f);
      } else {
        start += T0f;
      }
      if (start < toRead0 - 1) {
        lastPeriod = start;
      }
    }
    // The last period
    periodBuffer[numPeriods++] = start;

    //HNR, for last period not yet processed: compute averaged pitch period waveform for HNR
    // AND energy of averaged pitch period waveform (harmonic energy: H)
    FLOAT_DMEM Eh = 0.0;
    for (i=0; i<T0f && start+i < mat->nT; i++) {
      avgPeriodWf[i] += *(mat->data+(start+i));
      avgPeriodWf[i] /= (FLOAT_DMEM)numPeriods;
      if (i>2 && i<T0f-2) /* skip possibly unreliable beginning and end frames */
        Eh += avgPeriodWf[i]*avgPeriodWf[i];
    }
    if (T0f-4 > 0) {
      Eh /= (FLOAT_DMEM)(T0f - 4);
    }
    Eh = sqrt(Eh);

    //HNR, 2nd pass: compute energy of diff between each period and averaged waveform (noise energy N)
    long j;
    FLOAT_DMEM En = 0.0; long nEn = 0;
    if (pp > 0) {
      periodBuffer[numPeriods] = start+pp;
    }
    for (i=0; i<numPeriods; i++) {
      long n = 2;
      for (j=periodBuffer[i]+2; j<MIN(periodBuffer[i+1],periodBuffer[i]+T0f)-2; j++) {
        FLOAT_DMEM delta = mat->data[j] - avgPeriodWf[n++];
        En += delta*delta;
        nEn++;
      }
    }
    if (nEn > 0) En /= (FLOAT_DMEM)nEn;
    En = sqrt(En);

    // export variables
    eH = Eh;
    eN = En;
    if (En > 0.0) {
      HNR = Eh/En;
      if (HNR > 0.0) {
        lgHNR = (FLOAT_DMEM)(20.0 * log((double)HNR) / log(10.0));
      } else {
        lgHNR = lgHNRfloor;
      }
    }

    // laryngalisation
    if (numPeriods > 0) {
      sumCC /= (FLOAT_DMEM)numPeriods;

    }

    // update end pointer
    lastMis = toRead0 - lastPeriod; // - start;

    free(cc);
    free(periodBuffer);
    free(avgPeriodWf);

  } else { // for unvoiced frames:
    start = toRead0;
    lastPeriod = toRead0;
    lastMis = 0;
    // reset memory:
    lastT0 = 0.0; lastDiff=0.0;
    lastJitterDDP = 0.0; lastJitterLocal = 0.0;
    lastShimmerLocal = 0.0;

    //saveFloatDmemVectorWlen_bin("dataJ.dat",mat->data,toRead,1);  

    // HNR:
    // noise energy is now signal RMS energy.. TODO!
    if (noiseERMS || linearHNR || logHNR) {
      long i; double E=0.0;
      for (i=0; i<mat->nT; i++) {
        E += mat->data[i]*mat->data[i];
      }
      E /= (double)mat->nT;

      // export variables
      eH = 0.0;
      HNR = 0.0;
      eN = (FLOAT_DMEM)sqrt(E);
      lgHNR = lgHNRfloor;
    }
  }

  //lastIdx += start;
  lastIdx += lastPeriod;

  // build output vector:
  if (Nout == 0) return TICK_INACTIVE;

  if (onlyVoiced && (F0==0.0)) { return TICK_INACTIVE; }

  long n=0;
  if (out == NULL) out = new cVector(Nout);
  if (out == NULL) OUT_OF_MEMORY;

  
  if ((nPeriods>0.0)&&(nPeriodsLocal > 0.0)&&(F0>0.0)) {
    JitterLocal /= nPeriodsLocal;
    lastJitterLocal_b = lastJitterLocal = JitterLocal / (avgPeriod/nPeriods);
  }
  if (jitterLocal) {
    //printf("avgperiod = %f (over %f)  (F0=%f)\n", 1.0/(avgPeriod/nPeriods), nPeriods, F0);
    if ((nPeriods>0.0)&&(nPeriodsLocal > 0.0)&&(F0>0.0)) {
      //JitterLocal /= nPeriodsLocal;
      if (lastJitterLocal > 1.0) lastJitterLocal = 1.0;
      out->data[n] = lastJitterLocal; // = JitterLocal / (avgPeriod/nPeriods);
    } else {
      if ((nPeriods == 0.0)&&(F0>0.0)) {
        if (lastJitterLocal > 1.0) lastJitterLocal = 1.0;
        out->data[n] = lastJitterLocal;
      } else {
        out->data[n] = 0.0;
      }
    }
    n++;
  }
  if (jitterLocalEnv) {
    if (lastJitterLocal_b > 1.0) lastJitterLocal_b = 1.0;
    out->data[n] = lastJitterLocal_b;
    n++;
  }

  if ((nPeriods>0.0)&&(nPeriodsDDP > 0.0)&&(F0>0.0)) {
    JitterDDP /= nPeriodsDDP;
    lastJitterDDP_b = lastJitterDDP = JitterDDP / (avgPeriod/nPeriods);
  }
  if (jitterDDP) {
    if ((nPeriods>0.0)&&(nPeriodsDDP > 0.0)&&(F0>0.0)) {
      //JitterDDP /= nPeriodsDDP;
      if (lastJitterDDP > 1.0) lastJitterDDP = 1.0;
      out->data[n] = lastJitterDDP;
    } else {
      if ((nPeriods == 0.0)&&(F0>0.0)) {
        if (lastJitterDDP > 1.0) lastJitterDDP = 1.0;
        out->data[n] = lastJitterDDP;
      } else {
        out->data[n] = 0.0;
      }
    }
    n++;
  }

  if (jitterDDPEnv) {
    if (lastJitterDDP_b > 1.0) lastJitterDDP_b = 1.0;
    out->data[n] = lastJitterDDP_b;
    n++;
  }
  
  if ((nPeriods>0.0)&&(F0>0.0)) {
    if (avgAmp > 0.0) {
      lastShimmerLocal_b = lastShimmerLocal = (avgAmpDiff/avgAmp);
    } else {
      lastShimmerLocal=0.0;
    }
  }
  if (shimmerLocal || shimmerLocalDB) { // shimmer local
    if ((nPeriods>0.0)&&(F0>0.0)) {
      if (lastShimmerLocal > 1.0) lastShimmerLocal = 1.0;
      if (shimmerLocal) {
        out->data[n] = lastShimmerLocal;
        n++;
      } 
      if (shimmerLocalDB) {
        out->data[n] = (FLOAT_DMEM)smileDsp_amplitudeRatioToDB(lastShimmerLocal + 1.0);
        n++;
      }
    } else {
      if ((nPeriods == 0.0)&&(F0>0.0)) {
        if (lastShimmerLocal > 1.0) lastShimmerLocal = 1.0;
        if (shimmerLocal) {
          out->data[n] = lastShimmerLocal;
          n++;
        }
        if (shimmerLocalDB) {
          out->data[n] = (FLOAT_DMEM)smileDsp_amplitudeRatioToDB(lastShimmerLocal + 1.0);
          n++;
        }
      } else {
        if (shimmerLocal) {
          out->data[n] = 0.0;
          n++;
        }
        if (shimmerLocalDB) {
          out->data[n] = 0.0;
          n++;
        }
      }
    }
  }
  if (shimmerLocalEnv) {
    if (lastShimmerLocal_b > 1.0) lastShimmerLocal_b = 1.0;
    out->data[n] = lastShimmerLocal_b;
    n++;
  }

  if (harmonicERMS) {
//      printf("XX   Eh %f\n", eH);
    out->data[n++] = eH;
  }
  if (noiseERMS) {
    out->data[n++] = eN;
  }
  if (linearHNR) {
    out->data[n++] = HNR;
  }
  if (logHNR) {
    if (lgHNR < lgHNRfloor) lgHNR = lgHNRfloor;
    out->data[n++] = lgHNR;
  }
  if (refinedF0) {
    // TODO: refined F0 only for step size, i.e. only until toRead0
    if (nPeriods > 0.0 && F0 > 0.0) {
      out->data[n++] = (FLOAT_DMEM)1.0 / (avgPeriod / nPeriods);
    } else {
      out->data[n++] = 0.0;
    }
  }

  // larynx
  if (sourceQualityMean) {
    out->data[n++] = sumCC;
  }
  if (sourceQualityRange) {
    out->data[n++] = fabs(maxCC-minCC);
  }

  out->setTimeMeta(fvec->tmeta);
  writer_->setNextFrame(out);

  return TICK_SUCCESS;


////////////////////////////////////

#if 0
  double startVidx = fvec->tmeta->time / fvec->tmeta->period;
  
  double pitchT = fvec->tmeta->period; // pitch frame period
  double T = reader_->getLevelT(); // sample period

  double T0 = 1.0/F0; // pitch period

  long toRead = (long)ceil((pitchT+T0)/T)+lastMis;
  //double len = fvec->tmeta->lengthSec;

  double Tf = T0/T; // f0 period in frames (samples)
  long T0f = (long)round(Tf);

  // upper / lower T0 bounds:
  double T0min = (1.0-searchRangeRel)*T0;
  double T0max = (1.0+searchRangeRel)*T0;
  long T0minF = (long)floor(T0min/T);
  long T0maxF = (long)ceil(T0max/T);

  // maxmimum number of frames we may read...
  long maxAvail = lenF+lastMis;

  // get pcm data
  long nRead = maxAvail; // MIN(maxAvail,toRead);
  cMatrix *mat = reader_->getMatrix(lastIdx,nRead);

  lastIdx += nRead;
  if (mat == NULL) { 
    // frame not available..? If F0 frame was available this can only mean we have to catch up...
    
    // TODO : write NULL jitter/shimmer frame for proper sync?
    //...
    printf("Not avail\n");
    return TICK_SOURCE_NOT_AVAIL; 
  }

  if (F0 > 0.0) {
    // waveform matching  (TODO: sync to amplitude max.)
    double *cc = (double*)calloc(1,sizeof(double)*(T0maxF-T0minF+1));

    long start = 0;
    //printf("read N %i  2*T0maxF %i\n",mat->nT,2*T0maxF);
    // TODO: multi-channel (mat->N > 1) support!
    while (start<mat->nT - 2*T0maxF) {
      long _Tf;
      for (_Tf = T0minF; _Tf <= T0maxF; _Tf++) {
        long mid = start+_Tf;
        long end = start+2*_Tf;
        //if (end < vec->nT) {
        cc[_Tf-T0minF] = crossCorr(mat->data+start,_Tf,mat->data+mid,_Tf);
        //} 
      }

      // peak picking and (TODO: parabolic interpolation)
      double max=cc[T0f-T0minF]; long maxI=-1;
      //printf("T0maxF %i T0minF %i T0 %f, F0 %f, T0min %f , T0max %f T %f\n",T0maxF,T0minF,T0,F0,T0min,T0max,T);
      for (i=1; i<T0maxF-T0minF-1; i++) {
        printf("cc[%i] = %f\n",i,cc[i]);
        if ( (cc[i-1] < cc[i]) && (cc[i] > cc[i+1]) ) { // peak
          if (maxI == -1) {
            maxI = i; max = cc[i];
          } else {
            if (cc[i] > max) {
              maxI = i; max = cc[i];
            }
          }
        }
      }
      // TODO: parabolic interpolation of maxI for jitter comp. ??

      // assign new start
      long os = start;
      long pp = T0minF+maxI;
      if (maxI >= 0) {
        printf("jitter: %i  (maxI: %i)\n",pp-T0f,maxI);
        start += pp;
      } else {
        start += T0f;
        printf("no match\n");
      }
      saveFloatDmemVectorWlen_bin("dataJ.dat",mat->data+os,start-os,1);  


      // save period start...
    }



    free(cc);

    lastMis = nRead - start;

  } else {
    // compute lastMis (i.e.: maxAvail - lastPeriodEnd)
    lastMis = 0;
    //lastIdx += start;
  }
  
    // zero jitter/shimmer for unvoiced frames ?
// lastIdx += lenF;
// printf("unvoiced \n");
//}
  
// build output vector

// save output
  //writer->setNextFrame(vecO);
  return TICK_SUCCESS;  
#endif
}


cPitchJitter::~cPitchJitter()
{
  // cleanup...
  if (out != NULL) {
    delete out;
  }
  if (F0reader != NULL) {
    delete F0reader;
  }
  if (filehandle != NULL) {
    fclose(filehandle);
  }
}

