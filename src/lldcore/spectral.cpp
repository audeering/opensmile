/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:
computed spectral features such as flux, roll-off, centroid, etc.
*/


#include <lldcore/spectral.hpp>
#include <math.h>

#define MODULE "cSpectral"


SMILECOMPONENT_STATICS(cSpectral)

SMILECOMPONENT_REGCOMP(cSpectral)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CSPECTRAL;
  sdescription = COMPONENT_DESCRIPTION_CSPECTRAL;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("squareInput","1/0 = square input values (e.g. if input is magnitude and not power spectrum)",1);
    ct->setField("bands","bands[n] = LoFrq[Hz]-HiFrq[Hz]  (e.g. 0-250), compute energy in this spectral band (only integer frequencies are allowed!)","250-650",ARRAY_TYPE);
    ct->setField("normBandEnergies","(1/0=yes/no) normalise the band energies to the total frame energy (-> relative band energies). Also affects 'harmonicity', normalises the average min to max variations in the spectrum by the total frame energy (sum of magnitudes or squares).", 0);

    ct->setField("rollOff","rollOff[n] = X  (X in the range 0..1), compute X*100 percent spectral roll-off point",0.90,ARRAY_TYPE);
    ct->setField("specDiff","(1/0=yes/no) enable computation of spectral difference (root-mean-square of absolute differences over all bins)", 0);
    ct->setField("specPosDiff","(1/0=yes/no) enable computation of spectral positive difference (sum of squared positive differences normalised by number of bins and then sqrt taken)", 0);
    ct->setField("flux","(1/0=yes/no) enable computation of spectral flux",1);
    ct->setField("fluxCentroid","(1/0=yes/no) enable computation of spectral flux centroid (in Hz), i.e. the frequency with currently the most flux around it.", 0);
    ct->setField("fluxAtFluxCentroid","(1/0=yes/no) enable computation of spectral flux only at and around (+- 2 bins) the centroid of flux (as computed by the 'fluxCentroid' option).",0);
    ct->setField("centroid","(1/0=yes/no) enable computation of spectral centroid",1);
    ct->setField("maxPos","(1/0=yes/no) enable computation of position of spectral maximum",1);
    ct->setField("minPos","(1/0=yes/no) enable computation of position of spectral minimum",1);
    ct->setField("entropy","(1/0=yes/no) enable computation of spectral entropy",0);
    ct->setField("standardDeviation","(1/0=yes/no) enable computation of spectral standard deviation (root of variance)", 0);
    ct->setField("variance","(1/0=yes/no) enable computation of spectral variance (mpeg7: spectral spread)",0);
    ct->setField("skewness","(1/0=yes/no) enable computation of spectral skewness",0);
    ct->setField("kurtosis","(1/0=yes/no) enable computation of spectral kurtosis",0);
    ct->setField("slope","(1/0=yes/no) enable computation of spectral slope over maximal frequency range",0);
    ct->setField("slopes","slopes[n] = LoFrq[Hz]-HiFrq[Hz] (e.g. 0-5000), computes spectral slope in the given band (only integer frequencies are allowed!)", (const char*)NULL, ARRAY_TYPE);
    ct->setField("alphaRatio", "(1/0=yes/no) enable computation of alpha ratio (ratio of energy above 1 kHz (up to 5 kHz) to energy below 1 kHz).", 0);
    ct->setField("hammarbergIndex", "(1/0=yes/no) enable computation of hammarberg index (ratio of energy peak (max) in 0-2 kHz region and energy peak (max) in 2-5 kHz region).", 0);
    ct->setField("sharpness","(1/0=yes/no) enable computation of psychoacoustic parameter 'sharpness'. In order to obtain proper values, use a bark scale spectrum as input (see cSpecScale component).",0);
    ct->setField("tonality","(1/0=yes/no) enable computation of consonance (ratio of consonance/dissonance, based on intervals between spectral peaks). (NOT YET IMPLEMENTED)",0);
    ct->setField("harmonicity","(1/0=yes/no) enable computation of harmonicity (mean of consecutive local min-max differences). Optionally normalised by the total frame energy, if normBandEnergies is set to 1.", 0);
    ct->setField("flatness","(1/0=yes/no) enable computation of spectral flatness (sfm = geometric_mean / arithmetic_mean of spectrum).",0);
    ct->setField("logFlatness","(1/0=yes/no) if flatness is enabled, output ln(flatness).", 0);

    ct->setField("buggyRollOff", "(1/0=yes/no) for backwards feature set compatibility, enable buggy roll-off computation (pre May 2013, pre 2.0 release).", 0);
    ct->setField("oldSlopeScale", "(1/0=yes/no) for backwards feature set compatibility, enable (incorrectly) scaled spectral slope computation (pre July 2013, pre 2.0 final release). Enabled by default, to preserve compatibility with older feature sets. Disable in new designs!", 1);
    ct->setField("freqRange", "range of spectrum to consider for spectral parameter computation (syntax: lowerHz-upperHz, e.g. 0-8000; use 0-0 (default) for full range)", "0-0");
    ct->setField("useLogSpectrum", "(1/0=yes/no) Compute the following parameters (if enabled) from the log spectrum instead of the power spectrum: spectral slope(s), centroid, maxpos/minpos, entropy, moments, sharpness, harmonicity, flatness. Please note, that the band energies are computed from the power spectrum, but the output will be in dB (log) if this option is enabled (1). Spectral roll-off and flux will always be computed from the power spectrum (no log).", 0);
    ct->setField("specFloor", "When using the log Spectrum, the square(!) of this value is used as a floor value for the power spectrum.", 0.0000001);
  )

  SMILECOMPONENT_MAKEINFO(cSpectral);
}

SMILECOMPONENT_CREATE(cSpectral)

cSpectral::cSpectral(const char *_name) :
  cVectorProcessor(_name),
  squareInput(1), nBands(0), nSlopes(0), nRollOff(0), entropy(0),
  bandsL(NULL), bandsH(NULL), 
  slopesL(NULL), slopesH(NULL),
  rollOff(NULL),
  bandsLi(NULL), bandsHi(NULL),
  wghtLi(NULL), wghtHi(NULL),
  slopeBandsLi(NULL), slopeBandsHi(NULL),
  slopeWghtLi(NULL), slopeWghtHi(NULL),
  fsSec(-1.0), frq(NULL), frqScale(-1), frqScaleParam(0.0),
  buggyRollOff(0), buggySlopeScale(0),
  specRangeLower(0), specRangeUpper(0),
  specRangeLowerBin(-1), specRangeUpperBin(-1),
  specFloor((FLOAT_DMEM)(0.0000001 * 0.0000001)),
  useLogSpectrum(0), logFlatness(0),
  requireMagSpec(true), requireLogSpec(true), requirePowerSpec(true),
  prevSpec(NULL), nSrcPrevSpec(NULL), nFieldsPrevSpec(0),
  sharpnessWeights(NULL)
{
  logSpecFloor = (FLOAT_DMEM)(10.0 * log(specFloor) / log (10.0));
}

