/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#include <lld/harmonics.hpp>
#include <math.h>

#define MODULE "cHarmonics"

// TODO: harmonics energy ratios (magnitudes squared)

SMILECOMPONENT_STATICS(cHarmonics)

SMILECOMPONENT_REGCOMP(cHarmonics)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CHARMONICS;
  sdescription = COMPONENT_DESCRIPTION_CHARMONICS;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    // overwrite some defaults from cVectorProcessor
    ct->setField("processArrayFields", NULL, 0);
    ct->setField("includeSingleElementFields", NULL, 1);
    // define our own new options:
    ct->setField("f0ElementName", "Name of F0 element in input vector to use.", "F0final");
    ct->setField("f0ElementNameIsFull", "1/0= f0ElementName is a partial name (glob with *x*) or the exact full name.", 1);
    ct->setField("magSpecFieldName", "Name of magnitude spectrum field in input vector to use.", "pcm_fftMag");
    ct->setField("magSpecFieldNameIsFull", "1/0= magSpecFieldName is a partial name (glob with *x*) or the exact full name.", 0);
    ct->setField("formantFrequencyFieldName", "Name of formant frequency field. Typcially formantFreqLpc", (const char*)NULL);
    ct->setField("formantFrequencyFieldNameIsFull", "1/0= formantFrequencyFieldName is a partial name (glob with *x*) or the exact full name.", 1);
    ct->setField("formantBandwidthFieldName", "Name of formant bandwidth field. Typically formantBandwidthLpc", (const char*)NULL);
    ct->setField("formantBandwidthFieldNameIsFull", "1/0= formantBandwidthFieldName is a partial name (glob with *x*) or the exact full name.", 1);

    ct->setField("nHarmonics", "Maximum number of harmonics to search for (including F0) (approximately Fmax / F_lowest_f0).", 100);
    ct->setField("firstHarmonicMagnitude", "Index of first harmonic magnitude to output (0 is magnitude of F0).", 1);
    ct->setField("nHarmonicMagnitudes", "Number of harmonic magnitudes to output. 0 to disable.", 0);
    ct->setField("outputLogRelMagnitudes", "1 = output logarithmic magnitudes (dB) normalised by the magnitude of F0 (0dB), if nHarmonicMagnitudes > 0.", 1);
    ct->setField("outputLinearMagnitudes", "1 = output the linear magnitudes as obtained from the FFT for the nHarmonicMagnitudes harmonics (if nHarmonicMagnitudes > 0).", 0);
    ct->setField("harmonicDifferences", "Array that specifies harmonic differences (ratios in linear scale) to compute. Syntax for one element: H1-H2 (ratio of H1 to H2), H0 is fundamental frequency. A1,A2,...,AN is the amplitude (highest peak in the formant range) of the N-th formant, if formant frequency AND bandwidth information is given in the input (see formantFrequencyFieldName and formantBandwidthFieldName options).", (const char*)NULL, ARRAY_TYPE);
    ct->setField("harmonicDifferencesLog", "1 = Harmonic differences in log magnitude scale (actually differences of the log values then).", 1);
    ct->setField("harmonicDifferencesRatioLinear", "1 = Harmonic differences in linear magnitude scale (actually ratios of the linear values then).", 0);
    ct->setField("formantAmplitudes", "1 = Enable output of formant amplitudes (amplitude of highest peak in the formant range, half bandwidth left and right of formant frequency).", 0);
    ct->setField("formantAmplitudesLinear", "1 = Linear formant amplitude output, requires formantAmplitudes=1 .", 0);
    ct->setField("formantAmplitudesLogRel", "1 = Logarithmic relative to F0 formant amplitude output in dB, requires formantAmplitudes=1 .", 1);
    ct->setField("formantAmplitudesStart", "First formant to compute amplitudes for, 0 is F0, 1 is first formant, etc.", 1);
    ct->setField("formantAmplitudesEnd", "Last formant to compute amplitudes for. Default '-1' is last formant found in the input.", -1);

    ct->setField("computeAcfHnrLogdB", "1 = enable HNR (logarithmic in dB) from ACF at F0 position (vs. total energy). Will be zero for unvoiced frames (where F0 = 0).", 0);
    ct->setField("computeAcfHnrLinear", "1 = enable HNR (linear ACF amplitude ratio) from ACF at F0 position (vs. total energy).  Will be zero for unvoiced frames (where F0 = 0).", 0);
    ct->setField("logRelValueFloorUnvoiced", "Sets the value that is returned for LogRel (relative to F0) type features when F0==0 (unvoiced). Logical default is the general floor of -201.0, however if unvoiced regions should always be zero, in order to be discarded/ignored e.g. by a functionals component, then this should be set to 0.0", -201.0);
    // TODO: acf refined F0
    //       harmonic flux
    //       fft refined formants (and optionally bandwidths for ease of use)

    /*    ct->setField("harmonicsSlope", "    ", 0);
    ct->setField("harmonicsMeanMagnitude", "", 0);
    ct->setField("harmonicsStddevMagnitude", "", 0);*/
    // harmonics flux: deviation of harmonics from expected positions  (could be due to poor spectral res also.. so use a min threshold and subtract that (0.5 - 1 bin width)
  )

  SMILECOMPONENT_MAKEINFO(cHarmonics);
}

SMILECOMPONENT_CREATE(cHarmonics)


cHarmonics::cHarmonics(const char *_name) :
  cVectorProcessor(_name),
  f0ElementName_(NULL), magSpecFieldName_(NULL), idxF0(-1), fsSec(-1.0),
  frq_(NULL), nFrq_(0), harmonics_(NULL),
  w_(NULL), ip_(NULL), acfdata_(NULL), acf_(NULL), cnt_(0),
  harmonicDifferences_(NULL), haveFormantDifference_(false),
  logRelValueFloorUnvoiced_(-201.0),
  formantAmplitudes_(0)
{}

