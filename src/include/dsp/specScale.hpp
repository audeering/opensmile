/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

*/


#ifndef __CSPECSCALE_HPP
#define __CSPECSCALE_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>
#include <memory>

#define BUILD_COMPONENT_SpecScale
#define COMPONENT_DESCRIPTION_CSPECSCALE "This component performs linear/non-linear axis scaling of FFT magnitude spectra with spline interpolation."
#define COMPONENT_NAME_CSPECSCALE "cSpecScale"

/*
#define SPECTSCALE_LINEAR   0
#define SPECTSCALE_LOG      1
#define SPECTSCALE_BARK     2
#define SPECTSCALE_MEL      3
#define SPECTSCALE_SEMITONE 4
#define SPECTSCALE_BARK_SCHROED     5
#define SPECTSCALE_BARK_SPEEX       6
*/

#define INTERP_NONE       0
#define INTERP_LINEAR     1
#define INTERP_SPLINE     2

class cSpecScale : public cVectorProcessor {
  private:
    int scale; // target scale
    int sourceScale;
    int interpMethod;
    int specSmooth, specEnhance;
    int auditoryWeighting;
    double logScaleBase, logSourceScaleBase;
    double minF, maxF, fmin_t, fmax_t;
    long nPointsTarget;
    double firstNote, param;

    long nMag, magStart;
    double fsSec;
    double deltaF, deltaF_t;

    double *f_t;
    double *spline_work;
    double *y, *y2;
    double *audw;

    // caches for optimized spline interpolation functions
    std::unique_ptr<sSmileMath_splineCache> splineCache;
    std::unique_ptr<sSmileMath_splintCache> splintCache;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
	  virtual int setupNewNames(long nEl) override;
    virtual int dataProcessorCustomFinalise() override;
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cSpecScale(const char *_name);
    
    virtual ~cSpecScale();
};

#endif // __CSPECSCALE_HPP
