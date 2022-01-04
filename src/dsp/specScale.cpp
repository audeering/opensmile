/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

scaling of an FFT magnitude spectrum to non-linear axis:
 * log (OK)
 * bark (TODO)
 * semi-tone (=log2) (OK)
 * mel (TODO)
 * ... (TODO)

*/



#include <dsp/specScale.hpp>
#include <smileutil/smileUtilSpline.h>

#define MODULE "cSpecScale"

SMILECOMPONENT_STATICS(cSpecScale)

SMILECOMPONENT_REGCOMP(cSpecScale)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CSPECSCALE;
  sdescription = COMPONENT_DESCRIPTION_CSPECSCALE;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")

  // if the inherited config type was found, we register our configuration variables
  SMILECOMPONENT_IFNOTREGAGAIN( {} // <- this is only to avoid compiler warnings...
    // name append has a special role: it is defined in cDataProcessor, and can be overwritten here:
	  // if you set description to NULL, the existing description will be used, thus the following call can
  	// be used to update the default value:
    //ct->setField("nameAppend",NULL,"processed");
    ct->setField("scale","The target scale, one of the following:\n   'log(arithmic)' (logarithmic, see 'logScaleBase')\n   'oct(ave)' (octave scale = logarithmic with base 2)\n   'sem(itone)' (musical semi-tone scale)\n   'lin(ear)' (linear scale)\n   'bar(k)' (bark scale)\n   'bao' (old - pre 2.0 - approximation of bark scale)\n   'mel' (mel frequency scale)", "log");
    ct->setField("sourceScale","The source scale (currently only 'lin(ear)' is supported, all other options (as found for target scale) are experimental)", "lin");

    ct->setField("logScaleBase","The base for log scales (a log base of 2.0 - the default - corresponds to an octave target scale)", 2.0);  
    ct->setField("logSourceScaleBase","The base for log source scales (a log base of 2.0 - the default - corresponds to an octave target scale)", 2.0);  
    ct->setField("firstNote","The first note (in Hz) for a semi-tone scale", 55.0);  

    ct->setField("interpMethod","The interpolation method for rescaled spectra: 'none', 'spline'", "spline");  

    ct->setField("minF","The minimum frequency of the target scale", 25.0);  
    ct->setField("maxF","The maximum frequency of the target scale (-1.0 : set to maximum frequency of the source spectrum)", -1.0);
    ct->setField("nPointsTarget","The number of frequency points in target spectrum (<= 0 : same as input spectrum)", 0);

    ct->setField("specSmooth","1 = perform spectral smoothing before applying the scale transformation",0);
    ct->setField("specEnhance","1 = do spectral peak enhancement before applying smoothing (if enabled) and scale transformation",0);

    //ct->setField("psymodel",">0 : if target scale == bark, perform masking analysis and global loudness compensation and subtract combined masking threshold from spectrum (if set to 1, output spectrum floored to masking levels, where signal is below masking thresholds, if set to 2, outputs spectrum with masking levels subtracted, i.e. the perceptual spectrum)",0);

    ct->setField("auditoryWeighting","1 = enable post-scale auditory weighting (this is currently only supported for octave (log2) scales)",0);
/*
 * TODO: support auditory weighting for arbitrary scales, but esp. for the bark scale (copy from plp component...?)
 *
 * aud. weighting: Terhardt 1997
 * Tq(flin) = 3.64*pow( (f/1000.0), -0.8 ) - 6.5 exp(-0.6*pow( (f/1000.0) - 3.3 ,2)) + 0.001*pow(f/1000.0 , 4);  // dB SPL
 *
 * TODO: support for masking analysis in the linear (input) domain or bark output domain
 *
 */

  	ct->setField("processArrayFields",NULL,0);
  )

  // The configType gets automatically registered with the config manger by the SMILECOMPONENT_IFNOTREGAGAIN macro
  
  // we now create out sComponentInfo, including name, description, success status, etc. and return that
  SMILECOMPONENT_MAKEINFO(cSpecScale);
}

SMILECOMPONENT_CREATE(cSpecScale)

//-----

cSpecScale::cSpecScale(const char *_name) :
  cVectorProcessor(_name),
    fsSec(-1.0), magStart(0), nMag(0), 
    f_t(NULL), spline_work(NULL), 
    y(NULL), y2(NULL), audw(NULL)
{

}

