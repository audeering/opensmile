/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: cNullSink
----------------------------------
NULL sink: reads data vectors from data memory and destroys them without writing them anywhere
----------------------------------

11/2009  Written by Florian Eyben
*/


#include <core/nullSink.hpp>

#define MODULE "cNullSink"


SMILECOMPONENT_STATICS(cNullSink)

SMILECOMPONENT_REGCOMP(cNullSink)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CNULLSINK;
  sdescription = COMPONENT_DESCRIPTION_CNULLSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN( {} )

  SMILECOMPONENT_MAKEINFO(cNullSink);
}

SMILECOMPONENT_CREATE(cNullSink)

//-----

cNullSink::cNullSink(const char *_name) :
  cDataSink(_name)
{
}

eTickResult cNullSink::myTick(long long t)
{
  SMILE_IDBG(4,"tick # %i, reading value vector to NULL sink",t);
  cVector *vec= reader_->getNextFrame();
  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;
  
  nWritten_++;

  return TICK_SUCCESS;
}


cNullSink::~cNullSink()
{
}

