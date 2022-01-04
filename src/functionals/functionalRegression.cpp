/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: linear and quadratic regression coefficients

*/


#include <functionals/functionalRegression.hpp>
#include <cmath>

#define MODULE "cFunctionalRegression"


#define FUNCT_LINREGC1     0
#define FUNCT_LINREGC2     1
#define FUNCT_LINREGERRA   2
#define FUNCT_LINREGERRQ   3
#define FUNCT_QREGC1       4
#define FUNCT_QREGC2       5
#define FUNCT_QREGC3       6
#define FUNCT_QREGERRA     7
#define FUNCT_QREGERRQ     8
#define FUNCT_CENTROID     9
#define FUNCT_QREGLS       10  // left slope of parabola
#define FUNCT_QREGRS       11  // right slope of parabola
#define FUNCT_QREGX0       12  // vertex coordinates
#define FUNCT_QREGY0       13  // vertex coordinates
//#define FUNCT_QREGYL       14  // left value (t=0) of parabola  == qregc3 !!
#define FUNCT_QREGYR       14  // right value (t=Nin) of parabola

#define FUNCT_QREGY0NN    15  // vertex y coordinate, not normalised
#define FUNCT_QREGC3NN    16  // qregc3, not normalised
#define FUNCT_QREGYRNN    17  // not normalised

#define N_FUNCTS  18

#define NAMES  "linregc1","linregc2","linregerrA","linregerrQ","qregc1","qregc2","qregc3","qregerrA","qregerrQ","centroid","qregls","qregrs","qregx0","qregy0","qregyr","qregy0nn","qregc3nn","qregyrnn"

const char *regressionNames[] = {NAMES};  // change variable name to your clas...

SMILECOMPONENT_STATICS(cFunctionalRegression)

SMILECOMPONENT_REGCOMP(cFunctionalRegression)
{
  SMILECOMPONENT_REGCOMP_INIT
  
  scname = COMPONENT_NAME_CFUNCTIONALREGRESSION;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALREGRESSION;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("linregc1","1/0=enable/disable output of slope m (linear regression line)",1);
    ct->setField("linregc2","1/0=enable/disable output of offset t (linear regression line)",1);
    ct->setField("linregerrA","1/0=enable/disable output of linear error between contour and linear regression line",1);
    ct->setField("linregerrQ","1/0=enable/disable output of quadratic error between contour and linear regression line",1);
    ct->setField("qregc1","1/0=enable/disable output of quadratic regression coefficient 1 (a)",1);
    ct->setField("qregc2","1/0=enable/disable output of quadratic regression coefficient 2 (b)",1);
    ct->setField("qregc3","1/0=enable/disable output of quadratic regression coefficient 3 (c = offset)",1);
    ct->setField("qregerrA","1/0=enable/disable output of linear error between contour and quadratic regression line (parabola)",1);
    ct->setField("qregerrQ","1/0=enable/disable output of quadratic error between contour and quadratic regression line (parabola)",1);
    ct->setField("centroid","1/0=enable/disable output of centroid of contour (this is computed as a by-product of the regression coefficients).",1);
    ct->setField("centroidNorm","normalise time-scale of centroid to time in seconds (seconds), frame index (frame), or relative segment percentage (segment).", "segment");
    ct->setField("centroidUseAbsValues", "1/0=enable/disable. Use absolute values when computing temporal centroid. Default in pre 2.2 versions was 0. In 2.2 the default changes to 1!", 1);
    ct->setField("centroidRatioLimit", "(1/0) = yes/no. Apply soft limiting of centroid to valid (segment range) in order to avoid high uncontrolled output values if the denominator (absolute mean of values) is close to 0. For strict compatibility with pre 2.2 openSMILE releases (also release candidates 2.2rc1), set it to 0. Default in new versions is 1 (enabled).", 1);
    ct->setField("qregls","1/0=enable/disable output of left slope of parabola (slope of the line from first point on the parabola at t=0 to the vertex).",0);
    ct->setField("qregrs","1/0=enable/disable output of right slope of parabola (slope of the line from the vertex to the last point on the parabola at t=N).",0);
    ct->setField("qregx0","1/0=enable/disable output of x coordinate of the parabola vertex (since for very flat parabolas this can be very large/small, it is clipped to range -Nin - +Nin ).",0);
    ct->setField("qregy0","1/0=enable/disable output of y coordinate of the parabola vertex.",0);
    ct->setField("qregyr","1/0=enable/disable output of y coordinate of the last point on the parabola (t=N).",0);
    ct->setField("qregy0nn","1/0=enable/disable output of y coordinate of the parabola vertex. This value is unnormalised, regardless of value of normInput.",0);
    ct->setField("qregc3nn","1/0=enable/disable output of y coordinate of the first point on the parabola (t=0). This value is unnormalised, regardless of value of normInput.",0);
    ct->setField("qregyrnn","1/0=enable/disable output of y coordinate of the last point on the parabola (t=N). This value is unnormalised, regardless of value of normInput.",0);

    ct->setField("normRegCoeff","If > 0, do normalisation of regression coefficients, slopes, and coordinates on the time scale.\n  If == 1 (segment relative scaling), the coefficients are scaled (multiplied by the contour length) so that a regression line or parabola approximating the contour can be plotted over an x-axis range from 0 to 1, i.e. this makes the coefficients independent of the contour length (a longer contour with a lower slope will then have the same 'm' (slope) linear regression coefficient as a shorter but steeper slope).\n  If == 2, normalisation of time scale to the units of seconds, i.e. slope is value_delta/second.\n  Note: The unnormalised slope is value_delta/timestep.", 0);
    ct->setField("normInputs","1/0=enable/disable normalisation of regression coefficients, coordinates, and regression errors on the value scale. If enabled all input values will be normalised to the range 0..1. Use this in conjunction with normRegCoeff.",0);
    ct->setField("oldBuggyQerr","Set this to 1 (default) to output the (input lengthwise) unnormalised quadratic regression errors (if qregerr* == 1) for compatibility with older feature sets. In new setups you should always change from the default to 0 to enable the proper scaling of the quadratic error!",1);
    ct->setField("doRatioLimit", "(1/0) = yes/no. Apply soft limiting of ratio features (slopes etc.) in order to avoid high uncontrolled output values if the denominator is close to 0. For strict compatibility with pre 2.2 openSMILE releases (also release candidates 2.2rc1), set it to 0 (current default)", 0);
  )

  SMILECOMPONENT_MAKEINFO_NODMEM(cFunctionalRegression);
}

