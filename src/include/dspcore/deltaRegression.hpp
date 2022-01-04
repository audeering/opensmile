/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: delta coefficients

compute delta regression using regression formula

*/



#ifndef __CDELTAREGRESSION_HPP
#define __CDELTAREGRESSION_HPP

#include <core/smileCommon.hpp>
#include <core/windowProcessor.hpp>
#include <cmath>

#define COMPONENT_DESCRIPTION_CDELTAREGRESSION "This component computes delta regression coefficients using the regression equation from the HTK book."
#define COMPONENT_NAME_CDELTAREGRESSION "cDeltaRegression"

class cDeltaRegression : public cWindowProcessor {
private:
  int halfWaveRect, absOutput;
  int deltawin;
  FLOAT_DMEM norm;

  int zeroSegBound;
  int onlyInSegments;
  int relativeDelta;

protected:
  SMILECOMPONENT_STATIC_DECL_PR


  int isNoValue(FLOAT_DMEM x) {
    if (onlyInSegments && x==0.0) return 1;
    if ((std::isnan)(x)) return 1;
    return 0;
  }

  virtual void myFetchConfig() override;

  //virtual int configureWriter(const sDmLevelConfig *c) override;
  //virtual int setupNamesForField(int i, const char*name, long nEl) override;

  // buffer must include all (# order) past samples
  virtual int processBuffer(cMatrix *_in, cMatrix *_out, int _pre, int _post) override;


public:
  SMILECOMPONENT_STATIC_DECL

    cDeltaRegression(const char *_name);

  virtual ~cDeltaRegression();
};


#endif // __CDELTAREGRESSION_HPP
