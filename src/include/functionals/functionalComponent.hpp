/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

a single statistical functional or the like....
(template class)

*/


#ifndef __CFUNCTIONAL_HPP
#define __CFUNCTIONAL_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <core/dataWriter.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALCOMPONENT "This is an abstract functional class, it is the base for all functional extractor classes."
#define COMPONENT_NAME_CFUNCTIONALCOMPONENT "cFunctionalComponent"

#define TIMENORM_UNDEFINED   -1
#define TIMENORM_SEGMENT   0
#define TIMENORM_SECOND    1
#define TIMENORM_SECONDS   1
#define TIMENORM_FRAME     2
#define TIMENORM_FRAMES    2
#define TIMENORM_SAMPLE    3  // currently not implemented
#define TIMENORM_SAMPLES   3  // currently not implemented
#define _TIMENORM_MAX      3

// obsolete: please use TIMENORM_XXX constants in new code..!
#define NORM_TURN     0
#define NORM_SECOND   1
#define NORM_FRAME    2
#define NORM_SAMPLES  3


class cFunctionalComponent : public cSmileComponent {
  private:
    double T;
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    int nEnab, nTotal_;
    int *enab;
    int timeNorm, timeNormIsSet;
    const char **functNames;
    
    virtual void myFetchConfig() override;
    // get size of one input element/frame in seconds
    double getInputPeriod() {
      return T;
    }
    // parse a custom 'norm' (timeNorm) option if set (call this from myFetchConfig)
    void parseTimeNormOption();

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFunctionalComponent(const char *name, int nTotal=0, const char *names[]=NULL);
    void setInputPeriod(double _T) { T=_T; }

    void setTimeNorm(int _norm) { 
      if (!timeNormIsSet) {
        if ((_norm >= 0)&&(_norm <= _TIMENORM_MAX)&&(_norm!=TIMENORM_UNDEFINED)) {
          timeNorm = _norm;
        }
      }
    }

    // inputs: sorted and unsorted array of values, out: pointer to space in output array,
    // You may not return MORE than Nout elements, please return as return value the number
    // of actually computed elements (usually Nout)
    virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout);
    virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM min,
        FLOAT_DMEM max, FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout)
    {
      return process(in,inSorted,out,Nin,Nout);
    }

    virtual void setFieldMetaData(cDataWriter *writer, const FrameMetaInfo *fmeta, int idxi, long nEl);
    virtual long getNoutputValues() { return nEnab; }
    virtual long getNumberOfElements(long j) { return 1; }
    virtual const char* getValueName(long i);
    virtual int getRequireSorted() { return 0; }

    virtual ~cFunctionalComponent();
};




#endif // __CFUNCTIONAL_HPP