sHarmonicDifferences * cHarmonics::parseHarmonicDifferences(int * Ndiff, bool *haveFormant, int * maxHarmonic)
{
  int N = getArraySize("harmonicDifferences");
  if (N > 0) {
    int err = 0;
    sHarmonicDifferences *hd = (sHarmonicDifferences *)calloc(1, sizeof(sHarmonicDifferences) * N);
    for (int i = 0; i < N; i++) {
      const char *tmp = getStr_f(myvprint("harmonicDifferences[%i]", i));
      if (tmp != NULL) {
        char *h1 = strdup(tmp);
        char *h2 = strchr(h1, '-');
        if (h2 != NULL && h2 > h1) {
          *h2 = 0;
          h2++;
          if (h1[0] == 'H') {
            char *ep = NULL;
            int r = strtol(h1 + 1, &ep, 10);
            if ((r == 0) && (ep == h1 + 1)) {
              SMILE_IERR(1, "Error parsing %i. harmonic difference (part 1): %s", i, h1);
              err = 1;
            } else {
              hd[i].h1formant = -1;
              hd[i].h1idx = r;
              if (maxHarmonic != NULL && *maxHarmonic < r) {
                *maxHarmonic = r;
              }
            }
          } else if (h1[0] == 'A') {
            if (haveFormant != NULL) {
              *haveFormant = true;
            }
            char *ep = NULL;
            int r = strtol(h1 + 1, &ep, 10);
            if ((r == 0) && (ep == h1 + 1)) {
              SMILE_IERR(1, "Error parsing %i. harmonic difference (part 1): %s", i, h1);
              err = 1;
            } else {
              hd[i].h1formant = r;
              hd[i].h1idx = -1;
            }
          } else {
            SMILE_IERR(1, "Invalid identifier in %i. harmonic difference (part 1): %c. Allowed are 'A' for Formant, and 'H' for harmonic.", i, h1[0]);
            err = 1;
          }
          if (h2[0] == 'H') {
             char *ep = NULL;
             int r = strtol(h2 + 1, &ep, 10);
             if ((r == 0) && (ep == h2 + 1)) {
               SMILE_IERR(1, "Error parsing %i. harmonic difference (part 2): %s", i, h2);
               err = 1;
             } else {
               hd[i].h2formant = -1;
               hd[i].h2idx = r;
               if (maxHarmonic != NULL && *maxHarmonic < r) {
                 *maxHarmonic = r;
               }
             }
           } else if (h2[0] == 'A') {
             if (haveFormant != NULL) {
               *haveFormant = true;
             }
             char *ep = NULL;
             int r = strtol(h2 + 1, &ep, 10);
             if ((r == 0) && (ep == h2 + 1)) {
               SMILE_IERR(1, "Error parsing %i. harmonic difference (part 2): %s", i, h2);
               err = 1;
             } else {
               hd[i].h2formant = r;
               hd[i].h2idx = -1;
             }
           } else {
             SMILE_IERR(1, "Invalid identifier in %i. harmonic difference (part 2): %c. Allowed are 'A' for Formant, and 'H' for harmonic.", i, h2[0]);
             err = 1;
           }
        } else {
          SMILE_IERR(1, "Invalid range specified for %i. harmonic difference: '%s'", i, h1);
          err = 1;
        }
        if (err == 0) {
          hd[i].text = tmp;
        }
        free(h1);
      }
    }
    if (err == 1) {
      free(hd);
      return NULL;
    } else {
      if (Ndiff != NULL) {
        *Ndiff = N;
      }
      return hd;
    }
  } else {
    if (Ndiff != NULL) {
      *Ndiff = 0;
    }
  }
  return NULL;
}

void cHarmonics::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();
  nHarmonics_ = getInt("nHarmonics");
  nHarmonicMagnitudes_ = getInt("nHarmonicMagnitudes");
  firstHarmonicMagnitude_ = getInt("firstHarmonicMagnitude");
  outputLogRelMagnitudes_ = getInt("outputLogRelMagnitudes");
  outputLinearMagnitudes_ = getInt("outputLinearMagnitudes");
  int maxHarmonic = 0;
  harmonicDifferences_ = parseHarmonicDifferences(&nHarmonicDifferences_,
      &haveFormantDifference_, &maxHarmonic);
  harmonicDifferencesLog_ = getInt("harmonicDifferencesLog");
  harmonicDifferencesLinear_ = getInt("harmonicDifferencesRatioLinear");
  formantAmplitudes_ = getInt("formantAmplitudes");
  formantAmplitudesLogRel_ = getInt("formantAmplitudesLogRel");
  formantAmplitudesLinear_ = getInt("formantAmplitudesLinear");
  formantAmplitudesStart_ = getInt("formantAmplitudesStart");
  formantAmplitudesEnd_ = getInt("formantAmplitudesEnd");
  f0ElementName_ = getStr("f0ElementName");
  f0ElementNameIsFull_ = getInt("f0ElementNameIsFull");
  magSpecFieldName_ = getStr("magSpecFieldName");
  magSpecFieldNameIsFull_ = getInt("magSpecFieldNameIsFull");
  formantFrequencyFieldName_ = getStr("formantFrequencyFieldName");
  formantFrequencyFieldNameIsFull_ = getInt("formantFrequencyFieldNameIsFull");
  formantBandwidthFieldName_ = getStr("formantBandwidthFieldName");
  formantBandwidthFieldNameIsFull_ = getInt("formantBandwidthFieldNameIsFull");
  computeAcfHnrLogdB_ = getInt("computeAcfHnrLogdB");
  computeAcfHnrLinear_ = getInt("computeAcfHnrLinear");
  if (nHarmonics_ < nHarmonicMagnitudes_ + firstHarmonicMagnitude_ + 1) {
    nHarmonics_ = nHarmonicMagnitudes_ + firstHarmonicMagnitude_ + 1;
  }
  if (nHarmonics_ < maxHarmonic + 1) {
    nHarmonics_ = maxHarmonic + 1;
  }
  if (formantAmplitudesLogRel_ == 0 && formantAmplitudesLinear_ == 0) {
    formantAmplitudes_ = 0;
    SMILE_IWRN(1, "Disabling formant amplitude output, because neither formantAmplitudesLogRel nor formantAmplitudesLinear is enabled.");
  }
  logRelValueFloorUnvoiced_ = (FLOAT_DMEM)getDouble("logRelValueFloorUnvoiced");
}


