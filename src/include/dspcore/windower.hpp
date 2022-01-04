/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

data windower.
takes frames from one level, applies window function, and saves to other level

*/


#ifndef __WINDOWER_HPP
#define __WINDOWER_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CWINDOWER "This component applies applies window function to pcm frames."
#define COMPONENT_NAME_CWINDOWER "cWindower"


struct sWindowerConfigParsed
{
  int winFunc;
  int squareRoot;
  double offset, gain;
  double sigma, alpha, alpha0, alpha1, alpha2, alpha3;
  double * win;
  long frameSizeFrames;
};


// WINF_XXXXXX constants are defined in smileUtil.hpp !
class cWindower : public cVectorProcessor {
  private:
    double offset, gain;
    double sigma, alpha, alpha0, alpha1, alpha2, alpha3;
    long  frameSizeFrames;
    int   winFunc;    // winFunc as numeric constant (see #defines above)
    int   squareRoot;
    double xscale;
    double xshift;
    double fade;
    const char *saveWindowToFile;
    double *win;
    
    void precomputeWinFunc();
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    virtual int myFinaliseInstance() override;

    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cWindower(const char *_name);

    // return the parsed windower config. If this function is called after the component was finalised, the *win variable will point to the precomputed window.
    struct sWindowerConfigParsed * getWindowerConfigParsed();

    virtual ~cWindower();
};




#endif // __WINDOWER_HPP
