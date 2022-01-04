/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

functionals: percentiles and quartiles, and inter-percentile/quartile ranges

*/


#include <math.h>
#include <functionals/functionalPercentiles.hpp>


#define MODULE "cFunctionalPercentiles"


#define FUNCT_QUART1        0
#define FUNCT_QUART2        1
#define FUNCT_QUART3        2
#define FUNCT_IQR12         3
#define FUNCT_IQR23         4
#define FUNCT_IQR13         5
#define FUNCT_PERCENTILE    6
#define FUNCT_PCTLRANGE     7
#define FUNCT_PCTLQUOT      8

#define N_FUNCTS  9

#define NAMES     "quartile1","quartile2","quartile3","iqr1-2","iqr2-3","iqr1-3","percentile","pctlrange","pctlquotient"
#define IDX_VAR_FUNCTS 6  // start of percentile, pctlrange, etc. arrays of variable length

const char *percentilesNames[] = {NAMES};  // change variable name to your clas...

SMILECOMPONENT_STATICS(cFunctionalPercentiles)

SMILECOMPONENT_REGCOMP(cFunctionalPercentiles)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFUNCTIONALPERCENTILES;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALPERCENTILES;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("quartiles","1/0=enable/disable output of all quartiles (overrides individual settings quartile1, quartile2, and quartile3)", 0);
    ct->setField("quartile1","1/0=enable/disable output of quartile1 (0.25)",0,0,0);
    ct->setField("quartile2","1/0=enable/disable output of quartile2 (0.50)",0,0,0);
    ct->setField("quartile3","1/0=enable/disable output of quartile3 (0.75)",0,0,0);
    ct->setField("iqr","1/0=enable/disable output of all inter-quartile ranges (overrides individual settings iqr12, iqr23, and iqr13)", 0);
    ct->setField("iqr12","1/0=enable/disable output of inter-quartile range 1-2 (quartile2-quartile1)",0,0,0);
    ct->setField("iqr23","1/0=enable/disable output of inter-quartile range 2-3 (quartile3-quartile2)",0,0,0);
    ct->setField("iqr13","1/0=enable/disable output of inter-quartile range 1-3 (quartile3-quartile1)",0,0,0);
    ct->setField("iqq","1/0=enable/disable output of all inter-quartile quotients (ratios) (overrides individual settings iqq12, iqq23, and iqq13)", 0);
    ct->setField("iqq12","1/0=enable/disable output of inter-quartile quotient q1/q2",0,0,0);
    ct->setField("iqq23","1/0=enable/disable output of inter-quartile quotient q2/q3",0,0,0);
    ct->setField("iqq13","1/0=enable/disable output of inter-quartile quotient q1/q3",0,0,0);

    ct->setField("percentile","Array of p*100 percent percentiles to compute. p = 0..1. Array size indicates the number of total percentiles to compute (excluding quartiles), duplicate entries are not checked for and not removed  : percentile[n] = p (p=0..1)",0.9,ARRAY_TYPE);
    ct->setField("pctlrange","Array that specifies which inter percentile ranges to compute. A range is specified as 'n1-n2' (where n1 and n2 are the indicies of the percentiles as they appear in the percentile[] array, starting at 0 with the index of the first percentile)","0-1",ARRAY_TYPE);
    ct->setField("pctlquotient","Array that specifies which inter percentile quotients to compute. A quotient is specified as 'n1-n2' (where n1 and n2 are the indicies of the percentiles as they appear in the percentile[] array, starting at 0 with the index of the first percentile). The quotient is computed as n1/n2.","0-1",ARRAY_TYPE);
//    ct->setField("quickAlgo","do not sort input, use Dejan's quick estimation method instead",0);
    ct->setField("interp","If set to 1, percentile values will be linearly interpolated, instead of being rounded to the nearest index in the sorted array",1);
     // TOOD: implement quotients!
  )
  
  SMILECOMPONENT_MAKEINFO_NODMEM(cFunctionalPercentiles);
}

SMILECOMPONENT_CREATE(cFunctionalPercentiles)

//-----

cFunctionalPercentiles::cFunctionalPercentiles(const char *name) :
  cFunctionalComponent(name, N_FUNCTS, percentilesNames),
  pctl(NULL),
  pctlr1(NULL), pctlr2(NULL),
  pctlq1(NULL), pctlq2(NULL),
  tmpstr(NULL),
  quickAlgo(0),
  interp(0),
  varFctIdx(0)
{}