int cHarmonics::setupNewNames(long nEl)
{
  namesAreSet_ = 1;
  int newNEl = 0;
  if (fsSec == -1.0) {
    const sDmLevelConfig *c = reader_->getLevelConfig();
    fsSec = c->frameSizeSec;  // WHAT about this value if we use pitch from 60ms win and spectrum from 25ms win???
    // We should rather use the meta data info on the spectral field...! However this will not be passed properly.
  }
  // setup output fields
  if (computeAcfHnrLogdB_) {
    writer_->addField("HarmonicsToNoiseRatioACFLogdB", 1);
    newNEl++;
  }
  if (computeAcfHnrLinear_) {
    writer_->addField("HarmonicsToNoiseRatioACFLinear", 1);
    newNEl++;
  }
  if (outputLogRelMagnitudes_) {
    if (nHarmonicMagnitudes_ > 0) {
      writer_->addField("HarmonicMagnitudeRelativeF0dB", nHarmonicMagnitudes_, firstHarmonicMagnitude_);
      newNEl += nHarmonicMagnitudes_;
    }
  }
  if (outputLinearMagnitudes_) {
    if (nHarmonicMagnitudes_ > 0) {
      writer_->addField("HarmonicMagnitudeLinear", nHarmonicMagnitudes_, firstHarmonicMagnitude_);
      newNEl += nHarmonicMagnitudes_;
    }
  }
  // find input fields
  if (f0ElementName_ == NULL) {
    SMILE_IERR(1, "F0ElementName not specified! This is required! Aborting.");
    return 0;
  }
  if (magSpecFieldName_ == NULL) {
    SMILE_IERR(1, "Magnitude spectrum field name not specified! This is required! Aborting.");
    return 0;
  }
  idxF0 = findElement(f0ElementName_, f0ElementNameIsFull_, NULL, NULL, NULL);
  idxSpec = findField(magSpecFieldName_, magSpecFieldNameIsFull_, &nSpecBins, NULL, -1, NULL, &idxSpecField_);
  if (idxF0 == idxSpec) {
    SMILE_IERR(1, "Index found for f0Element and magSpecField are the same (%i)! There is an error somewhere, correct your config! Maybe one of the two fields (or both) does not exist in the input level!?", idxF0);
    return 0;
  }
  nFFreq_ = 0;
  nFBandw_ = 0;
  idxFFreq_ = -1;
  idxFBandw_ = -1;
  if (formantFrequencyFieldName_ != NULL) {
    if (formantBandwidthFieldName_ == NULL) {
      if (formantAmplitudes_) {
        SMILE_IERR(2, "Formant bandwidth field name is missing. Both frequency and bandwidth of formants are required! Disabling formant based harmonics features!!!");
        if (haveFormantDifference_) {
          SMILE_IWRN(2, "Also disabling harmonic difference features, because they contain some formant amplitude differences!");
          nHarmonicDifferences_ = 0;
        }
        formantAmplitudes_ = 0;
      }
      formantAmplitudes_ = 0;
    }
    idxFFreq_ = findField(formantFrequencyFieldName_, formantFrequencyFieldNameIsFull_, &nFFreq_, NULL, -1, NULL);
    idxFBandw_ = findField(formantBandwidthFieldName_, formantBandwidthFieldNameIsFull_, &nFBandw_, NULL, -1, NULL);
    // some sanity checks...
    if (idxFFreq_ == idxFBandw_) {
      SMILE_IERR(1, "Index for formant frequency field and formant bandwidth field are the same!");
      return 0;
    }
    if (nFFreq_ != nFBandw_) {
      SMILE_IERR(1, "Number of formant frequencies and bandwidth does not match! (%i != %i)", nFFreq_, nFBandw_);
      return 0;
    }
  } else {
    if (formantAmplitudes_) {
      SMILE_IERR(2, "Formant amplitudes cannot be computed, formantFrequencyFieldName_ and formantBandwidthFieldName_ are not set! Disabling formant amplitudes.");
      if (haveFormantDifference_) {
        SMILE_IWRN(2, "Also disabling harmonic difference features, because they contain some formant amplitude differences!");
        nHarmonicDifferences_ = 0;
      }
      formantAmplitudes_ = 0;
    }
  }
  // add harmonic difference field
  if (nHarmonicDifferences_ > 0 && (harmonicDifferencesLog_ || harmonicDifferencesLinear_)) {
    for (int i = 0; i < nHarmonicDifferences_; i++) {
      if (harmonicDifferences_[i].text != NULL) {
        char *tmp = strdup(harmonicDifferences_[i].text);
        // remove spaces before and after
        char *x = tmp;
        int l = strlen(x);
        while (*x == ' ' && l > 0) {
          x++; l--;
        }
        while (*(x+l) == ' ' && l > 0) {
          *(x+l) = 0; l--;
        }
        if (harmonicDifferencesLinear_) {
          char *y = myvprint("HarmonicDifferenceRatioLinear%s", x);
          writer_->addField(y, 1);
          newNEl++;
          free(y);
        }
        if (harmonicDifferencesLog_) {
          char *y = myvprint("HarmonicDifferenceLogRel%s", x);
          writer_->addField(y, 1);
          newNEl++;
          free(y);
        }
        free(x);
      } else {
        SMILE_IERR(1, "Inconsistency in harmonic differences! Element %i has NULL text. Seems like a bug somewhere. Aborting config.", i);
        return 0;
      }
    }
  }
  // add formant amplitudes field
  if (formantAmplitudes_) {
    if (formantAmplitudesEnd_ == -1) {
      formantAmplitudesEnd_ = nFFreq_;
    }
    if (formantAmplitudesStart_ < 0) {
      formantAmplitudesStart_ = 0;
    }
    if (formantAmplitudesEnd_ > nFFreq_) {
      formantAmplitudesEnd_ = nFFreq_;
    }
    if (formantAmplitudesEnd_ < formantAmplitudesStart_) {
      SMILE_IERR(1, "Inconsistency in the formant amplitudes range configuration found (end (%i) < start (%i)). Disabling formant amplitude output.", formantAmplitudesEnd_, formantAmplitudesStart_);
      formantAmplitudes_ = 0;
    } else {
      if (formantAmplitudesLinear_) {
        writer_->addField("FormantAmplitudeByMaxHarmonicLinear", formantAmplitudesEnd_ - formantAmplitudesStart_ + 1, formantAmplitudesStart_);
        newNEl += formantAmplitudesEnd_ - formantAmplitudesStart_ + 1;
      }
      if (formantAmplitudesLogRel_) {
        writer_->addField("FormantAmplitudeByMaxHarmonicLogRelF0", formantAmplitudesEnd_ - formantAmplitudesStart_ + 1, formantAmplitudesStart_);
        newNEl += formantAmplitudesEnd_ - formantAmplitudesStart_ + 1;
      }
    }
  }
  return newNEl;
}

