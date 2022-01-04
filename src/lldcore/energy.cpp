/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

compute RMS and log frame energy

*/


#include <lldcore/energy.hpp>

#define MODULE "cEnergy"
double minE = 8.674676e-019;

SMILECOMPONENT_STATICS(cEnergy)

SMILECOMPONENT_REGCOMP(cEnergy)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CENERGY;
  sdescription = COMPONENT_DESCRIPTION_CENERGY;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("nameAppend",NULL,"energy");
    ct->setField("processArrayFields",NULL,0);
    ct->setField("htkcompatible","If set to 1, enable htk compatible log-energy output (this will output log-energy ONLY! no rms energy...)",0);
    ct->setField("rms","1/0 = on/off   output of root-mean-square (RMS) energy ( = square root of ( the sum over all input values n=0...N of (x[n]*x[n]) divided by N)",1);
    ct->setField("energy2", "1/0 = on/off   output of quadratic energy (sum of all x[n]^2  divided by N)", 0);
    ct->setField("log","1/0 = on/off   output logarithmic energy (log-energy). E = log( sum (x[n]*x[n]/N ) ; The argument of the log() function is floored to 8.674676e-019. The output unit has the unit of 'neper' (which is simlar to decibel, with the only difference that the natural logarithm (base e) is used instead of the base-10 logarithm) ",1);
    ct->setField("escaleLog","scale factor to multiply log energy by",1.0);
    ct->setField("escaleRms","scale factor to multiply rms energy by",1.0);
    ct->setField("escaleSquare","scale factor to multiply quadratic energy by",1.0);
    ct->setField("ebiasLog","bias to add to log energy",0.0);
    ct->setField("ebiasRms","bias to add to rms energy",0.0);
    ct->setField("ebiasSquare","bias to add to quadratic energy",0.0);
  )
  SMILECOMPONENT_MAKEINFO(cEnergy);
}

SMILECOMPONENT_CREATE(cEnergy)

//-----

cEnergy::cEnergy(const char *_name) :
  cVectorProcessor(_name)
{

}

void cEnergy::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();
  
  htkcompatible = getInt("htkcompatible");
  if (htkcompatible) { SMILE_IDBG(2,"htkcompatible log-energy is enabled"); }

  erms = getInt("rms");
  energy2 = getInt("energy2");
  elog = getInt("log");

  if (htkcompatible) { elog=1; erms=0;  }

  if (erms) { SMILE_IDBG(2,"rms-energy output enabled"); }
  if (elog) { SMILE_IDBG(2,"log-energy output enabled"); }

  ebiasLog = (FLOAT_DMEM)getDouble("ebiasLog");
  ebiasRms = (FLOAT_DMEM)getDouble("ebiasRms");
  ebiasSquare = (FLOAT_DMEM)getDouble("ebiasSquare");
  escaleRms = (FLOAT_DMEM)getDouble("escaleRms");
  escaleSquare = (FLOAT_DMEM)getDouble("escaleSquare");
  escaleLog = (FLOAT_DMEM)getDouble("escaleLog");
}

/*
int cEnergy::myConfigureInstance()
{
  int ret=1;
  ret *= cVectorProcessor::myConfigureInstance();
  if (ret == 0) return 0;

//...


  return ret;
}
*/

/*
int cEnergy::configureWriter(const sDmLevelConfig *c)
{

  // you must return 1, in order to indicate configure success (0 indicated failure)
  return 1;
}

*/

int cEnergy::setupNamesForField(int i, const char*name, long nEl)
{
  int n=0;

  if (erms) {
    addNameAppendFieldAuto(name, "RMS", 1); n++;
  }
  if (energy2) {
    addNameAppendFieldAuto(name, "SQUARED", 1); n++;
  }
/* 
    if ((nameAppend!=NULL)&&(strlen(nameAppend)>0)) {
    xx = myvprint("%s_RMS%s",name,nameAppend);
  } else { xx = myvprint("%s_RMS",name); }
  writer->addField( xx, 1 ); n++;
  free(xx);
  }
  */
  

  if (elog) {
    addNameAppendFieldAuto(name, "LOG", 1); n++;
  }
/*  if ((nameAppend!=NULL)&&(strlen(nameAppend)>0)) {
    xx = myvprint("%s_LOG%s",name,nameAppend);
  } else { xx = myvprint("%s_LOG",name); }
  writer->addField( xx, 1 ); n++;
  free(xx);
  }  */
  
  return n;
}


/*
int cEnergy::myFinaliseInstance()
{
  int ret=1;
  ret *= cVectorProcessor::myFinaliseInstance();
//.....
  return ret;
}
*/

// a derived class should override this method, in order to implement the actual processing
int cEnergy::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // do domething to data in *src, save result to *dst
  // NOTE: *src and *dst may be the same...
  if (Nsrc == 0) return 0;
  
  long i;
  double d=0.0;
  for (i=0; i<Nsrc; i++) {
    FLOAT_DMEM tmp=*(src++);
    d += tmp*tmp;
  }

  int n=0;
  if (erms) {
    dst[n++] = (FLOAT_DMEM)sqrt(d/(FLOAT_DMEM)Nsrc) * escaleRms + ebiasRms;
  }
  if (energy2) {
    dst[n++] = (FLOAT_DMEM)(d/(double)Nsrc) * escaleSquare + ebiasSquare;
  }

  //double minE = 8.674676e-019;

  if (elog) {
    if (!htkcompatible) {
      d /= (FLOAT_DMEM)Nsrc;
      if (d<minE) d = minE;
      dst[n++] = (FLOAT_DMEM)log(d) * escaleLog + ebiasLog;
    } else {
      d *= 32767.0*32767.0;
      if (d<=1.0) d = 1.0;
      dst[n++] = (FLOAT_DMEM)log(d) * escaleLog + ebiasLog;
    }
  }

  return n;
}

cEnergy::~cEnergy()
{
}

