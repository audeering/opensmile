/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

computes spectral features such as flux, roll-off, centroid, etc.

*/


#ifndef __CSPECTRAL_HPP
#define __CSPECTRAL_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CSPECTRAL "This component computes spectral features such as flux, roll-off, centroid, and user defined band energies (rectangular summation of FFT magnitudes), etc."
#define COMPONENT_NAME_CSPECTRAL "cSpectral"

class cSpectral : public cVectorProcessor {
  private:
    int frqScale;
    int normBandEnergies;
    double frqScaleParam;

    int squareInput, centroid, nBands, nSlopes, nRollOff, entropy;
    int specDiff, specPosDiff;
    int flux, fluxCentroid, fluxAtFluxCentroid;
    int standardDeviation, variance, skewness, kurtosis, slope;
    int sharpness, tonality, harmonicity, flatness;
    int alphaRatio, hammarbergIndex;

    long specRangeLower, specRangeUpper, specRangeLowerBin, specRangeUpperBin;
    long nScale;
    int maxPos, minPos;
    long *bandsL, *bandsH; // frequencies
    long *slopesL, *slopesH; // frequencies    
    double *bandsLi, *bandsHi; // indicies (fft bins), real values
    double *wghtLi, *wghtHi; // indicies (fft bins), real values
    double *slopeBandsLi, *slopeBandsHi; // indicies (fft bins), real values
    double *slopeWghtLi, *slopeWghtHi; // indicies (fft bins), real values
    double *rollOff;
    double fsSec;
    const double *frq;
    int buggyRollOff, buggySlopeScale;
    int useLogSpectrum;
    int logFlatness;
    FLOAT_DMEM specFloor, logSpecFloor;
    bool requireMagSpec, requirePowerSpec, requireLogSpec;

    FLOAT_DMEM **prevSpec;
    long nFieldsPrevSpec;
    long *nSrcPrevSpec;

    double *sharpnessWeights;

    void setRequireLorPspec();
    
    int isBandValid(long start, long end)
    {
      if ((start>=0)&&(end>0)) return 1;
      else return 0;
    }
    
    void parseRange(const char *val, long *lowerHz, long *upperHz);
    int parseBandsConfig(const char * field, long ** bLow, long ** bHigh);
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;

    virtual int setupNamesForField(int i, const char*name, long nEl) override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;


  public:
    SMILECOMPONENT_STATIC_DECL
    
    cSpectral(const char *_name);

    virtual ~cSpectral();
};




#endif // __CSPECTRAL_HPP
