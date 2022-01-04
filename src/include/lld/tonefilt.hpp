/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

on-line semi-tone filter bank

*/


#ifndef __CTONEFILT_HPP
#define __CTONEFILT_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>

#define BUILD_COMPONENT_Tonefilt
#define COMPONENT_DESCRIPTION_CTONEFILT "This component implements an on-line, sample by sample semi-tone filter bank which can be used as first step for the computation of CHROMA features as a replacement of cTonespec. The filter is based on correlating with a sine wave of the exact target frequency of a semi-tone for each note in the filter-bank."
#define COMPONENT_NAME_CTONEFILT "cTonefilt"

class cTonefilt : public cDataProcessor {
  private:
    double outputPeriod; /* in seconds */
    long  /*outputBuffersize,*/ outputPeriodFrames; /* in frames */
    int nNotes;
    double firstNote;
    long N, Nf;
    double inputPeriod;
    double decayFN, decayF0;
    
    cVector *tmpVec;
    FLOAT_DMEM *tmpFrame;

    double **corrS, **corrC;
    double *decayF;
    double *freq;
    long * pos;

    void doFilter(int i, cMatrix *row, FLOAT_DMEM*x);
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNewNames(long nEl) override;
      
  public:
    SMILECOMPONENT_STATIC_DECL
    
    cTonefilt(const char *_name);

    virtual ~cTonefilt();
};




#endif // __CTONEFILT_HPP
