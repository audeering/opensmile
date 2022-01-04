/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#ifndef __CSMILEUTILBASE_HPP
#define __CSMILEUTILBASE_HPP

#include <core/smileCommon.hpp>
#include <core/smileLogger.hpp>

class cSmileUtilBase {
private:
  const char * instname_;
public:
  cSmileUtilBase() : instname_(NULL) {}
  void setInstName(const char *name) {
    instname_ = name;
  }
  const char * getInstName() {
    return instname_;
  }
};

#endif  // __CSMILEUTILBASE_HPP