void cSpecScale::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();
  
  
  const char *scaleStr = getStr("scale");
  SMILE_IDBG(2,"scale = '%s'",scaleStr);
  if (!strncasecmp(scaleStr,"log",3)) {
    scale = SPECTSCALE_LOG;
    logScaleBase = getDouble("logScaleBase");
    if ((logScaleBase <= 0.0)||(logScaleBase==1.0)) {
      SMILE_IERR(1,"logScaleBase must be > 0.0 and != 1.0 ! You have set it to: %f (I will set it to 2.0 now, but you are advised to correct your configuration!)",logScaleBase);
      logScaleBase = 2.0;
    }
  } else if (!strncasecmp(scaleStr,"oct",3)) {
    scale = SPECTSCALE_LOG;
    logScaleBase = 2.0;
  } else if (!strncasecmp(scaleStr,"sem",3)) {
    scale = SPECTSCALE_SEMITONE;
    firstNote = getDouble("firstNote");
  } else if (!strncasecmp(scaleStr,"lin",3)) {
    scale = SPECTSCALE_LINEAR;
  } else if (!strncasecmp(scaleStr,"bar",3)) {
      scale = SPECTSCALE_BARK;
  } else if (!strncasecmp(scaleStr,"bao",3)) {
      scale = SPECTSCALE_BARK_OLD;
  } else if (!strncasecmp(scaleStr,"mel",3)) {
     scale = SPECTSCALE_MEL;
  } else {
    SMILE_IERR(1,"unknown target scale type scale='%s' , please check your config!",scaleStr);
    COMP_ERR("aborting");
  }

  scaleStr = getStr("sourceScale");
  SMILE_IDBG(2,"sourceScale = '%s'",scaleStr);
  if (!strncasecmp(scaleStr,"log",3)) {
    sourceScale = SPECTSCALE_LOG;
    logSourceScaleBase = getDouble("logSourceScaleBase");
    if ((logSourceScaleBase <= 0.0)||(logSourceScaleBase==1.0)) {
      SMILE_IERR(1,"logSourceScaleBase must be > 0.0 and != 1.0 ! You have set it to: %f (I will set it to 2.0 now, but you are advised to correct your configuration!)",logSourceScaleBase);
      logScaleBase = 2.0;
    }
  } else if (!strncasecmp(scaleStr,"oct",3)) {
    sourceScale = SPECTSCALE_LOG;
    logSourceScaleBase = 2.0;
  } else if (!strncasecmp(scaleStr,"lin",3)) {
    sourceScale = SPECTSCALE_LINEAR;
  } else if (!strncasecmp(scaleStr,"bar",3)) {
      sourceScale = SPECTSCALE_BARK;
  } else if (!strncasecmp(scaleStr,"mel",3)) {
      sourceScale = SPECTSCALE_MEL;
  } else {
    SMILE_IERR(1,"unknown source scale type scale='%s' , please check your config!",scaleStr);
    COMP_ERR("aborting");
  }

  if (scale == SPECTSCALE_LOG) {
    SMILE_IDBG(2,"logScaleBase = %i",logScaleBase);
  }
  if (sourceScale == SPECTSCALE_LOG) {
    SMILE_IDBG(2,"logSourceScaleBase = %i",logSourceScaleBase);
  }

  specEnhance = getInt("specEnhance");
  SMILE_IDBG(2,"specEnhance = %i",specEnhance);

  specSmooth = getInt("specSmooth");
  SMILE_IDBG(2,"specSmooth = %i",specSmooth);

  auditoryWeighting = getInt("auditoryWeighting");
  if (auditoryWeighting) {
    if (!((scale==SPECTSCALE_LOG)&&(logScaleBase==2.0))) {
      auditoryWeighting = 0;
      SMILE_IWRN(1,"auditory weighting is currently only supported for octave target scales (log 2)! Disabling auditory weighting.");
    }
  }
  SMILE_IDBG(2,"auditoryWeighting = %i",auditoryWeighting);

  minF = getDouble("minF");
  if (minF < 1.0) {
    minF = 1.0;
    SMILE_IERR(1,"minF (%f) must be >= 1.0",minF);
  }
  SMILE_IDBG(2,"minF = %i",minF);
  maxF = getDouble("maxF");
  SMILE_IDBG(2,"maxF = %i",maxF);
  nPointsTarget = getInt("nPointsTarget");
  SMILE_IDBG(2,"nPointsTarget = %i",nPointsTarget);

  if (scale == SPECTSCALE_LOG) param = logScaleBase;
  else if (scale == SPECTSCALE_SEMITONE) param = firstNote;
  else param = 0.0;
}