SMILECOMPONENT_CREATE(cFunctionalRegression)

//-----

cFunctionalRegression::cFunctionalRegression(const char *name) :
  cFunctionalComponent(name, N_FUNCTS, regressionNames),
  enQreg(0)
{
}

void cFunctionalRegression::myFetchConfig()
{
  if (getInt("linregc1")) enab[FUNCT_LINREGC1] = 1;
  if (getInt("linregc2")) enab[FUNCT_LINREGC2] = 1;
  if (getInt("linregerrA")) enab[FUNCT_LINREGERRA] = 1;
  if (getInt("linregerrQ")) enab[FUNCT_LINREGERRQ] = 1;
  if (getInt("qregc1")) { enab[FUNCT_QREGC1] = 1; enQreg=1; }
  if (getInt("qregc2")) { enab[FUNCT_QREGC2] = 1; enQreg=1; }
  if (getInt("qregc3")) { enab[FUNCT_QREGC3] = 1; enQreg=1; }
  if (getInt("qregerrA")) { enab[FUNCT_QREGERRA] = 1; enQreg=1; }
  if (getInt("qregerrQ")) { enab[FUNCT_QREGERRQ] = 1; enQreg=1; }
  if (getInt("centroid")) { enab[FUNCT_CENTROID] = 1; enQreg=1; }
  if (getInt("qregls")) { enab[FUNCT_QREGLS] = 1; enQreg=1; }
  if (getInt("qregrs")) { enab[FUNCT_QREGRS] = 1; enQreg=1; }
  if (getInt("qregx0")) { enab[FUNCT_QREGX0] = 1; enQreg=1; }
  if (getInt("qregy0")) { enab[FUNCT_QREGY0] = 1; enQreg=1; }
  if (getInt("qregyr")) { enab[FUNCT_QREGYR] = 1; enQreg=1; }
  if (getInt("qregy0nn")) { enab[FUNCT_QREGY0NN] = 1; enQreg=1; }
  if (getInt("qregc3nn")) { enab[FUNCT_QREGC3NN] = 1; enQreg=1; }
  if (getInt("qregyrnn")) { enab[FUNCT_QREGYRNN] = 1; enQreg=1; }
  const char *centrNorm = getStr("centroidNorm");
  if (!strncmp(centrNorm, "sec", 3)) {
    centroidNorm = TIMENORM_SECONDS;
  } else if (!strncmp(centrNorm, "fra", 3)) {
    centroidNorm = TIMENORM_FRAMES;
  } else if (!strncmp(centrNorm, "seg", 3)) {
    centroidNorm = TIMENORM_SEGMENT;
  } else {
    SMILE_IERR(1, "unknown value for option 'centroidNorm' (%s). Allowed values are: fra(mes), seg(ments), sec(onds)", centrNorm);
  }
  normRegCoeff = getInt("normRegCoeff");
  normInputs = getInt("normInputs");
  centroidUseAbsValues_ = getInt("centroidUseAbsValues");
  centroidRatioLimit_ = getInt("centroidRatioLimit");
  doRatioLimit_ = getInt("doRatioLimit");
  oldBuggyQerr = getInt("oldBuggyQerr");

  cFunctionalComponent::myFetchConfig();
}

