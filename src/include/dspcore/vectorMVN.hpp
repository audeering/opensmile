/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

This component extends the base class cVectorTransform and implements mean/variance normalisation

*/


#ifndef __CVECTORMVN_HPP
#define __CVECTORMVN_HPP

#include <core/smileCommon.hpp>
#include <core/vectorTransform.hpp>
#include <math.h>

#define COMPONENT_DESCRIPTION_CVECTORMVN "This component extends the base class cVectorTransform and implements mean/variance normalisation. You can use this component to perform on-line cepstral mean normalisation. See cFullinputMean for off-line cepstral mean normalisation."
#define COMPONENT_NAME_CVECTORMVN "cVectorMVN"

/* we define some transform type IDs, other will be defined in child classes */
//#define TRFTYPE_CMN     10    /* mean normalisation, mean vector only */
//#define TRFTYPE_MVN     20    /* mean variance normalisation, mean vector + stddev vector */
//#define TRFTYPE_UNDEF    0    /* undefined, or custom type */

#define STDDEV_FLOOR  0.0000001


class cVectorMVN : public cVectorTransform {
  private:
    FLOAT_DMEM specFloor;
    int spectralFlooring, subtractMeans;
    int meanEnable;
    int stdEnable, normEnable;
    int minMaxNormEnable;
    int htkLogEnorm;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
      
    /**** refactoring hooks ****/
    /* allocate the memory for vectors and userData, initialise header
       the only field that has been set is head.vecSize ! 
       This function is only called in modes analysis and incremental if no data was loaded */
    virtual void allocTransformData(struct sTfData *tf, int Ndst, int idxi) override;
    /* For mode == ANALYSIS or TRANSFORMATION, this functions allows for a final processing step
       at the end of input and just before the transformation data is saved to file */
    //virtual void computeFinalTransformData(struct sTfData *tf, int idxi) {}

    /* this will be called BEFORE the transform will be reset to initial values (at turn beginning/end) 
       you may modify the initial values here or the new values, 
       if you return 1 then no further changes to tf will be done,
       if you return 0 then tf0 will be copied to tf after running this function */
    //virtual int transformResetNotify(struct sTfData *tf, struct sTfData *tf0) { return 0; }

    /* Do the actual transformation (do NOT use this to calculate parameters!) 
       This function will only be called if not in ANALYSIS mode 
       Please return the number of output samples (0, if you haven't produced output) */
    virtual int transformData(const struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

    /* Update transform parameters incrementally
       This function will only be called if not in TRANSFORMATIONs mode 
       *buf is a pointer to a buffer if updateMethod is fixedBuffer */
    // return value: 0: no update was performed , 1: successful update
    virtual int updateTransformExp(struct sTfData * tf, const FLOAT_DMEM *src, int idxi) override;
    virtual int updateTransformBuf(struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *buf, long Nbuf, long wrPtr, int idxi) override;
    virtual int updateTransformAvg(struct sTfData * tf, const FLOAT_DMEM *src, int idxi) override;
    virtual int updateTransformAvgI(struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *buf, long * bufferNframes, long Nbuf, long wrPtr, int idxi) override;

    /* generic method, default version will select one of the above methods,
       overwrite to implement your own update strategy ('usr' option) */
    //virtual int updateTransform(struct sTfData * tf, const FLOAT_DMEM *src, FLOAT_DMEM *buf, long * bufferNframes, long Nbuf, int idxi) override;

/////////////////////////////////////////////
    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    //virtual eTickResult myTick(long long t) override;

    //virtual int configureWriter(sDmLevelConfig &c) override;

    //virtual int processComponentMessage(cComponentMessage *_msg) override;
    //virtual int setupNamesForField(int i, const char*name, long nEl) override;
    //virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;
    //virtual int flushVector(FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cVectorMVN(const char *_name);

    virtual ~cVectorMVN();
};




#endif // __CVECTORMVN_HPP