int cSpecScale::setupNewNames(long nEl)
{
  if (fsSec == -1.0) {
    const sDmLevelConfig *c = reader_->getLevelConfig();
    fsSec = (float)(c->frameSizeSec);
  }

  const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();
  int ri=0;
  const char *base;
  long idx = fmeta->findFieldByPartialName( "Mag" , &ri );
  if (idx < 0) {
    nMag = nEl;
    magStart = 0;
    base=NULL;
    SMILE_IWRN(2,"FFT magnitude field '*Mag*' not found, defaulting to use full input vector!");
  } else {
    magStart = ri;
    nMag = fmeta->field[idx].N;
    base = fmeta->field[idx].name;
  }

  if (nMag+magStart > nEl) nMag = nEl-magStart;
  if (magStart < 0) magStart = 0;

  deltaF = 1.0/fsSec;
  if (nPointsTarget <= 0) {
    nPointsTarget = nMag;
  }

  int n=0;
  const char *scaleStr = getStr("scale");
  char *name;
  if (scaleStr!=NULL) {
    name = myvprint("%s_%c%c%cScale",base,scaleStr[0],scaleStr[1],scaleStr[2]);
  } else {
    name = myvprint("%s_scaledSpectrum",base);
  }
  writer_->addField( name, nPointsTarget ); n+=nPointsTarget;
  free(name);

  /* source scale frequency labels -> target scale frequency labels*/
  // todo: check info type!!
  if (fmeta->field[idx].dataType >= DATATYPE_SPECTRUM_BINS_FIRST && fmeta->field[idx].dataType <= DATATYPE_SPECTRUM_BINS_LAST) {
    double * _buf = (double *)malloc(fmeta->field[idx].infoSize);
    double * in = (double *)( fmeta->field[idx].info );
    long i;
    for (i=0; i<fmeta->field[idx].infoSize/(long)sizeof(double); i++) {
      _buf[i] = smileDsp_specScaleTransfFwd(in[i],scale,param);
    }
    writer_->setFieldInfo(-1 /* last field added */,fmeta->field[idx].dataType,_buf,fmeta->field[idx].infoSize);
  } else {
    SMILE_IWRN(2,"The data type of the input field is not of type DATATYPE_SPECTRUM_BINS_* , spectral power/magnitude bins are required as input to this component! Please check your config!");
  }

  namesAreSet_ = 1;
  return n;
}

