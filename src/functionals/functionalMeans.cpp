/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals:
various means, arithmetic, geometric, quadratic, etc.
also number of non-zero values, etc.

*/


#include <functionals/functionalMeans.hpp>
#include <math.h>

#define MODULE "cFunctionalMeans"


#define FUNCT_AMEAN     0
#define FUNCT_ABSMEAN   1
#define FUNCT_QMEAN     2
#define FUNCT_NZAMEAN   3
#define FUNCT_NZABSMEAN 4
#define FUNCT_NZQMEAN   5
#define FUNCT_NZGMEAN   6
#define FUNCT_NNZ       7  // number of Non-Zero elements
#define FUNCT_FLATNESS  8  // flatness : geometric mean / arithmetic mean
#define FUNCT_POSAMEAN  9  // arithmetic mean from positive values only
#define FUNCT_NEGAMEAN  10 // arithmetic mean from negative values only
#define FUNCT_POSQMEAN  11
#define FUNCT_POSRQMEAN 12
#define FUNCT_NEGQMEAN  13
#define FUNCT_NEGRQMEAN 14
#define FUNCT_RQMEAN    15  // *r*oot *q*uadratic mean
#define FUNCT_NZRQMEAN  16

#define N_FUNCTS  17

#define NAMES     "amean","absmean","qmean","nzamean","nzabsmean","nzqmean","nzgmean","nnz","flatness","posamean","negamean","posqmean","posrqmean", "negqmean", "negrqmean", "rqmean", "nzrqmean"

const char *meansNames[] = {NAMES};  // change variable name to your class...

SMILECOMPONENT_STATICS(cFunctionalMeans)

SMILECOMPONENT_REGCOMP(cFunctionalMeans)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFUNCTIONALMEANS;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALMEANS;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("amean","1/0=enable/disable output of arithmetic mean",1);
    ct->setField("absmean","1/0=enable/disable output of arithmetic mean of absolute values",1);
    ct->setField("qmean","1/0=enable/disable output of quadratic mean",1);
    ct->setField("nzamean","1/0=enable/disable output of arithmetic mean (of non-zero values only)",1);
    ct->setField("nzabsmean","1/0=enable/disable output of arithmetic mean of absolute values (of non-zero values only)",1);
    ct->setField("nzqmean","1/0=enable/disable output of quadratic mean (of non-zero values only)",1);
    ct->setField("nzgmean","1/0=enable/disable output of geometric mean (of absolute values of non-zero values only)",1);
    ct->setField("nnz","1/0=enable/disable output of number of non-zero values (relative to the input segment length, in seconds, or in frames, see the 'norm' option or the 'masterTimeNorm' option of the cFunctionals parent component)",1); // default norm for compatibility : frames
    ct->setField("flatness","1/0=enable/disable output of contour flatness (ratio of geometric mean and absolute value arithmetic mean(absmean)))",0);
    ct->setField("posamean","1/0=enable/disable output of arithmetic mean of positive values only (usually you would apply this to a differential signal to measure how much the original signal is rising)",0);
    ct->setField("negamean","1/0=enable/disable output of arithmetic mean of negative values only",0);
    ct->setField("posqmean","1/0=enable/disable output of quadratic mean of positive values only",0);
    ct->setField("posrqmean","1/0=enable/disable output of root of quadratic mean of positive values only",0);
    ct->setField("negqmean","1/0=enable/disable output of quadratic mean of negative values only",0);
    ct->setField("negrqmean","1/0=enable/disable output of root of quadratic mean of negative values only",0);
    ct->setField("rqmean","1/0=enable/disable output of square root of quadratic mean",0);
    ct->setField("nzrqmean","1/0=enable/disable output of square root of quadratic mean of non zero values",0);
    ct->setField("norm","This option specifies how this component should normalise times (if it generates output values related to durations): \n   'segment' (or: 'turn') : normalise to the range 0..1, the result is the relative length wrt. to the segment length )\n   'second'  (absolute time in seconds) \n   'frame' (absolute time in number of frames of input level)","frames");
  )

  SMILECOMPONENT_MAKEINFO_NODMEM(cFunctionalMeans);
}

SMILECOMPONENT_CREATE(cFunctionalMeans)

//-----

cFunctionalMeans::cFunctionalMeans(const char *name) :
  cFunctionalComponent(name, N_FUNCTS, meansNames)
{
}

void cFunctionalMeans::myFetchConfig()
{
  parseTimeNormOption();

  if (getInt("amean")) enab[FUNCT_AMEAN] = 1;
  if (getInt("absmean")) enab[FUNCT_ABSMEAN] = 1;
  if (getInt("qmean")) enab[FUNCT_QMEAN] = 1;
  if (getInt("nzamean")) enab[FUNCT_NZAMEAN] = 1;
  if (getInt("nzabsmean")) enab[FUNCT_NZABSMEAN] = 1;
  if (getInt("nzqmean")) enab[FUNCT_NZQMEAN] = 1;
  if (getInt("nzgmean")) enab[FUNCT_NZGMEAN] = 1;
  if (getInt("nnz")) enab[FUNCT_NNZ] = 1;
  if (getInt("flatness")) enab[FUNCT_FLATNESS] = 1;
  if (getInt("posamean")) enab[FUNCT_POSAMEAN] = 1;
  if (getInt("negamean")) enab[FUNCT_NEGAMEAN] = 1;
  if (getInt("posqmean")) enab[FUNCT_POSQMEAN] = 1;
  if (getInt("posrqmean")) enab[FUNCT_POSRQMEAN] = 1;
  if (getInt("negqmean")) enab[FUNCT_NEGQMEAN] = 1;
  if (getInt("negrqmean")) enab[FUNCT_NEGRQMEAN] = 1;
  if (getInt("rqmean")) enab[FUNCT_RQMEAN] = 1;
  if (getInt("nzrqmean")) enab[FUNCT_NZRQMEAN] = 1;

  cFunctionalComponent::myFetchConfig();
}