void cSpectral::parseRange(const char *text, long *lowerHz, long *upperHz)
{
  if (text == NULL || lowerHz == NULL || upperHz == NULL) {
    return;
  }
  char *val = strdup(text);
  char *orig = strdup(val);
  int l = (int)strlen(val);
  int err = 0;
  char *s2 = strchr(val, '-');
  if (s2 != NULL) {
    *(s2++) = 0;
    char *ep = NULL;
    long r1 = strtol(val, &ep, 10);
    if ((r1 == 0) && (ep == val)) {
      err = 1;
    } else if (r1 < 0) {
      SMILE_IERR(1, "lower frequency of frequency '%s'  is out of range (allowed: [0..+inf])", orig);
    }
    ep = NULL;
    long r2 = strtol(s2, &ep, 10);
    if ((r2 == 0) && (ep == val)) {
      err = 1;
    } else {
      if (r2 < 0) {
        SMILE_IERR(1, "upper frequency of frequency range in '%s'  is out of range (allowed: [0..+inf])", orig);
      }
    }
    if (!err) {
      if (r1 <= r2) {
        *lowerHz = r1;
        *upperHz = r2;
      } else {
        *lowerHz = r2;
        *upperHz = r1;
      }
    }
  } else { err = 1; }

  if (err==1) {
    SMILE_IERR(1, "Error parsing '%s'! (The frequency range must be X-Y, where X and Y are positive integer numbers specifiying frequency in Hertz!)", orig);
    *lowerHz = -1;
    *upperHz = -1;
  }
  free(orig);
  free(val);
}

int cSpectral::parseBandsConfig(const char * field, long ** bLow, long ** bHigh)
{
  int N = getArraySize(field);
  if (N > 0) {
    long * bL = (long *)calloc(1,sizeof(long)*N);
    long * bH = (long *)calloc(1,sizeof(long)*N);
    for (int i = 0; i < N; i++) {
        const char *val = getStr_f(myvprint("%s[%i]", field, i));
        if (val == NULL) {
          bL[i] = -1;
          bH[i] = -1;
          continue;
        }
        char *tmp = strdup(val);
        char *orig = strdup(tmp);
        int l = (int)strlen(tmp);
        int err=0;
        char *s2 = strchr(tmp,'-');
        if (s2 != NULL) {
          *(s2++) = 0;
          char *ep=NULL;
          long r1 = strtol(tmp,&ep,10);
          if ((r1==0)&&(ep==tmp)) { 
            err=1; 
          } else if (r1 < 0) {
            SMILE_IERR(1,"(option '%s') low frequency of band %i in '%s'  is out of range (allowed: [0..+inf])", field, i, orig);
          }
          ep=NULL;
          long r2 = strtol(s2,&ep,10);
          if ((r2 == 0) && (ep == tmp)) { 
            err = 1;
          } else {
            if (r2 < 0) {
              SMILE_IERR(1,"(option '%s') high frequency of band %i in '%s'  is out of range (allowed: [0..+inf])", field, i, orig);
            }
          }
          if (!err) {
            if (r1 < r2) {
              bL[i] = r1;
              bH[i] = r2;
            } else {
              bL[i] = r2;
              bH[i] = r1;
            }
          }
        } else { err=1; }

        if (err==1) {
          SMILE_IERR(1, "(inst '%s', option '%s') Error parsing %s[%i] = '%s'! (The band range must be X-Y, where X and Y are positive integer numbers specifiying frequency in Hertz!)",
                    getInstName(),field, field, i, orig);
          bL[i] = -1;
          bH[i] = -1;
        }
        free(orig);
        free(tmp);
    }
    if (bLow != NULL) {
      if (*bLow != NULL) free(bLow);
      *bLow = bL;
    }
    if (bHigh != NULL) {
      if (*bHigh != NULL) free(bHigh);
      *bHigh = bH;
    }
  }
  return N;
}

void cSpectral::setRequireLorPspec()
{
  if (useLogSpectrum) {
    requireLogSpec = true;
  } else {
    requirePowerSpec = true;
  }
}

void cSpectral::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();
  
  normBandEnergies = getInt("normBandEnergies");

  squareInput = getInt("squareInput");
  if (squareInput) { SMILE_IDBG(2,"squaring of input values enabled"); }

  useLogSpectrum = getInt("useLogSpectrum");
  if (useLogSpectrum) {
    specFloor = (FLOAT_DMEM)getDouble("specFloor");
    if (specFloor <= 0.0) {
      SMILE_IWRN(1, "specFloor must be > 0.0. Re-setting it to the default of 0.0000001");
    }
    specFloor = specFloor * specFloor;
    logSpecFloor = (FLOAT_DMEM)(10.0 * log(specFloor) / log (10.0));
    SMILE_IMSG(2, "logSpecFloor = %.2f  (specFloor = %e)", logSpecFloor, specFloor);
  }

  specDiff = getInt("specDiff");
  if (specDiff) {
    requireMagSpec = true;
  }
  specPosDiff = getInt("specPosDiff");
  if (specPosDiff) {
    requireMagSpec = true;
  }
  flux = getInt("flux");
  if (flux) {
    requireMagSpec = true;
  }
  fluxCentroid = getInt("fluxCentroid");
  if (flux) {
    requireMagSpec = true;
  }
  fluxAtFluxCentroid = getInt("fluxAtFluxCentroid");
  if (flux) {
    requireMagSpec = true;
  }
  /*
  fluxAtSpecCentroid = getInt("fluxAtSpecCentroid");
  if (flux) {
    requireMagSpec = true;
  }
  */
  centroid = getInt("centroid");
  if (centroid) {
    setRequireLorPspec();
    SMILE_IDBG(2,"spectral centroid computation enabled");
  }
  maxPos = getInt("maxPos");
  if (maxPos) {
    setRequireLorPspec();
    SMILE_IDBG(2,"spectral max. pos. computation enabled");
  }
  minPos = getInt("minPos");
  if (minPos) {
    setRequireLorPspec();
    SMILE_IDBG(2,"spectral min. pos. computation enabled");
  }

  entropy = getInt("entropy");
  if (entropy) {
    setRequireLorPspec();
    SMILE_IDBG(2,"spectral entropy computation enabled");
  }
  standardDeviation = getInt("standardDeviation");
  if (standardDeviation) {
    setRequireLorPspec();
    SMILE_IDBG(2,"spectral variance computation enabled");
  }
  variance = getInt("variance");
  if (variance) {
    setRequireLorPspec();
    SMILE_IDBG(2,"spectral variance computation enabled");
  }
  skewness = getInt("skewness");
  if (skewness) {
    setRequireLorPspec();
    SMILE_IDBG(2,"spectral skewness computation enabled");
  }
  kurtosis = getInt("kurtosis");
  if (kurtosis) {
    setRequireLorPspec();
    SMILE_IDBG(2,"spectral kurtosis computation enabled");
  }
  slope = getInt("slope");
  if (slope) {
    setRequireLorPspec();
    SMILE_IDBG(2,"spectral slope computation enabled");
  }
  buggyRollOff = getInt("buggyRollOff");
  buggySlopeScale = getInt("oldSlopeScale");

  alphaRatio = getInt("alphaRatio");
  if (alphaRatio) {
    requirePowerSpec = true;
  }
  hammarbergIndex = getInt("hammarbergIndex");
  if (hammarbergIndex) {
    requirePowerSpec = true;
  }

  nBands = parseBandsConfig("bands", &bandsL, &bandsH);
  if (nBands > 0) {
    requirePowerSpec = true;
  }
  nSlopes = parseBandsConfig("slopes", &slopesL, &slopesH);
  if (nSlopes > 0) {
    setRequireLorPspec();
  }
  const char * specRange = getStr("freqRange");
  parseRange(specRange, &specRangeLower, &specRangeUpper);

  nRollOff = getArraySize("rollOff");
  if (nRollOff > 0) {
    requirePowerSpec = true;
    rollOff = (double*)calloc(1,sizeof(double)*nRollOff);
    for (int i = 0; i < nRollOff; i++) {
      rollOff[i] = getDouble_f(myvprint("rollOff[%i]",i));
      if (rollOff[i] < 0.0) {
        SMILE_IERR(1,"rollOff[%i] = %f is out of range (allowed 0..1), clipping to 0.0",i,rollOff[i]);
        rollOff[i] = 0.0;
      }
      else if (rollOff[i] > 1.0) {
        SMILE_IERR(1,"rollOff[%i] = %f is out of range (allowed 0..1), clipping to 1.0",i,rollOff[i]);
        rollOff[i] = 1.0;
      }
    }
  }

  sharpness = getInt("sharpness");
  if (sharpness) {
    requirePowerSpec = true;
    SMILE_IDBG(2,"sharpness computation enabled");
  }
  tonality = getInt("tonality");
  if (tonality) {
    //SMILE_IDBG(2,"tonality computation enabled");
    SMILE_IERR(1, "tonality (spectral) is not yet implemented!");
  }
  harmonicity = getInt("harmonicity");
  if (harmonicity) {
    setRequireLorPspec();
    SMILE_IDBG(2,"harmonicity computation enabled");
  }
  flatness = getInt("flatness");
  if (flatness) {
    setRequireLorPspec();
    SMILE_IDBG(2,"spectral flatness computation enabled");
  }
  logFlatness = getInt("logFlatness");
  if (logFlatness) {
    SMILE_IDBG(2, "output of log flatness (=ln(flatness)) is enabled");
  }
}