void cFunctionalPercentiles::myFetchConfig()
{
//  quickAlgo = getInt("quickAlgo");
  interp = getInt("interp");

  enab[FUNCT_QUART1] = enab[FUNCT_QUART2] = enab[FUNCT_QUART3] = 0;
  if (getInt("quartile1")) enab[FUNCT_QUART1] = 1;
  if (getInt("quartile2")) enab[FUNCT_QUART2] = 1;
  if (getInt("quartile3")) enab[FUNCT_QUART3] = 1;
  if (isSet("quartiles")) {
    enab[FUNCT_QUART1] = enab[FUNCT_QUART2] = enab[FUNCT_QUART3] = getInt("quartiles");
  }
  
  enab[FUNCT_IQR12] = enab[FUNCT_IQR23] = enab[FUNCT_IQR13] = 0;
  if (getInt("iqr12")) enab[FUNCT_IQR12] = 1;
  if (getInt("iqr23")) enab[FUNCT_IQR23] = 1;
  if (getInt("iqr13")) enab[FUNCT_IQR13] = 1;
  if (isSet("iqr")) {
    enab[FUNCT_IQR12] = enab[FUNCT_IQR23] = enab[FUNCT_IQR13] = getInt("iqr");
  }

  int i;
  nPctl = getArraySize("percentile");
  nPctlRange = getArraySize("pctlrange");
  nPctlQuot = getArraySize("pctlquotient");
  if (nPctl > 0) {
    enab[FUNCT_PERCENTILE] = 1;
    pctl = (double*)calloc(1,sizeof(double)*nPctl);
    // load percentile info
    for (i=0; i<nPctl; i++) {
      pctl[i] = getDouble_f(myvprint("percentile[%i]",i));
      if (pctl[i] < 0.0) {
        SMILE_IWRN(2,"percentile[%i] is out of range [0..1] : %f (clipping to 0.0)",i,pctl[i]);
        pctl[i] = 0.0;
      }
      if (pctl[i] > 1.0) {
        SMILE_IWRN(2,"percentile[%i] is out of range [0..1] : %f (clipping to 1.0)",i,pctl[i]);
        pctl[i] = 1.0;
      }
    }
    if (nPctlRange > 0) {
      enab[FUNCT_PCTLRANGE] = 1;
      pctlr1 = (int*)calloc(1,sizeof(int)*nPctlRange);
      pctlr2 = (int*)calloc(1,sizeof(int)*nPctlRange);
      for (i=0; i<nPctlRange; i++) {
        char *tmp = strdup(getStr_f(myvprint("pctlrange[%i]",i)));
        char *orig = strdup(tmp);
        int l = (int)strlen(tmp);
        int err=0;
        // remove spaces and tabs at beginning and end
//        while ( (l>0)&&((*tmp==' ')||(*tmp=='\t')) ) { *(tmp++)=0; l--; }
//        while ( (l>0)&&((tmp[l-1]==' ')||(tmp[l-1]=='\t')) ) { tmp[l-1]=0; l--; }
        // find '-'
        char *s2 = strchr(tmp,'-');
        if (s2 != NULL) {
          *(s2++) = 0;
          char *ep=NULL;
          int r1 = strtol(tmp,&ep,10);
          if ((r1==0)&&(ep==tmp)) { err=1; }
          else if ((r1 < 0)||(r1>=nPctl)) {
                 SMILE_IERR(1,"percentile range [%i] = '%s' (X-Y):: X (=%i) is out of range (allowed: [0..%i])",i,orig,r1,nPctl);
               }
          ep=NULL;
          int r2 = strtol(s2,&ep,10);
          if ((r2==0)&&(ep==tmp)) { err=1; }
          else {
               if ((r2 < 0)||(r2>=nPctl)) {
                 SMILE_IERR(1,"percentile range [%i] = '%s' (X-Y):: Y (=%i) is out of range (allowed: [0..%i])",i,orig,r2,nPctl);
               } else {
                 if (r2 == r1) {
                   SMILE_IERR(1,"percentile range [%i] = '%s' (X-Y):: X must be != Y !!",i,orig);
                 }
               }
             }
          if (!err) {
            pctlr1[i] = r1;
            pctlr2[i] = r2;
          }
        } else { err=1; }

        if (err==1) {
          COMP_ERR("(inst '%s') Error parsing percentile range [%i] = '%s'! (Range must be X-Y, where X and Y are positive integer numbers!)",getInstName(),i,orig);
          pctlr1[i] = -1;
          pctlr2[i] = -1;
        }
        free(orig);
        free(tmp);
      }
    }
    if (nPctlQuot > 0) {
      enab[FUNCT_PCTLQUOT] = 1;
      pctlq1 = (int*)calloc(1,sizeof(int)*nPctlQuot);
      pctlq2 = (int*)calloc(1,sizeof(int)*nPctlQuot);
      for (i=0; i<nPctlQuot; i++) {
        char *tmp = strdup(getStr_f(myvprint("pctlquotient[%i]", i)));
        char *orig = strdup(tmp);
        int l = (int)strlen(tmp);
        int err = 0;
        // remove spaces and tabs at beginning and end
        //        while ( (l>0)&&((*tmp==' ')||(*tmp=='\t')) ) { *(tmp++)=0; l--; }
        //        while ( (l>0)&&((tmp[l-1]==' ')||(tmp[l-1]=='\t')) ) { tmp[l-1]=0; l--; }
        // find '-'
        char *s2 = strchr(tmp,'-');
        if (s2 != NULL) {
          *(s2++) = 0;
          char *ep=NULL;
          int r1 = strtol(tmp,&ep,10);
          if ((r1==0)&&(ep==tmp)) {
            err=1;
          } else if ((r1 < 0)||(r1>=nPctl)) {
            SMILE_IERR(1,"(inst '%s') percentile quotient [%i] = '%s' (X-Y):: X (=%i) is out of range (allowed: [0..%i])",
                getInstName(), i, orig, r1, nPctl);
          }
          ep = NULL;
          int r2 = strtol(s2,&ep,10);
          if ((r2==0)&&(ep==tmp)) {
            err=1;
          } else {
            if ((r2 < 0)||(r2>=nPctl)) {
              SMILE_IERR(1,"(inst '%s') percentile quotient [%i] = '%s' (X-Y):: Y (=%i) is out of range (allowed: [0..%i])",
                  getInstName(),i,orig,r2,nPctl);
            } else {
              if (r2 == r1) {
                SMILE_IERR(1,"(inst '%s') percentile quotient [%i] = '%s' (X-Y):: X must be != Y !!",
                    getInstName(),i,orig);
              }
            }
          }
          if (!err) {
            pctlq1[i] = r1;
            pctlq2[i] = r2;
          }
        } else { err=1; }

        if (err==1) {
          COMP_ERR("(inst '%s') Error parsing percentile quotient [%i] = '%s'! (Quotient range must be X-Y, where X and Y are positive integer numbers!)",
              getInstName(),i,orig);
          pctlq1[i] = -1;
          pctlq2[i] = -1;
        }
        free(orig);
        free(tmp);
      }
    }
  }

  cFunctionalComponent::myFetchConfig();
  if (enab[FUNCT_PERCENTILE])
    nEnab += nPctl-1;
  if (enab[FUNCT_PCTLRANGE])
    nEnab += nPctlRange-1;
  if (enab[FUNCT_PCTLQUOT])
      nEnab += nPctlQuot-1;

  // compute new var index:
  varFctIdx = 0;
  for (i=0; i<IDX_VAR_FUNCTS; i++) {
    if (enab[i]) varFctIdx++;
  }
}