long cFunctionalMeans::process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM min, FLOAT_DMEM max, FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout)
{
  int i;
  if ((Nin>0)&&(out!=NULL)) {

    double tmp=(double)*in;
    double fa = fabs(tmp);

    double absmean = fa;
    double qmean = tmp*tmp;
    long nnz;

    double nzamean;
    double nzabsmean;
    double nzqmean;
    double nzgmean;
    double posamean=0.0, negamean=0.0;
    double posqmean=0.0, negqmean=0.0;
    long nPos=0,nNeg=0;

    if (tmp!=0.0) {
      nzamean = tmp;
      nzabsmean = fa;
      nzqmean = tmp*tmp;
      nzgmean = log(fa);
      nnz=1;
      if (tmp > 0) {
        posamean += tmp;
        posqmean += tmp*tmp;
        nPos++;
      } else {
        negamean += tmp;
        negqmean += tmp*tmp;
        nNeg++;
      }
    } else {
      nzamean = 0.0;
      nzabsmean = 0.0;
      nzqmean = 0.0;
      nzgmean = 0.0;
      nnz=0;
    }
    for (i=1; i<Nin; i++) {
      in++;
      tmp=(double)*in;
      fa = fabs(tmp);
      //      amean += tmp;
      absmean += fa;
      if (tmp > 0) {
        posamean += tmp;
        nPos++;
      }
      if (tmp < 0) {
        negamean += tmp;
        nNeg++;
      }
      double _tmp = tmp;
      if (tmp!=0.0) {
        nzamean += tmp;
        nzabsmean += fa;
        nzgmean += log(fa);
        tmp *= tmp;
        nzqmean += tmp;
        nnz++;
        if (_tmp > 0) posqmean += tmp;
        if (_tmp < 0) negqmean += tmp;
        qmean += tmp;
      }
    }
    tmp = (double)Nin;
    //    amean = amean / tmp;
    absmean = absmean / tmp;
    qmean = qmean / tmp;

    if (nnz>0) {
      tmp = (double)nnz;
      nzamean = nzamean / tmp;
      nzabsmean = nzabsmean / tmp;
      nzqmean = nzqmean / tmp;
      nzgmean /= tmp; //pow( 1.0/nzgmean, 1.0/tmp );
      nzgmean = exp(nzgmean);
    }
    if (nPos > 0) {
      posamean /= (double)nPos;
      posqmean /= (double)nPos;
    }
    if (nNeg > 0) {
      negamean /= (double)nNeg;
      negqmean /= (double)nNeg;
    }

    int n=0;
    if (enab[FUNCT_AMEAN]) out[n++]=(FLOAT_DMEM)mean;
    if (enab[FUNCT_ABSMEAN]) out[n++]=(FLOAT_DMEM)absmean;
    if (enab[FUNCT_QMEAN]) out[n++]=(FLOAT_DMEM)qmean;
    if (enab[FUNCT_NZAMEAN]) out[n++]=(FLOAT_DMEM)nzamean;
    if (enab[FUNCT_NZABSMEAN]) out[n++]=(FLOAT_DMEM)nzabsmean;
    if (enab[FUNCT_NZQMEAN]) out[n++]=(FLOAT_DMEM)nzqmean;
    if (enab[FUNCT_NZGMEAN]) out[n++]=(FLOAT_DMEM)nzgmean;
    if (timeNorm==TIMENORM_FRAMES) {
      if (enab[FUNCT_NNZ]) out[n++]=(FLOAT_DMEM)nnz;
    } else if (timeNorm==TIMENORM_SEGMENT) {
      if (enab[FUNCT_NNZ]) out[n++]=(FLOAT_DMEM)nnz/(FLOAT_DMEM)Nin;
    } else if (timeNorm==TIMENORM_SECONDS) {
      if (enab[FUNCT_NNZ]) out[n++]=(FLOAT_DMEM)nnz/(FLOAT_DMEM)getInputPeriod();
    }
    if (enab[FUNCT_FLATNESS]) {
      if (absmean != 0.0)
        out[n++] = (FLOAT_DMEM)(nzgmean/absmean);
      else out[n++] = 1.0;
    }
    
    if (enab[FUNCT_POSAMEAN]) {
      out[n++] = (FLOAT_DMEM)posamean;
    }
    if (enab[FUNCT_NEGAMEAN]) {
      out[n++] = (FLOAT_DMEM)negamean;
    }
    if (enab[FUNCT_POSQMEAN]) {
      out[n++] = (FLOAT_DMEM)posqmean;
    }
    if (enab[FUNCT_POSRQMEAN]) {
      out[n++] = (FLOAT_DMEM)sqrt(posqmean);
    }
    if (enab[FUNCT_NEGQMEAN]) {
      out[n++] = (FLOAT_DMEM)negqmean;
    }
    if (enab[FUNCT_NEGRQMEAN]) {
      out[n++] = (FLOAT_DMEM)sqrt(negqmean);
    }

    if (enab[FUNCT_RQMEAN]) {
      out[n++] = (FLOAT_DMEM)sqrt(qmean);
    }
    if (enab[FUNCT_NZRQMEAN]) {
      out[n++] = (FLOAT_DMEM)sqrt(nzqmean);
    }


    return n;
  }
  return 0;
}

cFunctionalMeans::~cFunctionalMeans()
{
}

