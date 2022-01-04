/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: linear and quadratic regression coefficients

*/


#ifndef __CFUNCTIONALREGRESSION_HPP
#define __CFUNCTIONALREGRESSION_HPP

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <functionals/functionalComponent.hpp>

#define COMPONENT_DESCRIPTION_CFUNCTIONALREGRESSION "  linear and quadratic regression coefficients and corresponding linear and quadratic regression errors. Linear regression line: y = m*x + t ; quadratic regression parabola: y = a*x^2 + b*x + c . Algorithm used: Minimum mean square error, direct analytic solution. This component also computes the centroid of the contour."
#define COMPONENT_NAME_CFUNCTIONALREGRESSION "cFunctionalRegression"

class cFunctionalRegression : public cFunctionalComponent {
  private:
    int enQreg;
    int oldBuggyQerr;
    int normRegCoeff;
    int normInputs;
    int centroidNorm;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    virtual void myFetchConfig() override;

    int doRatioLimit_;
    int centroidRatioLimit_;
    int centroidUseAbsValues_;

    // soft limit to [-10;10] (linear) and non-linear hard-limit
    // to [-20;20] for inputs [-inf;+inf]
    FLOAT_DMEM ratioLimit(FLOAT_DMEM x) {
      if (doRatioLimit_) {
        return smileMath_ratioLimit(x, 10.0, 10.0);
      }
      return x;
    }
    FLOAT_DMEM ratioLimitMax(FLOAT_DMEM altValue) {
      if (doRatioLimit_) {
        return 20.0;  // sum of limit1 + limit2 in above function
      }
      return altValue;
    }

    // limit to -1..+1
    FLOAT_DMEM ratioLimitUnity(FLOAT_DMEM x) {
      if (doRatioLimit_) {
        if (x > (FLOAT_DMEM)1.0)
          return (FLOAT_DMEM)1.0;
        if (x < (FLOAT_DMEM)-1.0)
          return (FLOAT_DMEM)-1.0;
      } 
      return x;
    }

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFunctionalRegression(const char *name);
    // inputs: sorted and unsorted array of values, out: pointer to space in output array, You may not return MORE than Nout elements, please return as return value the number of actually computed elements (usually Nout)
    virtual long process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM min, FLOAT_DMEM max, FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout) override;

    virtual long getNoutputValues() override { return nEnab; }
    virtual int getRequireSorted() override { return 0; }

    virtual ~cFunctionalRegression();
};




#endif // __CFUNCTIONALREGRESSION_HPP