int cHarmonics::isPeak(const FLOAT_DMEM * x, long N, long n)
{
  if (n >= N || n < 0) { return 0; }
  if (n + 1 < N) {
    if (n > 0) {
      if (x[n] > x[n - 1] && x[n] > x[n + 1]) {
        return 1;
      }
    } else {
      if (x[0] > x[1]) { 
        return 1; 
      }
    }
  } else {
    if (n > 0) {
      if (x[n] > x[n - 1]) {
        return 1;
      }
    }
  }
  return 0;
}

// this method only works for linear frequency scales!
int cHarmonics::freqToAcfBinLin(float freq)
{
  double fs = frq_[nFrq_-1] * 2.0;
  if (freq > 0.0) {
    return (int)floor(fs / freq);
  } else {
    return 0;
  }
}

int cHarmonics::freqToBin(float freq, int startBin)
{
  for (; startBin < nFrq_; startBin++) {
    if (frq_[startBin] > freq) {
      if (frq_[startBin] - freq > freq - frq_[startBin - 1]) {
        return startBin - 1;
      } else {
        return startBin;
      }
    }
  }
  return 0;
}

int cHarmonics::findHarmonicPeaks(float pitchFreq,
    const FLOAT_DMEM * magSpec, long nBins, sF0Harmonic * harmonics,
    int nHarmonics, const double * frq, float F0)
{
  if (nHarmonics <= 0) {
    return 0;
  }
  if (harmonics == NULL) {
    //harmonics = (sF0Harmonic *)calloc(1, sizeof(sF0Harmonic));
    return 0;
  }
  int nHarmonicsFound = 0;
  if (frq_ == NULL) {  // linear fft spectrum
    // loop over candidates and find closest peak
    int firstBin = (int)floor((float)(1.5) * pitchFreq / F0);
    for (int i = 0; i < nHarmonics; i++) {
      // map pitchFreq to bins
      int candBin = (int)floor((float)(i + 2) * pitchFreq / F0);
      int peakBin = -1;
      if (isPeak(magSpec, nBins, candBin)) {
        peakBin = candBin;
      } else {
        int candBinL = candBin - 1;
        int candBinR = candBin + 1;
        int lowerLimit = (int)floor(((float)i + 1.5) * pitchFreq / F0);
        int upperLimit = (int)floor(((float)i + 2.5) * pitchFreq / F0);
        // search left and right
        while (candBinL > lowerLimit && candBinR < upperLimit && peakBin == -1) {
          if (isPeak(magSpec, nBins, candBinL)) {
            peakBin = candBinL; break;
          }
          if (isPeak(magSpec, nBins, candBinR)) {
            peakBin = candBinR; break;
          }
          candBinR++;
          candBinL--;
        }
      }
      // found a valid peak for harmonic i?
      if (peakBin >= firstBin && peakBin < nBins - 1) {
        harmonics[i].bin = peakBin;
        harmonics[i].freqExpected = (float)(i + 2) * pitchFreq;
        harmonics[i].freqFromBin = (float)peakBin * F0;
        harmonics[i].magnitude = magSpec[peakBin];
        double magnitudeInterpolated = 0.0;
        float binInterpolated = (float)smileMath_quadFrom3pts(
            (double)(peakBin - 1), (double)magSpec[peakBin - 1],
            (double)peakBin, (double)magSpec[peakBin],
            (double)(peakBin + 1), (double)magSpec[peakBin + 1],
            &magnitudeInterpolated, NULL);
        harmonics[i].freqInterpolated = binInterpolated * F0;
        harmonics[i].magnitudeInterpolated = (float)magnitudeInterpolated;
        nHarmonicsFound++;
      } else {
        harmonics[i].bin = -1;
        harmonics[i].freqExpected = 0.0;
        harmonics[i].freqFromBin = 0.0;
        harmonics[i].magnitude = 0.0;
        harmonics[i].magnitudeInterpolated = 0.0;
      }
    }
  } else {  // possibly something else, nonlinear...
    // loop over candidates and find closest peak
    int lastBin = freqToBin(0.5f * pitchFreq, 1);
    int firstBin = freqToBin(0.5f * pitchFreq, lastBin);
    for (int i = 0; i < nHarmonics; i++) {
      // map pitchFreq to bins
      int candBin = freqToBin((float)(i + 1) * pitchFreq, lastBin);
      int peakBin = -1;
      if (candBin >= nBins) {
        harmonics[i].freqExpected = 0.0;
        harmonics[i].magnitudeLogRelF0 = -201.0;
        harmonics[i].bin = -1;
        harmonics[i].freqFromBin = 0.0;
        harmonics[i].freqInterpolated = 0.0;
        harmonics[i].magnitude = 0.0;
        harmonics[i].magnitudeInterpolated = 0.0;
        continue;
      }
      if (isPeak(magSpec, nBins, candBin)) {
        peakBin = candBin;
      } else {
        int candBinL = candBin - 1;
        int candBinR = candBin + 1;
        int lowerLimit = freqToBin(((float)i + 0.5f) * pitchFreq, lastBin);
        int upperLimit = freqToBin(((float)i + 1.5f) * pitchFreq, candBin);
        // search left and right
        while ((candBinL >= lowerLimit || candBinR <= upperLimit) && peakBin == -1) {
          if (candBinR <= upperLimit) {
            if (isPeak(magSpec, nBins, candBinR)) {
              peakBin = candBinR;
              break;
            }
            candBinR++;
          }
          if (candBinL >= lowerLimit) {
            if (isPeak(magSpec, nBins, candBinL)) {
              peakBin = candBinL;
              break;
            }
            candBinL--;
          }
        }
        // TODO: remove duplicate harmonics?
      }
      harmonics[i].freqExpected = (float)(i + 1) * pitchFreq;
      harmonics[i].magnitudeLogRelF0 = -201.0;
      // found a valid peak for harmonic i?
      if (peakBin >= firstBin && peakBin < nBins - 1) {
        harmonics[i].bin = peakBin;
        harmonics[i].freqFromBin = (float)frq_[peakBin];
        harmonics[i].magnitude = magSpec[peakBin];
        double magnitudeInterpolated = 0.0;
        harmonics[i].freqInterpolated = (float)smileMath_quadFrom3pts(
            frq_[peakBin - 1], (double)magSpec[peakBin - 1],
            frq_[peakBin], (double)magSpec[peakBin],
            frq_[peakBin + 1], (double)magSpec[peakBin + 1],
            &magnitudeInterpolated, NULL);
        harmonics[i].magnitudeInterpolated = (float)magnitudeInterpolated;
        nHarmonicsFound++;
      } else {
        harmonics[i].bin = candBin;
        harmonics[i].freqFromBin = 0.0;
        harmonics[i].freqInterpolated = 0.0;
        harmonics[i].magnitude = 0.0;
        harmonics[i].magnitudeInterpolated = 0.0;
      }
      lastBin = candBin;
    }
  }
  return nHarmonicsFound;
}