const char* cFunctionalPercentiles::getValueName(long i)
{
  if (i<varFctIdx) {
    return cFunctionalComponent::getValueName(i);
  }
  if (i>=varFctIdx) {
    int j=varFctIdx;
    int pr=0;
    // check, if percentile or pctlrange is referenced:
    i -= varFctIdx;
    if (i>=nPctl) {
      j++;
      i-= nPctl;
      pr = 1;
      if (i >= nPctlRange) {
        i -= nPctlRange;
        j++;
      }
    }
    const char *n = cFunctionalComponent::getValueName(j);
    // append [i]
    if (tmpstr != NULL) free(tmpstr);
    if (!pr) {
      tmpstr = myvprint("%s%.1f",n,pctl[i]*100.0);
    } else {
      if (i >= nPctlRange) {
        tmpstr = myvprint("%s%i-%i",n,pctlq1[i],pctlq2[i]);
      } else {
        tmpstr = myvprint("%s%i-%i",n,pctlr1[i],pctlr2[i]);
      }
    }
    return tmpstr;
  }
  return "UNDEF";
}

// convert percentile to absolute index
long cFunctionalPercentiles::getPctlIdx(double p, long N)
{
  long ret = (long)round(p*(double)(N-1));
  if (ret<0) return 0;
  if (ret>=N) return N-1;
  return ret;
}

// get linearly interpolated percentile
FLOAT_DMEM cFunctionalPercentiles::getInterpPctl(double p, FLOAT_DMEM *sorted, long N)
{
  double idx = p*(double)(N-1);
  long i1,i2;
  i1=(long)floor(idx);
  i2=(long)ceil(idx);
  if (i1<0) i1=0;
  if (i2<0) i2=0;
  if (i1>=N) i1=N-1;
  if (i2>=N) i2=N-1;
  if (i1!=i2) {
    double w1,w2;
    w1 = idx-(double)i1;
    w2 = (double)i2 - idx;
    return sorted[i1]*(FLOAT_DMEM)w2 + sorted[i2]*(FLOAT_DMEM)w1;
  } else {
    return sorted[i1];
  }
}

