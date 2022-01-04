/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

concatenates vectors from multiple levels and copy to another level

*/


#include <other/vectorConcat.hpp>

#define MODULE "cVectorConcat"

SMILECOMPONENT_STATICS(cVectorConcat)

SMILECOMPONENT_REGCOMP(cVectorConcat)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CVECTORCONCAT;
  sdescription = COMPONENT_DESCRIPTION_CVECTORCONCAT;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  
    SMILECOMPONENT_IFNOTREGAGAIN( {}
    //ct->setField("expandFields", "expand fields to single elements, i.e. each field in the output will correspond to exactly one element in the input [not yet implemented]", 0);
  )

  SMILECOMPONENT_MAKEINFO(cVectorConcat);
}

SMILECOMPONENT_CREATE(cVectorConcat)

//-----

cVectorConcat::cVectorConcat(const char *_name) :
  cVectorProcessor(_name)
{
}

int cVectorConcat::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  if (dst!=src)
    memcpy( dst, src,  MIN(Ndst,Nsrc)*sizeof(FLOAT_DMEM) );
  return 1;
}

cVectorConcat::~cVectorConcat()
{
}

