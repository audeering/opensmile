/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

computes mean of full input ?

*/


#ifndef __CFULLINPUTMEAN_HPP
#define __CFULLINPUTMEAN_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CFULLINPUTMEAN "This component performs mean normalizing on a data series. A 2-pass analysis of the data is performed, which makes this component unusable for on-line analysis. In the first pass, no output is produced and the mean value (over time) is computed for each input element. In the second pass the mean vector is subtracted from all input frames, and the result is written to the output dataMemory level. Attention: Due to the 2-pass processing the input level must be large enough to hold the whole data sequence."
#define COMPONENT_NAME_CFULLINPUTMEAN "cFullinputMean"

enum cFullinputMean_meanType {
  MEANTYPE_AMEAN = 0,
  MEANTYPE_RQMEAN = 1,
  MEANTYPE_ABSMEAN = 2,
  MEANTYPE_ENORM = 3   // htk compatible energy normalisation
};

class cFullinputMean : public cDataProcessor {
  private:
    bool mvn_;
    bool exclude_zeros_;
    int print_variances_;
    int print_means_;
    int first_frame_;
    long reader_pointer_;
    long reader_pointer2_;
    long reader_pointer3_;
    int multi_loop_mode_;
    enum cFullinputMean_meanType mean_type_;
    int spec_enorm_;
    int symm_subtract_;
    int symm_subtract_clip_to_zero_;
    int flag_; //, htkcompatible; long idx0;
    int eoi_flag_;  // this is set after an EOI condition
    cVector *means_;
    cVector *means2_;
    cVector *variances_;
    cVector *variances2_;
    long * n_means_arr_;
    long * n_means2_arr_;
    long * n_variances_arr_;
    long * n_variances2_arr_;
    long n_means_;
    long n_means2_;
    long n_variances_;
    long n_variances2_;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    eTickResult readNewData();
    void meanSubtract(cVector *vec);
    eTickResult doMeanSubtract();
    int doVarianceComputation();
    int finaliseMeans();
    int finaliseVariances();

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    //virtual int configureWriter(sDmLevelConfig &c) override;

    //virtual void configureField(int idxi, long __N, long nOut) override;
    //virtual int setupNamesForField(int i, const char*name, long nEl) override;
    //virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFullinputMean(const char *_name);

    virtual ~cFullinputMean();
};




#endif // __CFULLINPUTMEAN_HPP