int cSpectral::setupNamesForField(int i, const char*name, long nEl)
{
  int newNEl = 0;
  int ii;
  if (fsSec == -1.0) {
    const sDmLevelConfig *c = reader_->getLevelConfig();
    fsSec = c->frameSizeSec;
  }
  if (useLogSpectrum) {
    for (ii=0; ii<nBands; ii++) {
      if (isBandValid(bandsL[ii],bandsH[ii])) {
        char *xx = myvprint("%s_logFband%i-%i",name,bandsL[ii],bandsH[ii]);
        writer_->addField( xx, 1 ); newNEl++; free(xx);
      }
    }
  } else {
    for (ii=0; ii<nBands; ii++) {
      if (isBandValid(bandsL[ii],bandsH[ii])) {
        char *xx = myvprint("%s_fband%i-%i",name,bandsL[ii],bandsH[ii]);
        writer_->addField( xx, 1 ); newNEl++; free(xx);
      }
    }
  }
  if (useLogSpectrum) {
    for (ii=0; ii<nSlopes; ii++) {
      if (isBandValid(slopesL[ii], slopesH[ii])) {
        char *xx = myvprint("%s_logSpectralSlopeOfBand%i-%i",name,slopesL[ii],slopesH[ii]);
        writer_->addField( xx, 1 ); newNEl++; free(xx);
      }
    }
  } else {
    for (ii=0; ii<nSlopes; ii++) {
      if (isBandValid(slopesL[ii], slopesH[ii])) {
        char *xx = myvprint("%s_spectralSlopeOfBand%i-%i",name,slopesL[ii],slopesH[ii]);
        writer_->addField( xx, 1 ); newNEl++; free(xx);
      }
    }
  }

  if (useLogSpectrum) {
    if (alphaRatio) {
      char *xx = myvprint("%s_alphaRatioDB",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (hammarbergIndex) {
      char *xx = myvprint("%s_hammarbergIndexDB",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
  } else {
    if (alphaRatio) {
      char *xx = myvprint("%s_alphaRatio",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (hammarbergIndex) {
      char *xx = myvprint("%s_hammarbergIndex",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
  }

  for (ii=0; ii<nRollOff; ii++) {
    char *xx = myvprint("%s_spectralRollOff%.1f",name,rollOff[ii]*100.0);
    writer_->addField( xx, 1 ); newNEl++; free(xx);
  }
  if (specDiff) {
    char *xx = myvprint("%s_spectralAbsoluteDifference",name);
    writer_->addField( xx, 1 ); newNEl++; free(xx);
  }
  if (specPosDiff) {
    char *xx = myvprint("%s_spectralPositiveDifference",name);
    writer_->addField( xx, 1 ); newNEl++; free(xx);
  }
  if (flux) {
    char *xx = myvprint("%s_spectralFlux",name);
    writer_->addField( xx, 1 ); newNEl++; free(xx);
  }
  if (fluxCentroid) {
    char *xx = myvprint("%s_spectralFluxCentroid",name);
    writer_->addField( xx, 1 ); newNEl++; free(xx);
  }
  if (fluxAtFluxCentroid) {
    char *xx = myvprint("%s_spectralFluxAtFluxCentroid",name);
    writer_->addField( xx, 1 ); newNEl++; free(xx);
  }

  if (useLogSpectrum) {
    if (centroid) {
      char *xx = myvprint("%s_logSpectralCentroid",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (maxPos) {
      char *xx = myvprint("%s_spectralMaxPos",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (minPos) {
      char *xx = myvprint("%s_spectralMinPos",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (entropy) {
      char *xx = myvprint("%s_logSpectralEntropy",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (standardDeviation) {
      char *xx = myvprint("%s_logSpectralStdDev",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (variance) {
      char *xx = myvprint("%s_logSpectralVariance",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (skewness) {
      char *xx = myvprint("%s_logSpectralSkewness",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (kurtosis) {
      char *xx = myvprint("%s_logSpectralKurtosis",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (slope) {
      char *xx = myvprint("%s_logSpectralSlope",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (sharpness) {
      char *xx = myvprint("%s_psySharpness",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (tonality) {
      char *xx = myvprint("%s_logSpectralTonality",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (harmonicity) {
      char *xx = myvprint("%s_logSpectralHarmonicity",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (flatness) {
      if (logFlatness) {
        char *xx = myvprint("%s_logSpectralFlatnessLog",name);
        writer_->addField( xx, 1 ); newNEl++; free(xx);
      } else {
        char *xx = myvprint("%s_logSpectralFlatness",name);
        writer_->addField( xx, 1 ); newNEl++; free(xx);
      }
    }
  } else {
    if (centroid) {
      char *xx = myvprint("%s_spectralCentroid",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (maxPos) {
      char *xx = myvprint("%s_spectralMaxPos",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (minPos) {
      char *xx = myvprint("%s_spectralMinPos",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (entropy) {
      char *xx = myvprint("%s_spectralEntropy",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (standardDeviation) {
      char *xx = myvprint("%s_spectralStdDev",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (variance) {
      char *xx = myvprint("%s_spectralVariance",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (skewness) {
      char *xx = myvprint("%s_spectralSkewness",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (kurtosis) {
      char *xx = myvprint("%s_spectralKurtosis",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (slope) {
      char *xx = myvprint("%s_spectralSlope",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (sharpness) {
      char *xx = myvprint("%s_psySharpness",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (tonality) {
      char *xx = myvprint("%s_spectralTonality",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (harmonicity) {
      char *xx = myvprint("%s_spectralHarmonicity",name);
      writer_->addField( xx, 1 ); newNEl++; free(xx);
    }
    if (flatness) {
      if (logFlatness) {
        char *xx = myvprint("%s_spectralFlatnessLog",name);
        writer_->addField( xx, 1 ); newNEl++; free(xx);
      } else {
        char *xx = myvprint("%s_spectralFlatness",name);
        writer_->addField( xx, 1 ); newNEl++; free(xx);
      }
    }
  }
  if (i >= nFieldsPrevSpec) {
    nFieldsPrevSpec = i + 1;
  }
  return newNEl;
}

// a derived class should override this method, in order to implement the actual processing
int cSpectral::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  long n = 0;
  long i = 0;
  long j = 0;

  double N = (double) ((Nsrc-1)*2);  // assumes FFT magnitude input array!!
  double F0 = 1.0/fsSec;

  if (frq == NULL) { // input block's frequency axis
    // TODO: use frqSet flag to avoid doing the code below on every tick if info==NULL
    const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();
    if ((fmeta != NULL)&&(idxi < fmeta->N)) {
      // TODO: check input block's type (in fmeta)
      // fmeta->field[idxi].dataType & SPECTRAL == 1 ?
      nScale = fmeta->field[idxi].infoSize / sizeof(double);
      frq = (double *)(fmeta->field[idxi].info);
      if (nScale > 0) {
        // old frequency axis computation mode:
        //nScale=0;
        if (nScale != Nsrc) {
          SMILE_IWRN(2,"number of frequency axis points (from info struct) [%i] is not equal to Nsrc [%i] ! Field index: %i (check the processArrayFields option).",nScale,Nsrc,idxi);
          nScale = MIN(nScale,Nsrc);
        }
      }
    }
  }

  if (frqScale == -1) {
      cVectorMeta *mdata = writer_->getLevelMetaDataPtr();
      if (mdata != NULL && mdata->ID == 1001 /* SCALED_SPEC */) {
        frqScale = (int)mdata->fData[6];
        frqScaleParam = (double)mdata->fData[7];
      } else {
        frqScale = SPECTSCALE_LINEAR;
        frqScaleParam = 0.0;
      }
  }

  if (specRangeLowerBin == -1) {
    if (specRangeLower == specRangeUpper && specRangeUpper == 0) {
      specRangeLowerBin = 1;
      specRangeUpperBin = Nsrc - 1;
    } else {
      for (i = 0; i < Nsrc; i++) {
        if ((double)specRangeLower >= frq[i]) {
          specRangeLowerBin = i;
        }
        if ((double)specRangeUpper > frq[i]) {
          specRangeUpperBin = i;
        }
      }
      if (specRangeUpperBin == -1 || specRangeUpperBin >= Nsrc) {
        specRangeUpperBin = Nsrc - 1;
      }
      if (specRangeLowerBin < 0) {
        specRangeLowerBin = 0;
      }
    }
    SMILE_IMSG(3, "specRangeLower (Hz) = %i (bin = %i) ; specRangeUpper (Hz) = %i (bin = %i)",
        specRangeLower, specRangeLowerBin, specRangeUpper, specRangeUpperBin);
  }
  long nBins = specRangeUpperBin - specRangeLowerBin + 1;

  if (Nsrc <= 0) {
    return 0;
  }

  const FLOAT_DMEM *srcM = NULL; // magnitude spectrum
  const FLOAT_DMEM *srcL = NULL; // log spectrum
  const FLOAT_DMEM *srcP = NULL; // power spectrum
  const FLOAT_DMEM *srcLP = NULL; // log or power spectrum
  FLOAT_DMEM logSpecFactor = 1.0;
  // TODO: we might get lof spectral densities as input
  //        read the input type field and support all possible conversions!
  if (requireMagSpec) {
    if (squareInput) {
      srcM = src;
    } else {
      // compute linear from squared
      FLOAT_DMEM *srcTmp = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM) * Nsrc);
      for (i = 0; i < Nsrc; i++) {
        if (src[i] > 0.0) {
          srcTmp[i] = sqrt(src[i]);
        } else {
          srcTmp[i] = 0.0;
        }
      }
      srcM = srcTmp;
    }
  }
  if (requirePowerSpec) {
    if (squareInput) {
      // compute squared from linear
      FLOAT_DMEM *srcTmp = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM) * Nsrc);
      for (i = 0; i < Nsrc; i++) {
        srcTmp[i] = src[i] * src[i];
      }
      srcP = srcTmp;
    } else {
      srcP = src;
    }
  }
  if (requireLogSpec) {
    logSpecFactor = (FLOAT_DMEM)(10.0 / log(10.0));
    FLOAT_DMEM myLogSpecFactor = logSpecFactor;
    const FLOAT_DMEM *mySrc = NULL;
    if (requirePowerSpec) {
      // compute the log spectrum from the power spectrum we already have
      mySrc = srcP;
    } else if (requireMagSpec) {
      // compute the log spectrum from the magnitude spectrum instead
      mySrc = srcM;
      logSpecFactor *= 2.0;
    } else {
      // find out what we have to do...
      mySrc = src;
      if (squareInput) {
        logSpecFactor *= 2.0;
      }
    }
    FLOAT_DMEM *srcTmp = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM) * Nsrc);
    for (i = 0; i < Nsrc; i++) {
      if (mySrc[i] <= specFloor) {
        srcTmp[i] = logSpecFloor;
      } else {
        srcTmp[i] = myLogSpecFactor * log(mySrc[i]);
      }
    }
    srcL = srcTmp;
  }
  if (useLogSpectrum) {
    srcLP = srcL;
  } else {
    srcLP = srcP;
  }

  /*
  if (squareInput || useLogSpectrum) {
    src2 = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*Nsrc);
    // NOTE: bands and slopes can be outside of the specRange parameter!
    // thus, we need to square all inputs!
    if (useLogSpectrum) {
      src3 = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*Nsrc);
      logSpecFactor = 10.0 / log(10.0);
      if (squareInput) {
        logSpecFactor *= 2.0;
        for (i = 0; i < Nsrc; i++) {
          if (src[i] < specFloor) {
            src2[i] = logSpecFloor;
          } else {
            src2[i] = logSpecFactor * log(src[i]);
          }
          src3[i] = src[i] * src[i];
        }
      } else {
        for (i = 0; i < Nsrc; i++) {
          if (src[i] < specFloor) {
            src2[i] = logSpecFloor;
          } else {
            src2[i] = logSpecFactor * log(src[i]);
          }
          src3[i] = src[i];
        }
      }
    } else if (squareInput) {
      for (i = 0; i < Nsrc; i++) {
        src2[i] = src[i]*src[i];
      }
    }
  } else {
    srcP = (FLOAT_DMEM *)src;  // typecast ok here.. since we treat _src as read-only below...
  }
  */
  
  // compute total frame energy
  double frameSum = 0.0;
  if (normBandEnergies || sharpness || nRollOff > 0) {
    for (i = specRangeLowerBin; i <= specRangeUpperBin; i++) {
      frameSum += srcP[i];
    }
  }

  // process spectral bands (rectangular "filter")
  // NOTE: bands and slopes can be outside of the specRange parameter!
  for (i=0; i<nBands; i++) {
    if (isBandValid(bandsL[i],bandsH[i])) {
      double idxL;
      double wghtL;
      if ((nScale < Nsrc)||(frq==NULL)) {
        idxL =  (double)bandsL[i] / F0 ;
        wghtL =  ceil(idxL) - idxL;
      } else {
        if (bandsLi == NULL) { // map the frequency to an fft bin index
          int ii,iii;
          bandsLi=(double*)malloc(sizeof(double)*nBands);
          wghtLi=(double*)malloc(sizeof(double)*nBands);
          for (iii=0; iii<nBands; iii++) {
            for (ii=0; ii<nScale; ii++) {
              if (frq[ii] > (double)bandsL[iii]) break;
            }
            if ((ii<nScale)&&(ii>0)) {
              wghtLi[iii] = (frq[ii]-(double)bandsL[iii])/(frq[ii] - frq[ii-1]);
            } else { wghtLi[iii] = 1.0; }
            
            bandsLi[iii] = (double)ii-1.0;
            if (bandsLi[iii] < 0) bandsLi[iii] = 0;
            if (bandsLi[iii] >= Nsrc) bandsLi[iii] = Nsrc;
          }
        }
        idxL = bandsLi[i];
        wghtL = wghtLi[i];
      }
      if (wghtL == 0.0) wghtL = 1.0;

      double idxR;
      double wghtR ;
      if ((nScale < Nsrc)||(frq==NULL)) {
        idxR =  (double)bandsH[i] / F0 ;
        wghtR = idxR-floor(idxR);
      } else {
        if (bandsHi == NULL) { // map the frequency to an fft bin index
          int ii, iii;
          bandsHi=(double*)malloc(sizeof(double)*nBands);
          wghtHi=(double*)malloc(sizeof(double)*nBands);
          for (iii=0; iii<nBands; iii++) {
            for (ii=0; ii<nScale; ii++) {
              if (frq[ii] >= (FLOAT_DMEM)bandsH[iii]) break;
            }
            if ((ii<nScale)&&(ii>0)) {
              wghtHi[iii] = ((double)bandsH[iii]-frq[ii-1])/(frq[ii] - frq[ii-1]);
            } else { wghtHi[iii] = 1.0; }
            //if (wghtHi[iii] == 0.0) { wghtHi[iii] = 1.0; }
            if ((ii<nScale)&&(frq[ii] == (FLOAT_DMEM)bandsH[iii])) {
              bandsHi[iii] = (double)ii;
            } else {
              bandsHi[iii] = (double)ii-1.0;
            }
            if (bandsHi[iii] >= Nsrc) bandsHi[iii] = Nsrc-1;
          }
        }
        idxR = bandsHi[i];
        wghtR = wghtHi[i];
      }
      // TODO: interpolation instead of rounding boundaries to next lower bin and next higher bin
      // TODO: spectral band filter shapes...
      // TODO: debug output of actual filter bandwidth & range based on rounded (or interpolated) boundaries!

      if (wghtR == 0.0) wghtR = 1.0;
  
      long iL = (long)floor(idxL);
      long iR = (long)floor(idxR);

      if (iL >= Nsrc) { iL=iR=Nsrc-1; wghtR=0.0; wghtL=0.0; }
      if (iR >= Nsrc) { iR=Nsrc-1; wghtR = 1.0; } 
      if (iL < 0) iL=0;
      if (iR < 0) iR=0;
      double sum=(double)srcP[iL]*wghtL;
      for (j = iL + 1; j < iR; j++) {
        sum += (double)srcP[j];
      }
      sum += (double)srcP[iR]*wghtR;
      
      if (normBandEnergies) {
        // normalise band energy to frame energy
        if (frameSum > 0.0) {  // energy ratio .. never in dB
          dst[n++] = (FLOAT_DMEM)(sum/frameSum);
        } else {
          dst[n++] = (FLOAT_DMEM)0.0;
        }
      } else {
        // normalise band energy to frame size
        if (nBins > 0) {
          if (useLogSpectrum) {
            dst[n++] = (FLOAT_DMEM)(10.0 * log(sum/(double)nBins) / log(10.0));
          } else {
            dst[n++] = (FLOAT_DMEM)(sum/(double)nBins);
          }
        } else {
          SMILE_IWRN(3, "0 bins in band %i!", i);
          dst[n++] = 0.0;
        }
      }
    }
  }
  
  // compute spectral slopes in bands
  for (i=0; i<nSlopes; i++) {
    if (isBandValid(slopesL[i], slopesH[i])) {
      double idxL = 0.0;
      double wghtL = 0.0;
      if ((nScale < Nsrc) || (frq == NULL)) {
        idxL =  (double)slopesL[i] / F0 ;
        wghtL =  ceil(idxL) - idxL;
      } else {
        if (slopeBandsLi == NULL) { // map the frequency to an fft bin index
          int ii, iii;
          slopeBandsLi = (double*)malloc(sizeof(double)*nSlopes);
          slopeWghtLi = (double*)malloc(sizeof(double)*nSlopes);
          for (iii=0; iii<nSlopes; iii++) {
            for (ii=0; ii<nScale; ii++) {
              if (frq[ii] > (double)slopesL[iii]) break;
            }
            if ((ii<nScale)&&(ii>0)) {
              slopeWghtLi[iii] = (frq[ii]-(double)slopesL[iii])/(frq[ii] - frq[ii-1]);
            } else { slopeWghtLi[iii] = 1.0; }
            slopeBandsLi[iii] = (double)ii-1.0;
            if (slopeBandsLi[iii] < 0) slopeBandsLi[iii] = 0;
            if (slopeBandsLi[iii] >= Nsrc) slopeBandsLi[iii] = Nsrc;
          }
        }
        idxL = slopeBandsLi[i];
        wghtL = slopeWghtLi[i];
      }
      if (wghtL == 0.0) wghtL = 1.0;
      double idxR = 0.0;
      double wghtR = 0.0;
      if ((nScale < Nsrc) || (frq == NULL)) {
        idxR =  (double)slopesH[i] / F0 ;
        wghtR = idxR - floor(idxR);
      } else {
        if (slopeBandsHi == NULL) { // map the frequency to an fft bin index
          int ii, iii;
          slopeBandsHi = (double*)malloc(sizeof(double)*nSlopes);
          slopeWghtHi = (double*)malloc(sizeof(double)*nSlopes);
          for (iii = 0; iii < nSlopes; iii++) {
            for (ii = 0; ii < nScale; ii++) {
              if (frq[ii] >= (FLOAT_DMEM)slopesH[iii]) break;
            }
            if ((ii<nScale)&&(ii>0)) {
              slopeWghtHi[iii] = ((double)slopesH[iii]-frq[ii-1])/(frq[ii] - frq[ii-1]);
            } else { slopeWghtHi[iii] = 1.0; }
            if ((ii<nScale)&&(frq[ii] == (FLOAT_DMEM)slopesH[iii])) {
              slopeBandsHi[iii] = (double)ii;
            } else {
              slopeBandsHi[iii] = (double)ii - 1.0;
            }
            if (slopeBandsHi[iii] >= Nsrc) slopeBandsHi[iii] = Nsrc - 1;
          }
        }
        idxR = slopeBandsHi[i];
        wghtR = slopeWghtHi[i];
      }
      if (wghtR == 0.0) wghtR = 1.0;
      // TODO: interpolation instead of rounding boundaries to next lower bin and next higher bin
      // TODO: spectral band filter shapes...
      // TODO: debug output of actual filter bandwidth & range based on rounded (or interpolated) boundaries!
  
      long iL = (long)floor(idxL);
      long iR = (long)floor(idxR);
      if (iL >= Nsrc) { iL=iR=Nsrc-1; wghtR=0.0; wghtL=0.0; }
      if (iR >= Nsrc) { iR=Nsrc-1; wghtR = 1.0; } 
      if (iL < 0) iL=0;
      if (iR < 0) iR=0;

      // actual spectral slope computation
      double Sf = 0.0;
      double S2f = 0.0;
      double sumA = 0.0;
      double sumB = 0.0;
      double Nind = idxR - idxL;
      if ((nScale >= Nsrc && nScale > 0) && (frq != NULL)) {
        Sf  = (double)frq[iL] * wghtL;
        S2f = Sf * Sf;
        sumA = (double)frq[iL] * wghtL * (double)srcLP[iL];
        sumB = wghtL * srcLP[iL];
        int ii = iL + 1;
        for ( ; ii < iR && ii < nScale; ii++) {
          S2f += (double)frq[ii] * (double)frq[ii] ;
          Sf += (double)frq[ii];   //NOTE: if this line is commented out: old code for compatibility, dont' check into SVN!!
          sumA += (double)frq[ii] * (double)srcLP[ii];
          sumB += (double)srcLP[ii];
        }
        S2f += (double)frq[iR] * wghtR * (double)frq[iR] * wghtR;
        Sf  += (double)frq[iR] * wghtR;
        sumA += (double)frq[iR] * wghtR * (double)srcLP[iR];
        sumB += wghtR * (double)srcLP[iR];
      } else { 
        Sf   = (double)iL * wghtL;
        S2f  = Sf * Sf;
        sumA = (double)iL * wghtL * (double)srcLP[iL];
        sumB = wghtL * (double)srcLP[iL];
        int ii = iL + 1;
        for ( ; ii < iR && ii < nScale; ii++) {
          S2f  += (double)ii * (double)ii;
          Sf   += (double)ii;
          sumA += (double)ii * (double)srcLP[ii];
          sumB += (double)srcLP[ii];
        }
        S2f += (double)iR * wghtR * (double)iR * wghtR;
        Sf  += (double)iR * wghtR;
        sumA += (double)iR * wghtR * (double)srcLP[iR];
        sumB += wghtR * (double)srcLP[iR];
        S2f *= F0 * F0;
        Sf  *= F0;
        sumA *= F0;
      }
      double deno = (Nind * S2f - Sf * Sf);
      double slope = 0.0;
      if (deno != 0.0) slope = (Nind * sumA - Sf * sumB) / deno;
      // TODO: more options for slope normalisation!
      if (buggySlopeScale) {
        dst[n++] = (FLOAT_DMEM)(slope*(Nind-1.0));
      } else {
        dst[n++] = (FLOAT_DMEM)(slope);
      }
    }
  }

  // alpha ratio  (1-5k energy / 0-1k energy)
  if (alphaRatio) {
    FLOAT_DMEM sum01 = 0.0;
    FLOAT_DMEM sum15 = 0.0;
    if ((nScale < Nsrc) || (frq == NULL)) {
      double f = 0.0;
      for (j = 0; j < Nsrc; j++) {
        if (f > 5000.0) {
          break;
        }
        if (f < 1000.0) {
          sum01 += srcP[j];
        } else {
          sum15 += srcP[j];
        }
        f += F0;
      }
    } else {
      for (j = 0; j < Nsrc; j++) {
        if (frq[j] > 5000.0) {
          break;
        }
        if (frq[j] < 1000.0) {
          sum01 += srcP[j];
        } else {
          sum15 += srcP[j];
        }
      }
    }
    if (sum01 > 0.0) {
      if (useLogSpectrum) {
        if (sum15 > specFloor) {
          dst[n++] = (FLOAT_DMEM)(10.0 * log(sum15 / sum01) / log(10.0));
        } else {
          dst[n++] = (FLOAT_DMEM)(10.0 * (log(specFloor) - log(sum01)) / log(10.0));
        }
      } else {
        dst[n++] = sum15 / sum01;
      }
    } else {
      dst[n++] = 0.0;
    }
  }

  // hammarberg index
  if (hammarbergIndex) {
    FLOAT_DMEM max02 = 0.0;
    FLOAT_DMEM max25 = 0.0;
    if ((nScale < Nsrc) || (frq == NULL)) {
      double f = 0.0;
      for (j = 0; j < Nsrc; j++) {
        if (f > 5000.0) {
          break;
        }
        if (f < 2000.0) {
          if (srcP[j] > max02) {
            max02 = srcP[j];
          }
        } else {
          if (srcP[j] > max25) {
            max25 = srcP[j];
          }
        }
        f += F0;
      }
    } else {
      for (j = 0; j < Nsrc; j++) {
        if (frq[j] > 5000.0) {
          break;
        }
        if (frq[j] < 2000.0) {
          if (srcP[j] > max02) {
            max02 = srcP[j];
          }
        } else {
          if (srcP[j] > max25) {
            max25 = srcP[j];
          }
        }
      }
    }
    if (max25 > 0.0) {
      if (useLogSpectrum) {
        if (max02 > specFloor) {
          dst[n++] = (FLOAT_DMEM)(10.0 * log(max02 / max25) / log(10.0));
        } else {
          dst[n++] = (FLOAT_DMEM)(10.0 * (log(specFloor) - log(max25)) / log(10.0));
        }
      } else {
        dst[n++] = max02 / max25;
      }
    } else {
      dst[n++] = 0.0;
    }
  }

  // sum of all spectral amplitudes/powers/logs, required for spectral centroid/sharpness and moments
  double sumB=0.0, sumC=0.0;
  if (normBandEnergies && !useLogSpectrum) {
    sumB = frameSum;  // frameSum is the power spectrum sum
  } else {
    for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
      sumB += (double)srcLP[j];
    }
  }

  
  // compute rollOff(s):
  FLOAT_DMEM *ro = (FLOAT_DMEM *)calloc(1,sizeof(FLOAT_DMEM)*nRollOff);
  for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
    sumC += (double)srcP[j];
    for (i=0; i<nRollOff; i++) {
      if (buggyRollOff == 1 && i > 0) {
        sumC += (double)srcP[j];
      }
      if ((ro[i] == 0.0) && (sumC >= rollOff[i] * frameSum)) {
        if ((nScale >= Nsrc) && (frq!=NULL)) {
          ro[i] = (FLOAT_DMEM)frq[j];   // TODO: norm frequency ??
        } else {
          ro[i] = (FLOAT_DMEM)j * (FLOAT_DMEM)F0;   // TODO: norm frequency ??
        }
      }
    }
  }
  for (i=0; i<nRollOff; i++) {
    dst[n++] = ro[i];
  }
  free(ro);

  // flux  (requireMagSpec!)
  if (specPosDiff || specDiff || flux || fluxCentroid || fluxAtFluxCentroid) {
    if (prevSpec == NULL) {
      prevSpec = (FLOAT_DMEM**)calloc(1, sizeof(FLOAT_DMEM*) * nFieldsPrevSpec);
    }
    if (nSrcPrevSpec == NULL) {
      nSrcPrevSpec = (long*)calloc(1, sizeof(long) * nFieldsPrevSpec);
    }
    if (prevSpec[idxi] == NULL) {
      nSrcPrevSpec[idxi] = nBins;
      prevSpec[idxi] = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM) * nSrcPrevSpec[idxi]);
      dst[n++] = 0.0;
    } else {
      double normP=1.0, normC=1.0;
      FLOAT_DMEM *magP = prevSpec[idxi];
      double myA = 0.0;
      double myAf = 0.0;
      double d = 0.0;
      double dp = 0.0;
      // simple absolute spectral difference (root of)
      if (specDiff) {
        for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
          double myd = (srcM[j] - magP[j - specRangeLowerBin]);
          d += myd*myd;
        }
        d /= (double)(specRangeUpperBin - specRangeLowerBin + 1);
        if (d > 0.0) {
          dst[n++] = (FLOAT_DMEM)sqrt(d);
        } else {
          dst[n++] = 0.0;
        }
      }
      // positive spectral differences only (root of)
      if (specPosDiff) {
        for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
          double myd = (srcM[j] - magP[j - specRangeLowerBin]);
          if (myd > 0.0) {
            dp += myd*myd;
          }
        }
        dp /= (double)(specRangeUpperBin - specRangeLowerBin + 1);
        if (dp > 0.0) {
          dst[n++] = (FLOAT_DMEM)sqrt(dp);
        } else {
          dst[n++] = 0.0;
        }
      }
      if (flux || fluxCentroid) {
         for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
           double myB = ((double)srcM[j]/normC - (double)magP[j - specRangeLowerBin]/normP);
           myA += myB*myB;
         }
      }
      if (fluxCentroid) {
        if ((nScale>specRangeUpperBin)&&(frq!=NULL)) {
          for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
            double myB = ((double)srcM[j]/normC - (double)magP[j - specRangeLowerBin]/normP);
            myAf += myB*myB * frq[j];
          }
        } else {
          for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
            double myB = ((double)srcM[j]/normC - (double)magP[j - specRangeLowerBin]/normP);
            myAf += myB*myB * (double)j * F0;
          }
        }
      }
      if (flux) {
        double flux = 0.0;
        if (nBins > 0) {
          flux = myA / (double)(nBins);
        }
        if (flux > 0.0) {
          dst[n++] = (FLOAT_DMEM)sqrt(flux);
        } else {
          dst[n++] = 0.0;
        }
      }
      if (fluxCentroid || fluxAtFluxCentroid) {
        double fluxCentr = 0.0;
        if (myA > 0.0) {
          fluxCentr = myAf / myA;
        }
        if (fluxCentroid) {
          dst[n++] = (FLOAT_DMEM)fluxCentr;
        }
        if (fluxAtFluxCentroid) {
          // TODO: map flux centroid back to bin...
          int bin = specRangeUpperBin;
          if ((nScale>specRangeUpperBin)&&(frq!=NULL)) {
            for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
              if (frq[j] >= fluxCentr) {
                bin = j;
                break;
              }
            }
          } else {
            for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
              if ((double)j * F0 >= fluxCentr) {
                bin = j;
                break;
              }
            }
          }
          // range around bin...
          int range = 2;
          double myF = 0.0;
          int start = bin - range;
          int end = bin + range;
          if (start < specRangeLowerBin) {
            start = specRangeLowerBin;
          }
          if (end > specRangeUpperBin) {
            end = specRangeUpperBin;
          }
          for (j = start; j <= end; j++) {
            double myB = ((double)srcM[j]/normC - (double)magP[j - specRangeLowerBin]/normP);
            myF += myB*myB;
          }
          if (end- start + 1 > 0) {
            myF /= (double)(end - start + 1);
          } else {
            myF = 0.0;
          }
          dst[n++] = (FLOAT_DMEM)myF;
        }
      }
    }
    for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
      prevSpec[idxi][j - specRangeLowerBin] = srcM[j];
    }
  }

  // centroid
  FLOAT_DMEM ctr=0.0;
  double sumA = 0.0;
  double sumLog = 0.0;
  double minE = 0.0;
  double f = 0.0;
  if (centroid||standardDeviation||variance||skewness||kurtosis||slope) { // spectral centroid (mpeg7)
    if ((nScale >= Nsrc)&&(frq!=NULL)) {
      minE = (double)srcLP[specRangeLowerBin];
      for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
        sumA += (double)frq[j] * (double)srcLP[j];
        if (useLogSpectrum && (double)srcLP[j] < minE) {
          minE = (double)srcLP[specRangeLowerBin];
        }
      }
      if (useLogSpectrum) {
        for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
          sumLog += (double)frq[j] * minE;
        }
      }
    } else {
      if ((nScale>specRangeUpperBin)&&(frq!=NULL)) {
        minE = (double)srcLP[specRangeLowerBin];
        for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
          sumA += (double)frq[j] * (double)srcLP[j];
          if (useLogSpectrum && (double)srcLP[j] < minE) {
            minE = (double)srcLP[specRangeLowerBin];
          }
        }
        if (useLogSpectrum) {
          for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
            sumLog += (double)frq[j] * minE;
          }
        }
      } else {
        for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
          sumA += f * (double)srcLP[j];
          f += F0;
        }
        if (useLogSpectrum) {
          sumLog = F0 * minE * (double)nBins;
        }
      }
    }
    // TODO: in sumB computation the left and right boundary weights are considered for first and last bin
    //       in sumA computation these are not considered! Need to fix?
    if (sumB != 0.0) {
      ctr = (FLOAT_DMEM)(sumA/sumB);  // spectral centroid in Hz (unnormalised)  // TODO: normFrequency option
    }
    if (centroid) {
      if (useLogSpectrum) {
        // output a warning that the log spectral centroid is not standard and has potentially bogus values...!
        SMILE_IWRN(2, "Log-spectral centroid is non-standard and should not be used as it has unstable and bogus values! Remove the 'useLogSpectrum' option when enabling the centroid feature unless you know what you are doing!");
      }
      dst[n++] = ctr;
    }
  }
  
  if ((maxPos)||(minPos)) {
    long maP=specRangeLowerBin;
    long miP=specRangeLowerBin;
    FLOAT_DMEM max=srcLP[specRangeLowerBin];
    FLOAT_DMEM min=srcLP[specRangeLowerBin];
    for (j = specRangeLowerBin + 1; j < specRangeUpperBin; j++) {
      if (srcLP[j] < min) { min = srcLP[j]; miP = j; }
      if (srcLP[j] > max) { max = srcLP[j]; maP = j; }
    }
    if ((nScale >= Nsrc)&&(frq!=NULL)) {
      if (maxPos) dst[n++] = (FLOAT_DMEM)frq[maP];  // spectral minimum in Hz
      if (minPos) dst[n++] = (FLOAT_DMEM)frq[miP]; // spectral maximum in Hz
    } else {
      if (maxPos) dst[n++] = (FLOAT_DMEM)(maP*F0);  // spectral minimum in Hz
      if (minPos) dst[n++] = (FLOAT_DMEM)(miP*F0); // spectral maximum in Hz
    }
  }

  // spectral entropy
  if (entropy) {
    // TODO: normalise??
    dst[n++] = smileStat_entropy(srcLP + specRangeLowerBin, specRangeUpperBin - specRangeLowerBin + 1);
  }

  // compute various spectral moments
  double u = ctr; // use centroid as mean value
  double m2 = 0.0;
  double m3 = 0.0;
  double m4 = 0.0;

  if (standardDeviation||variance||skewness||kurtosis) {
    if ((nScale>=Nsrc)&&(frq!=NULL)) {
      for (i = specRangeLowerBin; i <= specRangeUpperBin; i++) {
        double t1 = ((double)frq[i]-u);
        double m = t1*t1 * (double)srcLP[i];
        m2 += m;
        m *= t1;
        m3 += m;
        m4 += m*t1;
      }
    } else {
      for (i = specRangeLowerBin; i <= specRangeUpperBin; i++) {
        double fi = (double)i*F0;
        double t1 = (fi-u);
        double m = t1*t1 * (double)srcLP[i];
        m2 += m;
        m *= t1;
        m3 += m;
        m4 += m*t1;
      }
    }

    double sigma2=0.0;
    if (sumB != 0.0) {
      sigma2 = m2/sumB;
    }
    if (standardDeviation) {
      if (sigma2 > 0.0) {
        dst[n++] = (FLOAT_DMEM)(sqrt(sigma2));
      } else {
        dst[n++] = (FLOAT_DMEM)0.0;
      }
    }
    // spectral spread (mpeg7) = spectral variance
    if (variance) { // save variance (sigma^2 = m2) in output vector
      dst[n++] = (FLOAT_DMEM)(sigma2);
    }
    // spectral skewness
    if (skewness) {
      if (sigma2 <= 0.0) {
        dst[n++] = 0.0;
      } else {
        dst[n++] = (FLOAT_DMEM)( m3/(sumB*sigma2*sqrt(sigma2)) );
      }
    }
    // spectral kurtosis
    if (kurtosis) {
      if (sigma2 == 0.0) {
        dst[n++] = 0.0;
      } else {
        dst[n++] = (FLOAT_DMEM)( m4/(sumB*sigma2*sigma2) );
      }
    }
  }

  // spectral slope
  if (slope) {
    double Sf = 0.0;
    double S2f = 0.0;
    double Nind = (double)nBins;
    if ((nScale >= Nsrc && nScale > 0)&&(frq!=NULL)) {
      for (i = specRangeLowerBin; i <= specRangeUpperBin && i < nScale; i++) {
        S2f += (double)frq[i] * (double)frq[i] ;
        Sf += (double)frq[i];   //NOTE: if this line is commented out: old code for compatibility, don't check into SVN!!
      }
    } else {
      double NNm1;
      double S1;
      double S2;
      NNm1 = Nind * (Nind - 1.0);
      S1 = NNm1/(double)2.0;  // sum of all i=0..N-1
      S2 = NNm1*((double)2.0*Nind-(double)1.0)/(double)6.0; // sum of all i^2 for i=0..N-1
      Sf = S1 * F0;
      S2f = S2 * F0*F0;
    }
    double deno = (Nind*S2f-Sf*Sf);
    double slope = 0.0;
    if (deno != 0.0) slope = (Nind*sumA-Sf*sumB)/deno;
    if (buggySlopeScale) {
      dst[n++] = (FLOAT_DMEM)(slope*(Nind-1.0));
    } else {
      dst[n++] = (FLOAT_DMEM)(slope);
    }
  }

  if (sharpness) {
    // TODO: check if we should use src3 when useLogSpectrum is on, or
    //  if its actually correct to take the centroid of the log spectrum
    // TODO: check the same for power spectrum!

    // psychoacoustically correct spectral centroid (= sharpness)
    FLOAT_DMEM ctr = 0.0;
    FLOAT_DMEM sumAA = 0.0;
    if (sharpness) { // spectral sharpness (weighted centroid, usually in bark scale)
      if ((nScale >= Nsrc)&&(frq!=NULL)) {
        // custom scale, frequency info given in meta data
        if (sharpnessWeights == NULL) {
          // compute weights and cache them
          sharpnessWeights = (double*)malloc(sizeof(double)*(specRangeUpperBin - specRangeLowerBin + 1));
          for (j = specRangeLowerBin; j <= specRangeUpperBin && j < nScale; j++) {
            // convert frequency to bark, if not already in bark
            double f = (double)frq[j];
            if (frqScale != SPECTSCALE_BARK) {
              // transform to linear scale...
              f = smileDsp_specScaleTransfInv(f,frqScale,frqScaleParam);
              // ...then from linear to bark
              f = smileDsp_specScaleTransfFwd(f,SPECTSCALE_BARK,0.0);
            }
            sharpnessWeights[j - specRangeLowerBin] = f * smileDsp_getSharpnessWeightG(f, SPECTSCALE_BARK, 0.0);           
          }
        }
        for (j = specRangeLowerBin; j <= specRangeUpperBin && j < nScale; j++) {
          sumAA += (FLOAT_DMEM)(sharpnessWeights[j - specRangeLowerBin] * (double)srcP[j]);
        }
      } else {
        // linear scale, no frequency axis info given in meta data
        if (sharpnessWeights == NULL) {
          // compute weights and cache them
          sharpnessWeights = (double*)malloc(sizeof(double)*(specRangeUpperBin - specRangeLowerBin + 1));
          for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
            double fb = smileDsp_specScaleTransfFwd(f,SPECTSCALE_BARK,0.0);
            sharpnessWeights[j - specRangeLowerBin] = fb * smileDsp_getSharpnessWeightG(fb, SPECTSCALE_BARK, 0.0);
            f += F0;
          }
        } 
        for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
          sumAA += (FLOAT_DMEM)(sharpnessWeights[j - specRangeLowerBin] * (double)srcP[j]);
        }
      }
      if (frameSum != 0.0) {
        ctr = (FLOAT_DMEM)(sumAA/frameSum);  // weighted spectral centroid in Bark (unnormalised)
      }
      dst[n++] = (FLOAT_DMEM)(0.11 * ctr); /* scaling for psychoacoustic sharpness */
    }
   }

   if (tonality) {
     dst[n++] = 0.0; /* NOT YET IMPLEMENTED */
   }

   if (harmonicity) {
     FLOAT_DMEM ptpSum=0.0; long nPtp=0;
     FLOAT_DMEM lastPeak = -99.0;
     for (j = specRangeLowerBin + 2; j < specRangeUpperBin - 1; j++) {
       // min/max finder
       if ( (srcLP[j-2] < srcLP[j] && srcLP[j-1] < srcLP[j] && srcLP[j] > srcLP[j+1] && srcLP[j] > srcLP[j+2])  //max
           || (srcLP[j-2] > srcLP[j] && srcLP[j-1] > srcLP[j] && srcLP[j] < srcLP[j+1] && srcLP[j] < srcLP[j+2]) ) // min
       {
         if (lastPeak != -99.0) {
           ptpSum += fabs(srcLP[j] - lastPeak);
           nPtp++;
         }
         lastPeak = srcLP[j];
       }
     }
     ptpSum /= 2.0;   // we only want to consider each peak once
     // normalise to spectral mean energy (~ frame energy, normalized by bin size):
     // NEW: this is linked to normBandEnergies.. only if this is set, then we will norm
     if (normBandEnergies && sumB != 0.0) {
       // NEW: we don't divide by number of bins and peaks anymore -> a single peak will have less harmonicity than a series of peaks
       if (useLogSpectrum) {
         ptpSum /= (FLOAT_DMEM)fabs(sumB);
       } else {
         ptpSum /= (FLOAT_DMEM)(frameSum);
       }
     } else {
       ptpSum /= (FLOAT_DMEM)nBins;
     }
     dst[n++] = ptpSum;
   }

   if (flatness) {
     FLOAT_DMEM sf = 0.0;
     FLOAT_DMEM gmean = 0.0;
     int nGm = 0;
     if (sumB != 0.0) {
       // compute geometric mean
       for (j = specRangeLowerBin; j <= specRangeUpperBin; j++) {
         if (srcLP[j] != 0.0) {
           gmean += log(fabs(srcLP[j]));
           nGm++;
         }
       }
       if (nGm > 0) {
         gmean /= (FLOAT_DMEM)nGm;
       }
       gmean = exp(gmean);
       // compute flatness
       sf = gmean / (FLOAT_DMEM)fabs(sumB/(double)nBins);
     }
     if (logFlatness) {
       if (sf > 0.0) {
         dst[n++] = (FLOAT_DMEM)log(sf);
       } else {
         dst[n++] = 0.0;
       }
     } else {
       dst[n++] = sf;
     }
   }

   if (srcP != NULL && srcP != src) {
     free((FLOAT_DMEM*)srcP);
   }
   if (srcM != NULL && srcM != src) {
     free((FLOAT_DMEM*)srcM);
   }
   if (srcL != NULL && srcL != src) {
     free((FLOAT_DMEM*)srcL);
   }
   return 1;
}

cSpectral::~cSpectral()
{
  // TODO: manage extra bandsL/H slopes, rolloff, etc. for each input field!!
  if (bandsL!=NULL) free(bandsL);
  if (bandsH!=NULL) free(bandsH);
  if (slopesL!=NULL) free(slopesL);
  if (slopesH!=NULL) free(slopesH);
  if (bandsLi!=NULL) free(bandsLi);
  if (bandsHi!=NULL) free(bandsHi);
  if (wghtLi!=NULL) free(wghtLi);
  if (wghtHi!=NULL) free(wghtHi);
  if (slopeBandsLi!=NULL) free(slopeBandsLi);
  if (slopeBandsHi!=NULL) free(slopeBandsHi);
  if (slopeWghtLi!=NULL) free(slopeWghtLi);
  if (slopeWghtHi!=NULL) free(slopeWghtHi);
  if (rollOff!=NULL) free(rollOff);
  if (prevSpec != NULL) {
    for (int i = 0; i < nFieldsPrevSpec; i++) {
      if (prevSpec[i] != NULL) {
        free(prevSpec[i]);
      }
    }
    free(prevSpec);
  }
  if (nSrcPrevSpec != NULL) {
    free(nSrcPrevSpec);
  }
  if (sharpnessWeights != NULL) free(sharpnessWeights);
}

