/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#include <core/exceptions.hpp>
#include <core/smileCommon.hpp>

char * cSMILException::getText() const {
  const char *etype;
  switch (type) {
    case EX_GENERIC: etype = ""; break;
    case EX_MEMORY: etype = "Memory"; break;
    default: etype = "Unknown";
  }
  return myvprint("%s ERROR : code = %i", etype, code);
}

char * cComponentException::getText() const {
  if (text != NULL) {
    const char *etype;
    switch (type) {
      case EX_COMPONENT: etype = "Component"; break;
      case EX_IO: etype = "I/O"; break;
      case EX_CONFIG: etype = "Config"; break;
      case EX_USER: etype = ""; break;
      default: etype = "Unknown"; break;
    }
    if (module != NULL) {
      tmp = myvprint("%s Exception in %s : %s [code = %i]", etype, module, text, code);
    }
    else {
      tmp = myvprint("%s Exception : %s [code = %i]", etype, text, code);
    }
  }
  return tmp;
}

void cComponentException::logException() {
#ifdef AUTO_EXCEPTION_LOGGING
  if (SMILE_LOG_GLOBAL != NULL) {
    SMILE_LOG_GLOBAL->error(text, 1, module);
  }
  text = NULL;
#endif
}
