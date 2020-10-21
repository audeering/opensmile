/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: statistical moments

*/



#include <functionals/functionalMoments.hpp>

#define MODULE "cFunctionalMoments"


#define FUNCT_VAR        0
#define FUNCT_STDDEV     1
#define FUNCT_SKEWNESS   2
#define FUNCT_KURTOSIS   3
#define FUNCT_AMEAN      4
#define FUNCT_STDDEV_NORM   5    // coefficient of variance, allows for negative values
#define FUNCT_STDDEV_NORM_ABS   6    // coefficient of variance, takes abs val of mean before dividing by mean

#define N_FUNCTS  7

#define NAMES     "variance","stddev","skewness","kurtosis","amean","stddevNorm","coeffOfVariation"

const char *momentsNames[] = {NAMES};  // change variable name to your class...

SMILECOMPONENT_STATICS(cFunctionalMoments)

SMILECOMPONENT_REGCOMP(cFunctionalMoments)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFUNCTIONALMOMENTS;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALMOMENTS;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("variance","1/0=enable/disable output of variance",1);
    ct->setField("stddev","1/0=enable/disable output of standard deviation",1);
    ct->setField("skewness","1/0=enable/disable output of skewness",1);
    ct->setField("kurtosis","1/0=enable/disable output of kurtosis",1);
    ct->setField("amean","1/0=enable/disable output of arithmetic mean",0);
    ct->setField("stddevNorm","(2)/1/0=(enable compatible)/enable/disable output of standard deviation normalised to the absolute value of the arithmetic mean (stddev/abs(mean), also referred to as 'coefficient of variance'). NOTE: in older pre 2.2-final versions of openSMILE, this option computed stddev/mean without the abs() operation. To enable a behaviour compatible to this, set this option to the value 2.", 0);
    ct->setField("doRatioLimit", "(1/0) = yes/no. Apply soft limiting of stddevNorm ratio feature in order to avoid high uncontrolled output values if the denominator is close to 0. For strict compatibility with pre 2.2 openSMILE releases (also release candidates 2.2rc1), set it to 0 (current default)", 0);
  )

  SMILECOMPONENT_MAKEINFO_NODMEM(cFunctionalMoments);
}

SMILECOMPONENT_CREATE(cFunctionalMoments)

//-----

cFunctionalMoments::cFunctionalMoments(const char *name) :
  cFunctionalComponent(name, N_FUNCTS, momentsNames)
{
}

void cFunctionalMoments::myFetchConfig()
{
  if (getInt("variance")) enab[FUNCT_VAR] = 1;
  if (getInt("stddev")) enab[FUNCT_STDDEV] = 1;
  if (getInt("skewness")) enab[FUNCT_SKEWNESS] = 1;
  if (getInt("kurtosis")) enab[FUNCT_KURTOSIS] = 1;
  if (getInt("amean")) enab[FUNCT_AMEAN] = 1;
  if (getInt("stddevNorm") == 2) {
    enab[FUNCT_STDDEV_NORM] = 1;
    enab[FUNCT_STDDEV_NORM_ABS] = 0;
    // NOTE: only of STDDEV_NORM or STDDEV_NORM_ABS might be enabled at once!!
  }
  else if (getInt("stddevNorm") == 1) {
    enab[FUNCT_STDDEV_NORM] = 0;
    enab[FUNCT_STDDEV_NORM_ABS] = 1;
  }
  doRatioLimit_ = getInt("doRatioLimit");
  cFunctionalComponent::myFetchConfig();
}

long cFunctionalMoments::process(FLOAT_DMEM *in,
    FLOAT_DMEM *inSorted, FLOAT_DMEM min, FLOAT_DMEM max,
    FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout)
{
  int i;
  if ((Nin>0)&&(out!=NULL)) {
    double m2 = 0.0;
    double m3 = 0.0;
    double m4 = 0.0;
    double Nind = (double)Nin;
    double tmp, tmp2;
    FLOAT_DMEM *in0=in;
    double meanD = (double)mean;
    for (i=0; i<Nin; i++) {
      tmp = ((double)*(in++) - meanD);
      tmp2 = tmp * tmp;
      m2 += tmp2;
      tmp2 *= tmp;
      m3 += tmp2;
      m4 += tmp2*tmp;
    }
    m2 /= Nind;  // variance

    int n=0;
    if (enab[FUNCT_VAR])
      out[n++] = (FLOAT_DMEM)m2;
    double sqm2 = sqrt(m2);
    if (enab[FUNCT_STDDEV]) {
      if (m2 > 0.0)
        out[n++] = (FLOAT_DMEM)sqm2;
      else
        out[n++] = 0.0;
    }
    if (enab[FUNCT_SKEWNESS]) {
      if (m2 > 0.0)
        out[n++] = (FLOAT_DMEM)( m3/(Nind*m2*sqm2) );
      else
        out[n++] = 0.0;
    }
    if (enab[FUNCT_KURTOSIS]) {
      if (m2 > 0.0)
        out[n++]=(FLOAT_DMEM)( m4/(Nind*m2*m2) );
      else
        out[n++] = 0.0;
    }
    if (enab[FUNCT_AMEAN])
      out[n++]=(FLOAT_DMEM)mean;
    if (enab[FUNCT_STDDEV_NORM] || enab[FUNCT_STDDEV_NORM_ABS]) {
      if (m2 > 0.0) {
        FLOAT_DMEM meanLocal;
        if (enab[FUNCT_STDDEV_NORM_ABS]) {
          meanLocal = fabs(mean);
        } else {
          meanLocal = mean;
        }
        if (doRatioLimit_) {
          if (meanLocal != 0.0) {
            double sqm2N = smileMath_ratioLimit((FLOAT_DMEM)(sqm2 / (double)meanLocal), 
              (FLOAT_DMEM)10.0, (FLOAT_DMEM)20.0);
            out[n++] = (FLOAT_DMEM)sqm2N;
          } else {
            out[n++] = 20.0;
          }
        } else {
          double mean1 = (double)meanLocal;
          if (mean1 == 0.0)
            mean1 = 1.0;
          double sqm2N = sqm2 / mean1;
          out[n++]=(FLOAT_DMEM)sqm2N;
        }
      } else {
        out[n++] = 0.0;
      }
    }
    return n;
  }
  return 0;
}

cFunctionalMoments::~cFunctionalMoments()
{
}

