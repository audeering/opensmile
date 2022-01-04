/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: preemphasis

simple preemphasis : y(t) = x(t) - k*x(t-1)

k = exp( -2*pi * preemphasisFreq./samplingFreq. )

*/


#include <dspcore/preemphasis.hpp>
//#include <math.h>

#define MODULE "cPreemphasis"


SMILECOMPONENT_STATICS(cPreemphasis)

SMILECOMPONENT_REGCOMP(cPreemphasis)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CPREEMPHASIS;
  sdescription = COMPONENT_DESCRIPTION_CPREEMPHASIS;

  // we inherit cWindowProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cWindowProcessor")

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("k","The pre-emphasis coefficient k in y[n] = x[n] - k*x[n-1]",0.97);
    ct->setField("f","The pre-emphasis frequency f in Hz : k = exp( -2*pi * f/samplingFreq. ) (if set, f will override k!)",0,0,0);
    ct->setField("de","1 = perform de-emphasis instead of pre-emphasis (i.e. y[n] = x[n] + k*x[n-1])",0);
  )

  SMILECOMPONENT_MAKEINFO(cPreemphasis);
}

SMILECOMPONENT_CREATE(cPreemphasis)

//-----

cPreemphasis::cPreemphasis(const char *_name) :
  cWindowProcessor(_name,1,0)
{
}


void cPreemphasis::myFetchConfig()
{
  cWindowProcessor::myFetchConfig();
  
  k = (FLOAT_DMEM)getDouble("k");

  if (isSet("f")) {
    f = getDouble("f");
  } else {
    f = -1.0;
  }

  if (f < 0.0) { 
    SMILE_IDBG(2,"k = %f",k);
    if ((k<0.0)||(k>1.0)) {
      SMILE_IERR(1,"k must be in the range [0;1]! Setting k=0.0 !");
      k=0.0;
    }
  } else {

    SMILE_IDBG(2,"using preemphasis frequency f=%f Hz instead of k",f);
  }

  de=getInt("de");
  if (de) {
    SMILE_IDBG(2,"performing de-emphasis instead of pre-emphasis");
  }
}

int cPreemphasis::dataProcessorCustomFinalise()
{
  int ret = cWindowProcessor::dataProcessorCustomFinalise();
  if (f >= 0.0) {
    double _T = reader_->getLevelT();
    k = (FLOAT_DMEM)exp( -2.0*M_PI * f * _T );
    SMILE_IDBG(2,"computed k from f (%f Hz) : k = %f  (samplingRate = %f Hz)",f,k,1.0/_T);
  }
  return ret;
}

// order is the amount of memory (overlap) that will be present in _in
// buf will have nT timesteps, however also order negative indicies (i.e. you may access a negative array index up to -order!)
int cPreemphasis::processBuffer(cMatrix *_in, cMatrix *_out, int _pre, int _post )
{
  long n;

  FLOAT_DMEM *x=_in->data;
  FLOAT_DMEM *y=_out->data;
  if (de) {
    for (n=0; n<_out->nT; n++) {
      *(y++) = *(x) + k * *(x-1);
      x++;
    }
  } else {
    for (n=0; n<_out->nT; n++) {
      *(y++) = *(x) - k * *(x-1);
      x++;
    }
  }
  return 1;
}


cPreemphasis::~cPreemphasis()
{
}

