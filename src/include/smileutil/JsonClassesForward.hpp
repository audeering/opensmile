/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#ifndef SMILEUTIL_JSONCLASSESFORWARD_HPP
#define SMILEUTIL_JSONCLASSESFORWARD_HPP

namespace smileutil {
namespace json {

    class JsonValue;
    class JsonAllocator;
    class JsonDocument;

    int safeJsonGetInt(const JsonValue &val, const char *name);
    
    template <typename T>
    T checkedJsonGet(const JsonDocument &jsonDoc, const char *varName);

}  // namespace json
}  // namespace smileutil


#endif // SMILEUTIL_JSONCLASSESFORWARD_HPP
