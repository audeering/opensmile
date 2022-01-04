/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

data framer
 
*/


#include <dspcore/framer.hpp>
//#include <math.h>

#define MODULE "cFramer"


SMILECOMPONENT_STATICS(cFramer)

SMILECOMPONENT_REGCOMP(cFramer)
{
  SMILECOMPONENT_REGCOMP_INIT


  scname = COMPONENT_NAME_CFRAMER;
  sdescription = COMPONENT_DESCRIPTION_CFRAMER;

  // we inherit cWinToVecProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cWinToVecProcessor")
  
  SMILECOMPONENT_IFNOTREGAGAIN( {} 

  )
  
  SMILECOMPONENT_MAKEINFO(cFramer);
}

SMILECOMPONENT_CREATE(cFramer)

//-----

cFramer::cFramer(const char *_name) :
  cWinToVecProcessor(_name)
{

}


// this must return the multiplier, i.e. the vector size returned for each input element (e.g. number of functionals, etc.)
int cFramer::getMultiplier()
{
  return frameSizeFrames;
}

// idxi is index of input element
// row is the input row
// y is the output vector (part) for the input row
int cFramer::doProcess(int idxi, cMatrix *row, FLOAT_DMEM*y)
{
  // copy row to matrix... simple memcpy here
  memcpy(y,row->data,row->nT*sizeof(FLOAT_DMEM));
  // return the number of components in y!!
  return row->nT;
}

cFramer::~cFramer()
{
}