long cFunctionalPercentiles::process(FLOAT_DMEM *in,
    FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout)
{
  long i;
  if ((Nin>0)&&(out!=NULL)) {
    int n=0;
    FLOAT_DMEM q1, q2, q3;

    if (quickAlgo) {
      // Not yet implemented....
      /*
      // use *in array...

      //find min/max:
      FLOAT_DMEM *in0 = in;
      FLOAT_DMEM *inE = in+Nin;
      FLOAT_DMEM max = *in;
      FLOAT_DMEM min = *in;
      while (in<inE) {
        if (*in < min) min = *in;
        if (*in > max) max = *(in++);
      }
      
      //create range buffers:
      long oversample = 10;
      long nBins = Nin * oversample;
      FLOAT_DMEM unit = (max-min)/(FLOAT_DMEM)nBins;
      long *bins = (long*)calloc(1,sizeof(long)*nBins);

      in=in0;
      FLOAT_DMEM cunit = min;
      for (i=0; i<Nin; i++) {
//NO!! compute bin index from value!        if ((*in > cuint)&&(*in < cuint+unit)) { bins[i]++; }
        FLOAT_DMEM idx = (*in-min)/unit;
        bins
        in++; cunit += unit;
      }
      
      // we need all required percentiles (sorted! wiht corresp. indicies) in an array.



      free(bins);
      */
    } else {
      long minpos=0, maxpos=0;
      if (inSorted == NULL) {
        SMILE_IERR(1,"expected sorted input, however got NULL!");
      }
      // quartiles:
      if (interp) {
        q1 = getInterpPctl(0.25,inSorted,Nin);
        q2 = getInterpPctl(0.50,inSorted,Nin);
        q3 = getInterpPctl(0.75,inSorted,Nin);
      } else {
        q1 = inSorted[getPctlIdx(0.25,Nin)];
        q2 = inSorted[getPctlIdx(0.50,Nin)];
        q3 = inSorted[getPctlIdx(0.75,Nin)];
      }
      if (enab[FUNCT_QUART1]) out[n++]=q1;
      if (enab[FUNCT_QUART2]) out[n++]=q2;
      if (enab[FUNCT_QUART3]) out[n++]=q3;
      if (enab[FUNCT_IQR12]) out[n++]=q2-q1;
      if (enab[FUNCT_IQR23]) out[n++]=q3-q2;
      if (enab[FUNCT_IQR13]) out[n++]=q3-q1;

      // percentiles
      if ((enab[FUNCT_PERCENTILE])||(enab[FUNCT_PCTLRANGE])||(enab[FUNCT_PCTLQUOT])) {
        int n0 = n; // start of percentiles array (used later for computation of pctlranges)
        if (interp) {
          for (i=0; i<nPctl; i++) {
            out[n++] = getInterpPctl(pctl[i],inSorted,Nin);
          }
        } else {
          for (i=0; i<nPctl; i++) {
            out[n++] = inSorted[getPctlIdx(pctl[i],Nin)];
          }
        }
        if (enab[FUNCT_PCTLRANGE]) {
          for (i=0; i<nPctlRange; i++) {
            if ((pctlr1[i]>=0)&&(pctlr2[i]>=0)) {
              out[n++] = fabs(out[n0+pctlr2[i]] - out[n0+pctlr1[i]]);
            } else { out[n++] = 0.0; }
          }
        }
        if (enab[FUNCT_PCTLRANGE]) {
          for (i = 0; i < nPctlQuot; i++) {
            if ((pctlq1[i] >= 0) && (pctlq2[i] >= 0) && (out[n0+pctlq1[i]] != 0.0)) {
              out[n++] = smileMath_ratioLimit(out[n0+pctlq1[i]] / out[n0+pctlq2[i]],
                  50.0, 100.0);
            } else {
              out[n++] = 0.0;
            }
          }
        }
      }
    }

    return n;
  }
  return 0;
}

cFunctionalPercentiles::~cFunctionalPercentiles()
{
  if (pctl != NULL)
    free(pctl);
  if (pctlr1 != NULL)
    free(pctlr1);
  if (pctlr2 != NULL)
    free(pctlr2);
  if (pctlq1 != NULL)
    free(pctlq1);
  if (pctlq2 != NULL)
    free(pctlq2);
  if (tmpstr != NULL)
    free(tmpstr);
}