void cHarmonics::postProcessHarmonics(sF0Harmonic * harmonics, int nHarmonics, bool logRelMagnitude)
{
  float magnitudeF0 = 0.0;
  if (logRelMagnitude) {
    magnitudeF0 = harmonics[0].magnitude;
    if (magnitudeF0 == 0.0) {
      logRelMagnitude = false;
    } else {
      magnitudeF0 = log10(magnitudeF0);
    }
    harmonics[0].magnitudeLogRelF0 = 0.0;
  }
  for (int i = 1; i < nHarmonics; i++) {
    if (logRelMagnitude) {
      double tmp = 0.0;
      if (harmonics[i].magnitudeInterpolated > 0.0) {
        tmp = log10(harmonics[i].magnitudeInterpolated);
        harmonics[i].magnitudeLogRelF0 = (float)(20.0 * (tmp - magnitudeF0));
        if (harmonics[i].magnitudeLogRelF0 < -200.0) {
          harmonics[i].magnitudeLogRelF0 = -200.0;
        }
      } else {
        harmonics[i].magnitudeLogRelF0 = -200.0;
      }
    } else {
      harmonics[i].magnitudeLogRelF0 = -201.0;
    }
    // remove (set to zero) duplicates:
    if (harmonics[i].bin == harmonics[i-1].bin) {
      harmonics[i].bin = 0;
      harmonics[i].freqFromBin = 0.0;
      harmonics[i].freqInterpolated = 0.0;
      harmonics[i].freqExpected = 0.0;
      harmonics[i].magnitude = 0.0;
      harmonics[i].magnitudeInterpolated = 0.0;
      harmonics[i].magnitudeLogRelF0 = -201.0;
    }
  }
}

int cHarmonics::computeAcf(const FLOAT_DMEM *magSpec, FLOAT_DMEM *acf, long nBins, bool squareInput)
{
  long N = (nBins-1)*2;
  // check for power of 2!!
  if (!smileMath_isPowerOf2(N)) {
    SMILE_IERR(1,"(Nsrc-1)*2 = %i is not a power of 2, this is required for acf!! make sure the input data really is fft magnitude data!",N);
    return 0;
  }
  // data preparation for inverse fft:
  if (acfdata_ == NULL) {
    acfdata_ = (FLOAT_TYPE_FFT*)malloc(sizeof(FLOAT_TYPE_FFT)*N);
  }
  if (ip_ == NULL) {
    ip_ = (int *)calloc(1,sizeof(int)*(N+2));
  }
  if (w_ == NULL) {
    w_ = (FLOAT_TYPE_FFT *)calloc(1,sizeof(FLOAT_TYPE_FFT)*(N*5)/4+2);
  }
  if (squareInput) {
    acfdata_[0] = (FLOAT_TYPE_FFT)(magSpec[0] * magSpec[0]);
    acfdata_[1] = (FLOAT_TYPE_FFT)(magSpec[nBins-1] * magSpec[nBins-1]);
    for (int i = 2; i < N-1; i += 2) {
      acfdata_[i] = (FLOAT_TYPE_FFT)(magSpec[i>>1] * magSpec[i>>1]);
      acfdata_[i+1] = 0.0;
    }
  } else {
    acfdata_[0] = (FLOAT_TYPE_FFT)(magSpec[0]);
    acfdata_[1] = (FLOAT_TYPE_FFT)(magSpec[nBins-1]);
    for (int i = 2; i < N-1; i += 2) {
      acfdata_[i] = (FLOAT_TYPE_FFT)(magSpec[i>>1]);
      acfdata_[i+1] = 0.0;
    }
  }
  // inverse fft
  rdft(N, -1, acfdata_, ip_, w_);
  // data output
  for (int i = 0; (i < N) && (i < nBins); i++) {
    acf[i] = (FLOAT_DMEM)fabs(acfdata_[i]) / (FLOAT_DMEM)nBins;
  }
  return 1;
}

long cHarmonics::getClosestPeak(const FLOAT_DMEM *x, long N, long idx)
{
  if (isPeak(x, N, idx)) {
    return idx;
  }
  long o = 1;
  while (idx - o > 0 || idx + o < N - 1) {
    if (idx - o > 0) {
      if (isPeak(x, N, idx - o)) {
        return idx - o;
      }
    }
    if (idx + o < N - 1) {
      if (isPeak(x, N, idx + o)) {
        return idx + o;
      }
    }
    o++;
  }
  if (x[0] > x[idx] && x[N-1] <= x[idx]) {
    return 0;
  } else if (x[0] <= x[idx] && x[N-1] > x[idx]) {
    return N-1;
  } else if (x[0] > x[idx] && x[N-1] > x[idx]) {
    if (idx < N/2) {
      return 0;
    } else {
      return N-1;
    }
  }
  // if all values are the same, we return idx,
  // otherwise we should already have returned an index above
  return idx;
}