long cFunctionalRegression::process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted,
    FLOAT_DMEM min, FLOAT_DMEM max, FLOAT_DMEM mean,
    FLOAT_DMEM *out, long Nin, long Nout)
{
  if ((Nin>0)&&(out!=NULL)) {
    //compute centroid
    FLOAT_DMEM *iE = in + Nin;
    FLOAT_DMEM *i0 = in;
    double Nind = (double)Nin;
    double range = max-min;
    double rangeInv;
    if (range <= 0.0) {
      range = 1.0;
      rangeInv = 0.0;
    } else {
      rangeInv = 1.0/range;
    }

    double num = 0.0;
    double numAbs = 0.0;
    double num2 = 0.0;
    double num2Abs = 0.0;
    double tmp = 0.0;
    double ii = 0.0;
    double asumAbs = 0.0;
    double asum = mean * Nind;
    if (centroidUseAbsValues_) {
      while (in<iE) {
        asumAbs += (double)fabs(*(in));
        tmp = (double)(fabs(*(in))) * ii;
        numAbs += tmp;
        tmp *= ii;
  //      ii += 1.0;
        num2Abs += tmp;
        tmp = (double)(*(in++)) * ii;
        num += tmp;
        tmp *= ii;
        ii += 1.0;
        num2 += tmp;
      }
    } else {
      while (in<iE) {
        tmp = (double)(*(in++)) * ii;
        num += tmp;
        tmp *= ii;
        ii += 1.0;
        num2 += tmp;
      }
    }

    double centroid;
    if (centroidUseAbsValues_) {
      if (asumAbs != 0.0) {
        centroid = numAbs / asumAbs;
      } else {
        centroid = 0.0;
      }
    } else {
      if (asum != 0.0) {
        centroid = num / asum;
      } else {
        centroid = 0.0;
      }
    }
    if (centroidRatioLimit_) {
      centroid = (double)smileMath_ratioLimit((FLOAT_DMEM)(centroid), 
        (FLOAT_DMEM)Nind, (FLOAT_DMEM)Nind);
    }

    if (centroidNorm == TIMENORM_SECONDS) {
      centroid *= getInputPeriod();
    } else if (centroidNorm == TIMENORM_SEGMENT) {
      centroid /= Nind;
    }
    in=i0;

    
    double m=0.0,t=0.0,leq=0.0,lea=0.0;
    double a=0.0,b=0.0,c=0.0,qeq=0.0,qea=0.0;
    double S1,S2,S3,S4;
    if (Nin > 1) {
      // LINEAR REGRESSION:
/*
      S1 = (Nind-1.0)*Nind/2.0;  // sum of all i=0..N-1
      S2 = (Nind-1.0)*Nind*(2.0*Nind-1.0)/6.0; // sum of all i^2 for i=0..N-1
                                              // num: if sum of y_i*i for all i=0..N-1
      t = ( asum - num*S1/S2) / ( Nind - S1*S1/S2 );
      m = ( num - t * S1 ) / S2;
*/ // optimised computation:
      double NNm1 = (Nind)*(Nind-(double)1.0);

      S1 = NNm1/(double)2.0;  // sum of all i=0..N-1
      S2 = NNm1*((double)2.0*Nind-(double)1.0)/(double)6.0; // sum of all i^2 for i=0..N-1

      // check!
      double S1dS2 = S1/S2;
      double tmp = ( Nind - S1*S1dS2 );
      if (tmp == 0.0) t = 0.0;
      else t = ( asum - num*S1dS2) / tmp;
      m = ( num - t * S1 ) / S2;

      // TEST: temporary only: compare with
      // smileMath_getLinearRegressionLine(FLOAT_DMEM *x, long N, FLOAT_DMEM *b)
      //FLOAT_DMEM bNew = 0.0;
      //FLOAT_DMEM mNew = smileMath_getLinearRegressionLine(i0, Nin, &bNew);
      //SMILE_IMSG(3, "bNew = %.4f , bOld = %.4f -- mNew = %.4f , mOld = %.4f (min %.4f, max %.4f)", bNew, t, mNew, m, min, max);

      S3 = S1*S1;
      double Nind1 = Nind-(double)1.0;
      S4 = S2 * ((double)3.0*(Nind1*Nind1 + Nind1)-(double)1.0) / (double)5.0;

      // QUADRATIC REGRESSION:
      if (enQreg) {

        double det;
        double S3S3 = S3*S3;
        double S2S2 = S2*S2;
        double S1S2 = S1*S2;
        double S1S1 = S3;
        det = S4*S2*Nind + (double)2.0*S3*S1S2 - S2S2*S2 - S3S3*Nind - S1S1*S4;

        if (det != 0.0) {
          a = ( (S2*Nind - S1S1)*num2 + (S1S2 - S3*Nind)*num + (S3*S1 - S2S2)*asum ) / det;
          b = ( (S1S2 - S3*Nind)*num2 + (S4*Nind - S2S2)*num + (S3*S2 - S4*S1)*asum ) / det;
          c = ( (S3*S1 - S2S2)*num2 + (S3*S2-S4*S1)*num + (S4*S2 - S3S3)*asum ) / det;
        } else {
          a=0.0; b=0.0; c=0.0;
        }

      }
//    printf("nind:%f  S1=%f,  S2=%f  S3=%f  S4=%f  num2=%f  num=%f  asum=%f t=%f\n",Nind,S1,S2,S3,S4,num2,num,asum,t);
    } else {
      m = 0; t=c=*in;
      a = 0.0; b=0.0;
    }
    
    // linear regression error:
    ii=0.0; double e;
    while (in<iE) {
      e = (double)(*(in++)) - (m*ii + t);
      if (normInputs) e *= rangeInv;
      lea += fabs(e);
      ii += 1.0;
      leq += e*e;
    }
    in=i0;

    double rs=0.0, ls=0.0;
    double x0=0.0, y0=0.0;
    double yr=0.0;
    double yrnn=0.0, c3nn=0.0;
    double x0nn=0.0, y0nn=0.0;

    // quadratic regresssion error:
    if (enQreg) {
      ii=0.0; double e;
      while (in<iE) {
        e = (double)(*(in++)) - (a*ii*ii + b*ii + c);
        if (normInputs) e *= rangeInv;
        qea += fabs(e);
        ii += 1.0;
        qeq += e*e;
      }
      in=i0;

      // parabola vertex (x coordinate clipped to range -Nind + Nind!)  // TODO: why -Nind?
      x0 = b/(-2.0*a);
      if (x0 < -1.0*Nind) x0 = -Nind;
      if (x0 > Nind) x0 = Nind;
      if (!std::isfinite(x0)) x0 = Nind;
      x0nn = x0;
      y0 = c - b*b/(4.0*a);  // TODO: also limit to range 0..Nind
      if (!std::isfinite(y0)) y0 = 0.0;
      y0nn = y0;

      // parabola left / right points
      yrnn = yr = a * (Nind-1.0)*(Nind-1.0) + b*(Nind-1.0) + c;
      if (!std::isfinite(yr)) { yr = 0.0; yrnn = 0.0; }
      c3nn = c;
    }

    int n=0;
    double NOneSec = 1.0;
    if (normRegCoeff == 2)
      NOneSec = 1.0 / getInputPeriod();
    
    if (doRatioLimit_) {
      m = smileMath_ratioLimit((FLOAT_DMEM)m, (FLOAT_DMEM)(range/10.0), 
        (FLOAT_DMEM)(range/10.0 + 0.01));
      a = smileMath_ratioLimit((FLOAT_DMEM)a, (FLOAT_DMEM)(sqrt(range/10.0)), 
        (FLOAT_DMEM)(sqrt(range/10.0) + 0.01));
      b = smileMath_ratioLimit((FLOAT_DMEM)b, (FLOAT_DMEM)(range/10.0), 
        (FLOAT_DMEM)(range/10.0 + 0.01));
    }
    if (normRegCoeff == 1) {  // normalise to segment
      m *= Nind-1.0;
      a *= (Nind-1.0)*(Nind-1.0);
      b *= Nind-1.0;
      if (Nind != 1.0)
        x0 /= Nind-1.0;
      else
        x0 = 0.0;
    }
    else if (normRegCoeff == 2) {  // normalise to seconds
      m *= NOneSec;
      a *= NOneSec * NOneSec;
      b *= NOneSec;
      if (NOneSec != 1.0)
        x0 /= NOneSec;
      else
        x0 = 0.0;
    }
    if (normInputs) {
      m *= rangeInv;
      t = (t - min) * rangeInv;
      a *= rangeInv;
      b *= rangeInv;
      c = (c - min) * rangeInv;
      y0 = (y0 - min) * rangeInv;
      yr = (yr - min) * rangeInv;
    }

    if (enQreg) {
      // parabola partial slopes
      // TODO: check behaviour if vertex is outside of current window
      if (x0 > 0)
        ls = (y0 - c) / x0;
      if (normRegCoeff == 1) {
        if (x0 < 1.0)
          rs = (yr - y0) / (1.0 - x0);
      } else if (normRegCoeff == 2) {
        double len_t = (Nind - 1.0) / NOneSec;
        if (x0 < len_t)
          rs = (yr - y0)/(len_t - x0);
      } else {
        if (x0 < Nind - 1.0)
          rs = (yr - y0) / (Nind - 1.0 - x0);
      }
    }

    // security checks:
    if (!std::isfinite(m)) m = 0.0;
    if (!std::isfinite(t)) t = 0.0;
    if (!std::isfinite(lea/Nind)) lea = 0.0;
    if (!std::isfinite(leq/Nind)) leq = 0.0;
    if (!std::isfinite(a)) a = 0.0;
    if (!std::isfinite(b)) b = 0.0;
    if (!std::isfinite(c)) { c = 0.0; c3nn = 0.0; }
    if (!std::isfinite(ls)) ls = 0.0;
    if (!std::isfinite(rs)) rs = 0.0;
    if (!std::isfinite(qea/Nind)) qea = 0.0;
    if (!std::isfinite(qeq/Nind)) qeq = 0.0;
    if (!std::isfinite(centroid)) centroid = 0.0;

    // save values:
    if (enab[FUNCT_LINREGC1]) out[n++]=(FLOAT_DMEM)m;
    if (enab[FUNCT_LINREGC2]) out[n++]=(FLOAT_DMEM)t;
    if (enab[FUNCT_LINREGERRA]) out[n++]=(FLOAT_DMEM)(lea/Nind);
    if (enab[FUNCT_LINREGERRQ]) out[n++]=(FLOAT_DMEM)(leq/Nind);

    if (enab[FUNCT_QREGC1]) out[n++]=(FLOAT_DMEM)a;
    if (enab[FUNCT_QREGC2]) out[n++]=(FLOAT_DMEM)b;
    if (enab[FUNCT_QREGC3]) out[n++]=(FLOAT_DMEM)c;
    if (!oldBuggyQerr) {
      if (enab[FUNCT_QREGERRA]) out[n++]=(FLOAT_DMEM)(qea/Nind);
      if (enab[FUNCT_QREGERRQ]) out[n++]=(FLOAT_DMEM)(qeq/Nind);
    } else {
      if (enab[FUNCT_QREGERRA]) out[n++]=(FLOAT_DMEM)(qea);
      if (enab[FUNCT_QREGERRQ]) out[n++]=(FLOAT_DMEM)(qeq);
    }

    if (enab[FUNCT_CENTROID]) out[n++]=(FLOAT_DMEM)centroid;

    if (enab[FUNCT_QREGLS]) out[n++]=(FLOAT_DMEM)ls;
    if (enab[FUNCT_QREGRS]) out[n++]=(FLOAT_DMEM)rs;
    if (enab[FUNCT_QREGX0]) out[n++]=(FLOAT_DMEM)x0;
    if (enab[FUNCT_QREGY0]) out[n++]=(FLOAT_DMEM)y0;
    if (enab[FUNCT_QREGYR]) out[n++]=(FLOAT_DMEM)yr;
    if (enab[FUNCT_QREGY0NN]) out[n++]=(FLOAT_DMEM)y0nn;
    if (enab[FUNCT_QREGC3NN]) out[n++]=(FLOAT_DMEM)c3nn;
    if (enab[FUNCT_QREGYRNN]) out[n++]=(FLOAT_DMEM)yrnn;

    return n;
  }
  return 0;
}

cFunctionalRegression::~cFunctionalRegression()
{
}

