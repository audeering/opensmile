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


#ifndef __CNULLSINK_HPP
#define __CNULLSINK_HPP

#include <core/smileCommon.hpp>
#include <core/dataSink.hpp>

#define COMPONENT_DESCRIPTION_CNULLSINK "This component reads data vectors from the data memory and 'destroys' them, i.e. does not write them anywhere. This component has no configuration options."
#define COMPONENT_NAME_CNULLSINK "cNullSink"

class cNullSink : public cDataSink {
  private:
    //int lag;    

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual eTickResult myTick(long long t) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cNullSink(const char *_name);
    virtual ~cNullSink();
};




#endif // __CNULLSINK_HPP
