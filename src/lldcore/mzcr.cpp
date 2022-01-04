/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

time signal properties: min/max sample value, mean and zero crossing rates

*/


#include <lldcore/mzcr.hpp>

#define MODULE "cMZcr"

SMILECOMPONENT_STATICS(cMZcr)

SMILECOMPONENT_REGCOMP(cMZcr)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CMZCR;
  sdescription = COMPONENT_DESCRIPTION_CMZCR;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("nameAppend","(ignored here)",(const char*)NULL);
    ct->setField("zcr","(1/0=yes/no) compute zero-crossing rate (zcr)",1);
    ct->setField("mcr","(1/0=yes/no) compute mean-crossing rate (mcr) (i.e. the rate at which a signal crosses its mean, for signals with mean 0 this is identical to the zero-crossing rate)",1);
    ct->setField("amax","(1/0=yes/no) enable output of the maximum *absolute* sample value",1);
    ct->setField("maxmin","(1/0=yes/no) enable output of the maximum and minimum sample value",1);
    ct->setField("dc","(1/0=yes/no) compute the DC-offset (= the arithmetic mean of input values)",0);
  )
  SMILECOMPONENT_MAKEINFO(cMZcr);
}

SMILECOMPONENT_CREATE(cMZcr)

//-----

cMZcr::cMZcr(const char *_name) :
  cVectorProcessor(_name),
  zcr(0), mcr(0),
  amax(0),
  maxmin(0)
{

}

void cMZcr::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();

  zcr = getInt("zcr");
  mcr = getInt("mcr");
  amax = getInt("amax");
  maxmin = getInt("maxmin");
  dc = getInt("dc");
}


int cMZcr::setupNamesForField(int i, const char*name, long nEl)
{
  int n=0;
  char * xx=NULL;

  if (zcr) {
    xx = myvprint("%s_zcr",name);
    writer_->addField( xx, 1 );
    free(xx); n++;
  }

  if (mcr) {
    xx = myvprint("%s_mcr",name);
    writer_->addField( xx, 1 );
    free(xx); n++;
  }

  if (amax) {
    xx = myvprint("%s_absmax",name);
    writer_->addField( xx, 1 );
    free(xx); n++;
  }

  if (maxmin) {
    xx = myvprint("%s_max",name);
    writer_->addField( xx, 1 );
    free(xx);
    xx = myvprint("%s_min",name);
    writer_->addField( xx, 1 );
    free(xx); n += 2;
  }
  if (dc) {
    xx = myvprint("%s_dc",name);
    writer_->addField( xx, 1 );
    free(xx); n++;
  }

  return n;
}


// a derived class should override this method, in order to implement the actual processing
int cMZcr::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // do domething to data in *src, save result to *dst
  // NOTE: *src and *dst may be the same...
  if (Nsrc == 0) return 0;
  
  FLOAT_DMEM mean=src[0];
  FLOAT_DMEM nzc=0.0, nmc = 4.0;
  FLOAT_DMEM max, min, absmax;

  long i;
  if (zcr || mcr || dc) {
    for (i=1; i<Nsrc-1; i++) {
      mean += src[i];
      if (  ( (src[i-1] * src[i+1] <= 0.0) && (src[i]==0.0) ) || (src[i-1] * src[i] < 0.0)  ) nzc += 1.0;
    }
    nzc /= (FLOAT_DMEM)Nsrc;
    mean /= (FLOAT_DMEM)Nsrc;
  }
  if (mcr) {
    for (i=1; i<Nsrc-1; i++) {
      if (  ( ((src[i-1]-mean) * (src[i+1]-mean) <= 0.0) && ((src[i]-mean)==0.0) ) || ((src[i-1]-mean) * (src[i]-mean) < 0.0)  ) nmc++;
    }
    nmc /= (FLOAT_DMEM)Nsrc;
  }
  if (amax || maxmin) {
    max=min=src[0];
    for (i=1; i<Nsrc; i++) {
	  if (src[i] < min) min = src[i];
	  if (src[i] > max) max = src[i];
	}
	if (fabs(min) > fabs(max)) absmax = fabs(min);
	else absmax = fabs(max);
  }
  int n=0;
  if (zcr) dst[n++] = nzc;
  if (mcr) dst[n++] = nmc;

  if (amax) dst[n++] = absmax;
  if (maxmin) {
    dst[n++] = max;
	dst[n++] = min;
  }
  if (dc) {
    dst[n++] = mean;
  }
  return n;
}

cMZcr::~cMZcr()
{
}