/* computes HNR from ACF in dB for a range -100 to +100 dB */
FLOAT_DMEM cHarmonics::computeAcfHnr_linear(const FLOAT_DMEM *a, long N, long F0bin)
{
  if (F0bin <= 0) {
    // find first peak of ACF?
    return 0.0;
  } else {

    double hnr = a[0] - a[F0bin];
    if (hnr == 0.0) {
      hnr = 10e10;
    } else {
      hnr = a[F0bin] / hnr;
    }
    if (hnr > 10e10) {
      hnr = 10e10;
    } else if (hnr < 10e-10) {
      hnr = 10e-10;
    }
    return (FLOAT_DMEM)hnr;
  }
}

FLOAT_DMEM cHarmonics::computeAcfHnr_dB(const FLOAT_DMEM *a, long N, long F0bin)
{
  if (F0bin <= 0) {
    // find first peak of ACF?
    return 0.0;
  } else {
    double hnr = a[0] - a[F0bin];
    double ret;
    if (hnr == 0.0) {
      hnr = 10e10;
    } else {
      hnr = a[F0bin] / hnr;
    }
    if (hnr > 10e10) {
      ret = 10.0 * log10(10e10);
    } else if (hnr < 10e-10) {
      ret = 10.0 * log10(10e-10);
    } else {
      ret = 10.0 * log10(hnr);
    }
    return (FLOAT_DMEM)ret;
  }
}

int * cHarmonics::getFormantAmplitudeIndices(const FLOAT_DMEM * centreFreq,
    const FLOAT_DMEM *bandw, int N)
{
  int * ampl = NULL;
  if (N > 0) {
    ampl = (int *)calloc(1, sizeof(int) * N);
  }
  for (int f = 0; f < N; f++) {
    // TODO: we don't seem to require the bandwidths... remove them or make an option to use them optionally!
    //float fLeft = (float)centreFreq[f] - 0.38f * (float)bandw[f];
    //float fRight = (float)centreFreq[f] + 0.38f * (float)bandw[f];
    float fLeft = 0.8f * (float)centreFreq[f];
    float fRight = 1.2f * (float)centreFreq[f];
    int maxIdx = -1;
    float maxMag = 0.0;
    for (int h = 0; h < nHarmonics_; h++) {
      if (harmonics_[h].freqInterpolated >= fLeft && harmonics_[h].freqInterpolated <= fRight) {
        if (harmonics_[h].magnitude > maxMag) {
          maxIdx = h;
          maxMag = harmonics_[h].magnitude;
        }
      }
    }
    ampl[f] = maxIdx;
  }
  return ampl;
}