int cSpecScale::dataProcessorCustomFinalise()
{
  int i;
  int ret = cVectorProcessor::dataProcessorCustomFinalise();
  if (!ret) return ret;

  // check maxF < sampleFreq.
  double samplF = deltaF * (double)(nMag - 1); // sampling frequency
  if ((maxF <= minF)||(maxF > samplF)) {
    maxF = samplF; 
  }
  
  // transform from source to target scale:

  fmin_t = smileDsp_specScaleTransfFwd(minF,scale,param); 
  fmax_t = smileDsp_specScaleTransfFwd(maxF,scale,param);

  // target delta f
  deltaF_t = (fmax_t - fmin_t) / (nPointsTarget - 1);

  // calculate the target frequencies of the linear scale fft input points
  if (f_t == NULL) f_t = (double*)malloc(sizeof(double)*nMag);

  if (scale == SPECTSCALE_LOG) {
    for (i=1; i < nMag; i++) {
      f_t[i] = smileDsp_specScaleTransfFwd( (double)i * (double)deltaF , scale, param );
    }
    f_t[0] = 2.0 * f_t[1] - f_t[2]; // heuristic for the 0th frequency (only valid for log2 ??)
  } else { // generic transform:
    for (i=0; i < nMag; i++) {
      f_t[i] = smileDsp_specScaleTransfFwd( (double)i * (double)deltaF , scale, param );
    }
  }

  splineCache = std::unique_ptr<sSmileMath_splineCache>(new sSmileMath_splineCache());
  smileMath_cspline_init(f_t, nMag, splineCache.get());

  double *x = (double*)malloc(sizeof(double)*nPointsTarget);  
  for (i=0; i < nPointsTarget; i++) {
    x[i] = fmin_t + (double)i * deltaF_t;
  }
  splintCache = std::unique_ptr<sSmileMath_splintCache>(new sSmileMath_splintCache());
  if (!smileMath_csplint_init(f_t, nMag, x, nPointsTarget, splintCache.get())) {
    SMILE_IERR(1,"smileMath_csplint_init failed. Output of this component will be invalid!");
    splintCache = NULL;
  }
  free(x);
  
  double nOctaves = log(maxF / minF)/log(2.0);
  double nPointsPerOctave = nPointsTarget / nOctaves; // this is valid for log(2.0) scale only...
  if (auditoryWeighting) {
    /* auditory weighting function (octave scale only...)*/
    double atan_s = nPointsPerOctave * smileMath_log2(65.0 / 50.0) - 1.0;
    audw = (double*)malloc(sizeof(double)*nPointsTarget);
    for (i=0; i < nPointsTarget; i++) {
      audw[i] = 0.5 + atan (3.0 * (i + 1 - atan_s) / nPointsPerOctave) / M_PI;
    }
  }

  cVectorMeta *mdata = writer_->getLevelMetaDataPtr();
  if (mdata != NULL) {
    //TODO: mdata ID!
    mdata->ID = 1001; /* SCALED_SPEC */;
    mdata->fData[0] = (FLOAT_DMEM)minF; /* min frequency (source) */
    mdata->fData[1] = (FLOAT_DMEM)maxF; /* max frequency (source) */
    mdata->fData[2] = (FLOAT_DMEM)nOctaves; /* number of octaves (log/octave scales only) */
    mdata->fData[3] = (FLOAT_DMEM)nPointsPerOctave; /* points per octave, valid only for log(2) scale! */
    mdata->fData[4] = (FLOAT_DMEM)fmin_t; /* min frequency (target) */
    mdata->fData[5] = (FLOAT_DMEM)fmax_t; /* max frequency (target) */
    mdata->fData[6] = (FLOAT_DMEM)scale; /* constant indicating the type of target scale */
    mdata->fData[7] = (FLOAT_DMEM)param; /* target scale param (if applicable) */
  }
  return ret;
}



// a derived class should override this method, in order to implement the actual processing
int cSpecScale::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // we assume we have fft magnitude as input...
  int i;
  if (nMag < 0) return 0;
  
  // allocate workspace arrays
  if (y == NULL) y = (double *)malloc(sizeof(double)*nMag);
  if (y2 == NULL) y2 = (double *)malloc(sizeof(double)*nMag);

  // copy and (possibly) convert input data
  for (i=magStart; i<magStart+nMag; i++) {
    y[i-magStart] = (double)src[i];
  }

  // do spectral peak enhancement, if enabled
  if (specEnhance) smileDsp_specEnhanceSHS(y,nMag);
  
  // do spectral smoothing, if enabled
  if (specSmooth) smileDsp_specSmoothSHS(y,nMag);

  // scale to the target scale and interpolate missing values via spline interpolation
  if (smileMath_cspline(f_t, y, nMag, 1e30, 1e30, y2, &spline_work, splineCache.get())) {
    // after successful spline computation, do the actual interpolation point by point
    double *out = (double*)malloc(sizeof(double)*nPointsTarget);
    smileMath_csplint(y, y2, splintCache.get(), out);
    for (i=0; i < nPointsTarget; i++) {
      dst[i] = (FLOAT_DMEM)out[i]; // save in output vector
    }
    free(out);
  } else {
    SMILE_IERR(3,"spline computation failed on current frame, zeroing the output (?!)");
    // zero output
    for (i=0; i < nPointsTarget; i++) {
      dst[i] = 0.0;
    }
  }


  // multiply by frequency selectivity of the auditory system (octave-scale only)
  if (auditoryWeighting) {
    for (i=0; i < nPointsTarget; i++) {
      if (dst[i] > 0.0) {
        dst[i] = (FLOAT_DMEM)( (double)dst[i] * audw[i] );
      } else {
        dst[i] = 0.0;
      }
    }
  }

  return nPointsTarget;
}

cSpecScale::~cSpecScale()
{
  if (y != NULL) free(y);
  if (y2 != NULL) free(y2);

  if (audw != NULL) free(audw);
  if (spline_work != NULL) free(spline_work);
  if (f_t != NULL) free(f_t);

  if (splineCache != NULL) smileMath_cspline_free(splineCache.get());
  if (splintCache != NULL) smileMath_csplint_free(splintCache.get());
}

