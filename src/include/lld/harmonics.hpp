/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

statistics of F0 harmonics

*/


#ifndef __CHARMONICS_HPP
#define __CHARMONICS_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define BUILD_COMPONENT_Harmonics
#define COMPONENT_DESCRIPTION_CHARMONICS "This component computes statistics of F0 harmonics. It requires an F0 (Hertz) input field and a linear frequency axis magnitude spectrum as input."
#define COMPONENT_NAME_CHARMONICS "cHarmonics"

typedef struct {
  int bin;
  int belowThreshold;   // 1 = non harmonic, because amplitude falls below threshold
  float freqExpected;
  float freqFromBin;
  float freqInterpolated;
  float magnitude;
  float magnitudeInterpolated;
  float magnitudeLogRelF0;  // magnitude relative to F0 magnitude, in dB
} sF0Harmonic;

typedef struct {
  int h1idx;   // index1 - index2
  int h1formant;  // -1 : h1idx is harmonic, >= 0, formant index for h1
  int h2idx;
  int h2formant;
  const char *text;  // name of difference pair from config
} sHarmonicDifferences;

/*
 * Outputs:
 * hnr log
 * hnr linear
 * nHarmonicMagnitudes_ x logrel mags
 * nHarmonicMagnitudes_ x linear mags
 * nHarmonicDifferences_ x (linear, logrel)
 * formantAmplitudesEnd_ - formantAmplitudesStart_ + 1
 */
class cHarmonics : public cVectorProcessor {
  private:
    int nHarmonics_;
    int nHarmonicMagnitudes_;
    int firstHarmonicMagnitude_;
    int outputLogRelMagnitudes_;
    int outputLinearMagnitudes_;
    int nHarmonicDifferences_;
    sHarmonicDifferences * harmonicDifferences_;
    int harmonicDifferencesLog_;
    int harmonicDifferencesLinear_;
    int formantAmplitudes_;
    int formantAmplitudesLinear_;
    int formantAmplitudesLogRel_;
    int formantAmplitudesStart_;
    int formantAmplitudesEnd_;
    int computeAcfHnrLinear_;
    int computeAcfHnrLogdB_;
    const char * formantFrequencyFieldName_;
    const char * formantBandwidthFieldName_;
    const char * f0ElementName_;
    const char * magSpecFieldName_;
    int f0ElementNameIsFull_;
    int magSpecFieldNameIsFull_;
    int formantFrequencyFieldNameIsFull_;
    int formantBandwidthFieldNameIsFull_;

    FLOAT_DMEM logRelValueFloorUnvoiced_;
    sF0Harmonic *harmonics_;
    FLOAT_TYPE_FFT *w_;
    int * ip_;
    FLOAT_TYPE_FFT *acfdata_;
    FLOAT_DMEM *acf_;

    bool haveFormantDifference_;
    int cnt_;
    double *frq_;
    long nFrq_;
    double fsSec;
    long idxF0;
    long idxSpec;
    int idxSpecField_;
    long nSpecBins;
    long idxFFreq_;
    long nFFreq_;
    long idxFBandw_;
    long nFBandw_;

    sHarmonicDifferences * parseHarmonicDifferences(int * Ndiff, bool *haveFormant, int *maxHarmonic);
    int isPeak(const FLOAT_DMEM * x, long N, long n);
    int findHarmonicPeaks(float pitchFreq, const FLOAT_DMEM * magSpec, long nBins,
        sF0Harmonic * harmonics, int nHarmonics, const double * frq, float F0);
    int freqToBin(float freq, int startBin);
    int freqToAcfBinLin(float freq);
    int computeAcf(const FLOAT_DMEM *magSpec, FLOAT_DMEM *acf, long nBins, bool squareInput);
    FLOAT_DMEM computeAcfHnr_linear(const FLOAT_DMEM *a, long N, long F0bin = -1);
    FLOAT_DMEM computeAcfHnr_dB(const FLOAT_DMEM *a, long N, long F0bin = -1);
    long getClosestPeak(const FLOAT_DMEM *x, long N, long idx);
    void postProcessHarmonics(sF0Harmonic * harmonics, int nHarmonics, bool logRelMagnitude = true);
    int * getFormantAmplitudeIndices(const FLOAT_DMEM * centreFreq, const FLOAT_DMEM *bandw, int N);

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    virtual int setupNewNames(long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    cHarmonics(const char *_name);
    virtual ~cHarmonics();
};

#endif // __CHARMONICS_HPP

