/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: contour smoother

smooth data contours by moving average filter

*/


#include <dspcore/contourSmoother.hpp>
#include <math.h>

#define MODULE "cContourSmoother"


SMILECOMPONENT_STATICS(cContourSmoother)

SMILECOMPONENT_REGCOMP(cContourSmoother)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CCONTOURSMOOTHER;
  sdescription = COMPONENT_DESCRIPTION_CCONTOURSMOOTHER;

  // we inherit cWindowProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cWindowProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("nameAppend", NULL, "sma");
    ct->setField("smaWin","The size of the moving average window. A larger window means more smoothing.",3);
    ct->setField("noZeroSma"," 1 = exclude frames where the element value is 0 from smoothing (i.e. 0 input will be 0 output, and zeros around non-zero values will not be included in the computation of the running average).",0);
    ct->setField("blocksize", NULL ,1);
  )
  SMILECOMPONENT_MAKEINFO(cContourSmoother);
}

SMILECOMPONENT_CREATE(cContourSmoother)

//-----

cContourSmoother::cContourSmoother(const char *_name) :
  cWindowProcessor(_name),
  smaWin(0)
{
}


void cContourSmoother::myFetchConfig()
{
  cWindowProcessor::myFetchConfig();
  
  noZeroSma = getInt("noZeroSma");
  SMILE_IDBG(2,"noZeroSma = %i",noZeroSma);

  smaWin = getInt("smaWin");
  
  if (smaWin < 1) {
    SMILE_IWRN(1,"smaWin must be >= 1 ! (setting to 1)");
    smaWin = 1;
  }
  if (smaWin % 2 == 0) {
    smaWin++;
    SMILE_IWRN(1,"smaWin must be an uneven number >= 1 ! (increasing smaWin by 1 -> smaWin=%i)",smaWin);
  }
  SMILE_IDBG(2,"smaWin = %i",smaWin);

  setWindow(smaWin/2,smaWin/2);
}

/*
int cContourSmoother::setupNamesForField(int i, const char*name, long nEl)
{
  char *tmp = myvprint("%s_de",name);
  writer->addField( name, nEl );
  return nEl;
}
*/

// order is the amount of memory (overlap) that will be present in _in
// buf will have nT timesteps, however also order negative indicies (i.e. you may access a negative array index up to -order!)
int cContourSmoother::processBuffer(cMatrix *_in, cMatrix *_out, int _pre, int _post )
{
  long n,w;
  FLOAT_DMEM *x=_in->data;
  FLOAT_DMEM *y=_out->data;

  if (noZeroSma) {
    for (n=0; n<_out->nT; n++) {
      if (x[n] != 0.0) {
        long N = 1;
        y[n] = x[n];
        for (w=1; w<=smaWin/2; w++) {
          if (x[n-w] != 0.0) { y[n] += x[n-w]; N++; }
          if (x[n+w] != 0.0) { y[n] += x[n+w]; N++; }
        }
        y[n] /= (FLOAT_DMEM)N;
      } else {
        y[n] = 0.0;
      }
    }
  } else {
    for (n=0; n<_out->nT; n++) {
      y[n] = x[n];
      for (w=1; w<=smaWin/2; w++) {
        y[n] += x[n-w];
        y[n] += x[n+w];
      }
      y[n] /= (FLOAT_DMEM)smaWin;
    }
  }

  return 1;
}


cContourSmoother::~cContourSmoother()
{
}

