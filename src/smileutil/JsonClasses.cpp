/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#include <smileutil/JsonClasses.hpp>
#include <stdexcept>
#include <string>

namespace smileutil {
namespace json {

    int safeJsonGetInt(const JsonValue &val, const char *name)
    {
        if (val->HasMember(name))
            return (*val)[name].GetInt();
        else
            return 0;
    }

    template <>
    bool checkedJsonGet<bool>(const JsonDocument &jsonDoc, const char *varName)
    {
        if (!jsonDoc->HasMember(varName) || !(*jsonDoc)[varName].IsBool())
            throw std::runtime_error(std::string("Variable '") + varName + "' is missing or has the wrong type");

        return (*jsonDoc)[varName].GetBool();
    }

    template <>
    int checkedJsonGet<int>(const JsonDocument &jsonDoc, const char *varName)
    {
        if (!jsonDoc->HasMember(varName) || !(*jsonDoc)[varName].IsInt())
            throw std::runtime_error(std::string("Variable '") + varName + "' is missing or has the wrong type");

        return (*jsonDoc)[varName].GetInt();
    }

    template <>
    double checkedJsonGet<double>(const JsonDocument &jsonDoc, const char *varName)
    {
        if (!jsonDoc->HasMember(varName) || (!(*jsonDoc)[varName].IsDouble() && !(*jsonDoc)[varName].IsInt()))
            throw std::runtime_error(std::string("Variable '") + varName + "' is missing or has the wrong type");

        if ((*jsonDoc)[varName].IsDouble())
            return (*jsonDoc)[varName].GetDouble();
        else
            return (double)(*jsonDoc)[varName].GetInt();
    }

    template <>
    float checkedJsonGet<float>(const JsonDocument &jsonDoc, const char *varName)
    {
        return (float)checkedJsonGet<double>(jsonDoc, varName);
    }

}  // namespace json
}  // namespace smileutil

