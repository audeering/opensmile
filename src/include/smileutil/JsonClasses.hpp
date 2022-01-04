/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#ifndef SMILEUTIL_JSONCLASSES_HPP
#define SMILEUTIL_JSONCLASSES_HPP

#include <smileutil/JsonClassesForward.hpp>
#define RAPIDJSON_HAS_STDSTRING 1 
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/reader.h>


namespace smileutil {
namespace json {
    class JsonValue
    {
    private:
        mutable rapidjson::Value *_p;

    public:
        JsonValue()
            : _p(NULL)
        {
        }

        JsonValue(rapidjson::Value *p)
            : _p(p)
        {
        }

        bool isValid() const
        {
            return (_p != NULL);
        }

        rapidjson::Value* operator-> () const
        {
            return _p;
        }

        rapidjson::Value& operator* () const
        {
            return *_p;
        }
    };

    class JsonAllocator
    {
    private:
        mutable rapidjson::MemoryPoolAllocator<> *_p;

    public:
        JsonAllocator(rapidjson::MemoryPoolAllocator<> *p)
            : _p(p)
        {
        }

        rapidjson::MemoryPoolAllocator<>* operator-> () const
        {
            return _p;
        }

        operator rapidjson::MemoryPoolAllocator<>& () const
        {
            return *_p;
        }
    };

    class JsonDocument
    {
    private:
        mutable rapidjson::Document *_p;

    public:
        JsonDocument(rapidjson::Document *p)
            : _p(p)
        {
        }

        JsonDocument(rapidjson::Document &p)
            : _p(&p)
        {
        }

        rapidjson::Document* operator-> () const
        {
            return _p;
        }

        rapidjson::Document& operator* () const
        {
            return *_p;
        }

        operator rapidjson::Document& () const
        {
            return *_p;
        }
    };

}  // namespace json
}  // namespace smileutil


#endif // SMILEUTIL_JSONCLASSES_HPP