// a derived class should override this method, in order to implement the actual processing
int cHarmonics::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  //printf("Nsrc = %i, Ndst = %i\n", Nsrc, Ndst);
  if (idxi > 0) {
    SMILE_IERR(1, "Field index > 0 in processVector. This should not happen, please use processArrayFields = 1!");
    return 0;
  }
  if (Nsrc <= 0) {
    return 0;
  }
  if (frq_ == NULL) { // input block's frequency axis
    const FrameMetaInfo * fmeta = reader_->getFrameMetaInfo();
    if ((fmeta != NULL)&&(idxSpecField_ < fmeta->N)) {
      // TODO: check input block's type (in fmeta)
      // fmeta->field[idxi].dataType & SPECTRAL == 1 ?
      frq_ = (double *)(fmeta->field[idxSpecField_].info);
      nFrq_ = fmeta->field[idxSpecField_].infoSize / sizeof(double);
      if (nFrq_ != nSpecBins) {
        SMILE_IWRN(2,"number of frequency axis points (from info struct) [%i] is not equal to Nsrc [%i] ! Field index: %i (check the processArrayFields option).",nFrq_, nSpecBins, idxi);
        nFrq_ = MIN(nFrq_, nSpecBins);
      }
      // TODO: spectrum scale is stored in level meta-data!
      // should not be, should be in field meta data! or datatype...?
      //reader_->getLevelMetaDataPtr();
      //fmeta->field[idxSpec].dataType
      // FOR NOW: assumes linear scale...
    } else {
      if (fmeta != NULL) {
        SMILE_IERR(1, "Unable to get frequency axis meta information from input level spectral magnitude field (field # %i of %i).", idxSpecField_, fmeta->N);
      } else {
        SMILE_IERR(1, "No frame meta information available from input level. This is required for spectral axis information!");
      }
      return 0;
    }
  }
  //printf("nFrq %i , idxi %i, Nsrc %i, Ndst %i, idxF0 %i, idxSpec %i \n", nFrq_, idxi, Nsrc, Ndst, idxF0, idxSpec);

  long dstPtr = 0;

  // get F0 input field....
  FLOAT_DMEM F0 = src[idxF0];
  // convert F0 (frequency) to T0 (acf bin) and FF0 (fft bin)
  long F0bin = freqToBin(F0, 1);
  long F0acfBin = freqToAcfBinLin(F0);
  // get magnitude spectrum pointer
  const FLOAT_DMEM *magSpec = src + idxSpec;

  if (computeAcfHnrLogdB_ || computeAcfHnrLinear_) {
    // do acf
    if (acf_ == NULL) {
      acf_ = (FLOAT_DMEM *)malloc(sizeof(FLOAT_DMEM) * nFrq_);
    }
    if (computeAcf(magSpec, acf_, nFrq_, 1 /* squareInput = true */)) {
      // compute ACF HNR
      long F0acfBinRefined = 0;
      if (F0acfBin > 0) {
        F0acfBinRefined = getClosestPeak(acf_, nFrq_, F0acfBin);
      }
      if (computeAcfHnrLogdB_) {
        dst[dstPtr++] = computeAcfHnr_dB(acf_, nFrq_, F0acfBinRefined);
      }
      if (computeAcfHnrLinear_) {
        dst[dstPtr++] = computeAcfHnr_linear(acf_, nFrq_, F0acfBinRefined);
      }
    } else {
      return 0;
    }
  }

  bool needHarmonics =
      ((nHarmonicMagnitudes_ > 0 && (outputLogRelMagnitudes_ || outputLinearMagnitudes_))
          || (nHarmonicDifferences_ > 0 && (harmonicDifferencesLog_ || harmonicDifferencesLinear_))
          || formantAmplitudes_);
  if (needHarmonics) {
    // F0 based harmonic features
    if (F0 > 0.0) {
      if (harmonics_ == NULL) {
        harmonics_ = (sF0Harmonic *)calloc(1, sizeof(sF0Harmonic) * nHarmonics_);
      }
      int nH = findHarmonicPeaks(F0, magSpec, nFrq_, harmonics_, nHarmonics_, frq_, 0.0);
      postProcessHarmonics(harmonics_, nHarmonics_, true);
      //printf("n harmonics found (frame %i): %i of %i\n", cnt_, nH,  nHarmonics_);
      //for (int i = 0; i < nHarmonics_; i++) {
//        printf("[%.1f %i %.1f; %.1f]  ", harmonics_[i].freqExpected, harmonics_[i].bin, harmonics_[i].freqInterpolated, harmonics_[i].magnitudeLogRelF0);
  //    }
      //printf("\n");
      if (outputLogRelMagnitudes_) {
        for (int i = firstHarmonicMagnitude_; i < nHarmonicMagnitudes_; i++) {
          dst[dstPtr++] = harmonics_[i].magnitudeLogRelF0;
        }
      }
      if (outputLinearMagnitudes_) {
        for (int i = firstHarmonicMagnitude_; i < nHarmonicMagnitudes_; i++) {
          dst[dstPtr++] = harmonics_[i].magnitude;
        }
      }
      int * faidxs = NULL;
      if (haveFormantDifference_ || formantAmplitudes_) {
        // get formant ranges
        const FLOAT_DMEM *Ff = src + idxFFreq_;
        const FLOAT_DMEM *Fb = src + idxFBandw_;
        // get formant amplitudes
        faidxs = getFormantAmplitudeIndices(Ff, Fb, nFFreq_);
        /*for (int i = 0; i < nFFreq_; i++) {
          printf("cnt[%i]: faidxs[%i] = %i\n", cnt_, i, faidxs[i]);
        }*/
        if (haveFormantDifference_ && nHarmonicDifferences_ > 0
            && (harmonicDifferencesLog_ || harmonicDifferencesLinear_)) {
          // get corresponding harmonic indices
          for (int i = 0; i < nHarmonicDifferences_; i++) {
            if (harmonicDifferences_[i].h1formant > 0) {
              if (harmonicDifferences_[i].h1formant <= nFFreq_) {
                harmonicDifferences_[i].h1idx = faidxs[harmonicDifferences_[i].h1formant - 1];
              } else {
                SMILE_IWRN(1, "Formant index %i in difference %s is higher than number of input formants (%i)!",
                    harmonicDifferences_[i].h1formant, harmonicDifferences_[i].text, nFFreq_);
              }
            } else if (harmonicDifferences_[i].h1formant == 0) {
              harmonicDifferences_[i].h1idx = 0;
            }
            if (harmonicDifferences_[i].h2formant > 0) {
              if (harmonicDifferences_[i].h2formant <= nFFreq_) {
                harmonicDifferences_[i].h2idx = faidxs[harmonicDifferences_[i].h2formant - 1];
              } else {
                SMILE_IWRN(1, "Formant index %i in difference %s is higher than number of input formants (%i)!",
                    harmonicDifferences_[i].h2formant, harmonicDifferences_[i].text, nFFreq_);
              }
            } else if (harmonicDifferences_[i].h2formant == 0) {
              harmonicDifferences_[i].h2idx = 0;
            }
          }
        }
      }
      if (nHarmonicDifferences_ > 0 && (harmonicDifferencesLog_ || harmonicDifferencesLinear_)) {
        for (int i = 0; i < nHarmonicDifferences_; i++) {
          if (harmonicDifferences_[i].h1idx >= 0 && harmonicDifferences_[i].h2idx >= 0
              && harmonicDifferences_[i].h1idx < nHarmonics_
              && harmonicDifferences_[i].h2idx < nHarmonics_) {
            // both valid
            if (harmonicDifferencesLinear_) {
              if (harmonics_[harmonicDifferences_[i].h2idx].magnitude > 10e-10) {
                dst[dstPtr++] = harmonics_[harmonicDifferences_[i].h1idx].magnitude
                                 / harmonics_[harmonicDifferences_[i].h2idx].magnitude;
              } else {
                dst[dstPtr++] = (FLOAT_DMEM)(harmonics_[harmonicDifferences_[i].h1idx].magnitude
                                 / 10e-10f);
              }
            }
            if (harmonicDifferencesLog_) {
              dst[dstPtr] = harmonics_[harmonicDifferences_[i].h1idx].magnitudeLogRelF0
                              - harmonics_[harmonicDifferences_[i].h2idx].magnitudeLogRelF0;
              if (dst[dstPtr] < -201.0) {
                dst[dstPtr] = -201.0;
              }
              if (dst[dstPtr] > 201.0) {
                dst[dstPtr] = 201.0;
              }
              dstPtr++;
            }
          } else {
            if (harmonicDifferences_[i].h1idx >= 0 && harmonicDifferences_[i].h1idx < nHarmonics_) {
              // h1 valid, assign h1
              if (harmonicDifferencesLinear_) {
                dst[dstPtr++] = (FLOAT_DMEM)(harmonics_[harmonicDifferences_[i].h1idx].magnitudeLogRelF0 / 10e-10f);
              }
              if (harmonicDifferencesLog_) {
                dst[dstPtr] = (FLOAT_DMEM)(harmonics_[harmonicDifferences_[i].h1idx].magnitudeLogRelF0 - 201.0f);
                if (dst[dstPtr] < (FLOAT_DMEM)-201.0) {
                  dst[dstPtr] = (FLOAT_DMEM)-201.0;
                }
                if (dst[dstPtr] > (FLOAT_DMEM)201.0) {
                  dst[dstPtr] = (FLOAT_DMEM)201.0;
                }
                dstPtr++;
              }
              if (harmonicDifferences_[i].h2idx >= 0) {
                SMILE_IWRN(2, "%i. harmonic index %i (b) is out of range (check nHarmonics (=%i)!)",
                    i, harmonicDifferences_[i].h2idx, nHarmonics_);
              }
            } else if (harmonicDifferences_[i].h2idx >= 0 && harmonicDifferences_[i].h2idx < nHarmonics_) {
              // h2 valid, assign h2
              if (harmonicDifferencesLinear_) {
                dst[dstPtr++] = 0.0;
              }
              if (harmonicDifferencesLog_) {
                dst[dstPtr] = (FLOAT_DMEM)(-201.0 - harmonics_[harmonicDifferences_[i].h2idx].magnitudeLogRelF0);
                if (dst[dstPtr] < (FLOAT_DMEM)-201.0) {
                  dst[dstPtr] = (FLOAT_DMEM)-201.0;
                }
                if (dst[dstPtr] > (FLOAT_DMEM)201.0) {
                  dst[dstPtr] = (FLOAT_DMEM)201.0;
                }
                dstPtr++;
              }
              if (harmonicDifferences_[i].h1idx >= 0) {
                SMILE_IWRN(2, "%i. harmonic index %i (a) is out of range (check nHarmonics (=%i)!)",
                    i, harmonicDifferences_[i].h1idx, nHarmonics_);
              }
            } else {
              // none valid
              if (harmonicDifferencesLinear_) {
                dst[dstPtr++] = (FLOAT_DMEM)1.0;
              }
              if (harmonicDifferencesLog_) {
                dst[dstPtr++] = (FLOAT_DMEM)0.0;
              }
              SMILE_IWRN(3, "%i. harmonic index %i (a) is out of range (check nHarmonics (=%i)!)",
                  i, harmonicDifferences_[i].h1idx, nHarmonics_);
              SMILE_IWRN(3, "%i. harmonic index %i (b) is out of range (check nHarmonics (=%i)!)",
                  i, harmonicDifferences_[i].h2idx, nHarmonics_);
            }
          }
        }
      }
      if (formantAmplitudes_) {
        for (int i = formantAmplitudesStart_; i <= formantAmplitudesEnd_; i++) {
          if (i > 0 && i <= nFFreq_ && faidxs != NULL) {
            if (formantAmplitudesLinear_) {
              if (faidxs[i - 1] >= 0) {
                dst[dstPtr++] = harmonics_[faidxs[i - 1]].magnitude;
              } else {
                dst[dstPtr++] = (FLOAT_DMEM)0.0;
              }
            }
            if (formantAmplitudesLogRel_) {
              if (faidxs[i - 1] >= 0) {
                dst[dstPtr++] = harmonics_[faidxs[i - 1]].magnitudeLogRelF0;
              } else {
                dst[dstPtr++] = (FLOAT_DMEM)0.0; // -201.0;
              }
            }
          } else if (i == 0) {
            if (formantAmplitudesLinear_) {
              dst[dstPtr++] = harmonics_[0].magnitude;
            }
            if (formantAmplitudesLogRel_) {
              dst[dstPtr++] = harmonics_[0].magnitudeLogRelF0;
            }
          } else {
            if (formantAmplitudesLinear_) {
              dst[dstPtr++] = (FLOAT_DMEM)0.0;
            }
            if (formantAmplitudesLogRel_) {
              dst[dstPtr++] = (FLOAT_DMEM)-201.0;
            }
          }
        }
      }
      if (faidxs != NULL) {
        free(faidxs);
      }
    } else {
      if (outputLogRelMagnitudes_) {
        for (int i = 0; i < nHarmonicMagnitudes_; i++) {
          dst[dstPtr++] = logRelValueFloorUnvoiced_;
        }
      }
      if (outputLinearMagnitudes_) {
        for (int i = 0; i < nHarmonicMagnitudes_; i++) {
          dst[dstPtr++] = (FLOAT_DMEM)0.0;
        }
      }
      if (nHarmonicDifferences_ > 0 && (harmonicDifferencesLog_ || harmonicDifferencesLinear_)) {
        for (int i = 0; i < nHarmonicDifferences_; i++) {
          if (harmonicDifferencesLinear_) {
            dst[dstPtr++] = (FLOAT_DMEM)1.0;
          }
          if (harmonicDifferencesLog_) {
            dst[dstPtr++] = (FLOAT_DMEM)0.0;
          }
        }
      }
      if (formantAmplitudes_) {
        if (formantAmplitudesLinear_) {
          for (int i = formantAmplitudesStart_; i <= formantAmplitudesEnd_; i++) {
            dst[dstPtr++] = (FLOAT_DMEM)0.0;
          }
        }
        if (formantAmplitudesLogRel_) {
          for (int i = formantAmplitudesStart_; i <= formantAmplitudesEnd_; i++) {
            dst[dstPtr++] = logRelValueFloorUnvoiced_;
          }
        }
      }
    }
  }
  cnt_++;
  return 1;
}

/*
 * F0 based harmonic detection
 *
 * ACF HNRs
 *
 * Get formant region peaks and formant amplitudes (add extra format frequency fields)
 *
 * Harmonic features (computed from harmonic peaks)
 *
 * General spectral peaks (non-harmonic)
 *
 * Features from these...
 *
 */

cHarmonics::~cHarmonics()
{
  if (harmonics_ != NULL) {
    free(harmonics_);
  }
  if (w_ != NULL) {
    free(w_);
  }
  if (ip_ != NULL) {
    free(ip_);
  }
  if (acfdata_ != NULL) {
    free(acfdata_);
  }
  if (acf_ != NULL) {
    free(acf_);
  }
  if (harmonicDifferences_ != NULL) {
    free(harmonicDifferences_);
  }
}

