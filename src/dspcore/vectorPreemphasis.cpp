/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

pre-emphasis per frame  (simplification, however, this is the way HTK does it... so for compatibility... here you go)
(use before window function is applied!)

*/


#include <dspcore/vectorPreemphasis.hpp>

#define MODULE "cVectorPreemphasis"

SMILECOMPONENT_STATICS(cVectorPreemphasis)

SMILECOMPONENT_REGCOMP(cVectorPreemphasis)
{
  SMILECOMPONENT_REGCOMP_INIT
  
  scname = COMPONENT_NAME_CVECTORPREEMPHASIS;
  sdescription = COMPONENT_DESCRIPTION_CVECTORPREEMPHASIS;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("k","The pre-emphasis coefficient k in y[n] = x[n] - k*x[n-1]",0.97);
    ct->setField("f","The pre-emphasis frequency f in Hz : k = exp( -2*pi * f/samplingFreq. ) (this option will override k)",0,0,0);
    ct->setField("de","1 = perform de- instead of pre-emphasis",0);
  )
  SMILECOMPONENT_MAKEINFO(cVectorPreemphasis);
}

SMILECOMPONENT_CREATE(cVectorPreemphasis)

//-----

cVectorPreemphasis::cVectorPreemphasis(const char *_name) :
  cVectorProcessor(_name),
  k(0.0)
{

}

void cVectorPreemphasis::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();
  
  k=(FLOAT_DMEM)getDouble("k");

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
  SMILE_IDBG(2,"de=%f",de);
}

int cVectorPreemphasis::dataProcessorCustomFinalise()
{
  int ret = cVectorProcessor::dataProcessorCustomFinalise();
  if (f >= 0.0) {
    double _T = getBasePeriod();
    k = (FLOAT_DMEM)exp( -2.0*M_PI * f * _T );
    SMILE_IDBG(2,"computed k from f (%f Hz) : k = %f  (samplingRate = %f Hz)",f,k,1.0/_T);
  }
  return ret;
}

// a derived class should override this method, in order to implement the actual processing
int cVectorPreemphasis::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // do domething to data in *src, save result to *dst
  // NOTE: *src and *dst may be the same...

  *(dst++) = (1-k) * *(src++); // - k * *(src-1);
  long n;
  if (de) {
    for (n=1; n<Ndst; n++) {
      *(dst++) = *(src) + k * *(src-1);
      src++;
    }
  } else {
    for (n=1; n<Ndst; n++) {
      *(dst++) = *(src) - k * *(src-1);
      src++;
    }
  }
  return 1;
}

cVectorPreemphasis::~cVectorPreemphasis()
{
}

