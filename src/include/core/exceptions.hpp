/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*

  SMILE exception classes
  ===========================

  Exception base class:
    cSMILException:
      + int getCode()
      + int getType()
        valid type constants:
          EX_GENERIC, EX_MEMORY, EX_COMPONENT, EX_IO, EX_CONFIG, EX_USER
      + getText()  : get human readable error message

  GenericException: (cSMILException) [Internal errors, usually not displayed or logged, only informative purpose]
    > GenericException(int code)
    - int getCode()

  MemoryException: (cSMILException) [Memory allocation errors, etc.]
    > MemoryException(int code)
    - int getCode()

  ----------------------------------------------------------------------------
  ComponentException: (cSMILException) [Any error in a smile component]
    > ComponentException([const char* module], const char* text)
    > ComponentException([const char* module], FMT(const char* fmt, ...))
    + getText()  : get full verbose text of error message
    - int getCode()


  ConfigException: (ComponentException) [Any configuration error]
    > ConfigException(int origin, [const char *module], const char* text)
    > ConfigException(int origin, [const char *module], FMT(const char* fmt, ...))
    + int getOrigin()
   origin: (exception raised when...)
          a) errors occur during config file parsing (origin = CE_PARSER)
          b) a module detects a bogus configuration, invalid value, etc.  (origin = CE_INVALID)
          c) mandatory parameters are not specified, etc. (origin = CE_MANAGER)

  IOException: (ComponentException) [Any I/O error, drive, network, sound, ...]
    > IOException([const char *module], [int code], const char* text, )
    > IOException([const char *module], [int code], FMT(const char* fmt, ...))

  UserException: (ComponentException) [Any other error, user defined...]
    > UserException([const char *module], int code, const char* text, )
    > UserException([const char *module], int code, FMT(const char* fmt, ...))

 */


#ifndef __EXCEPTIONS_HPP
#define __EXCEPTIONS_HPP

#include <core/smileCommon.hpp>

#define EX_GENERIC    0   // fast, integer based, exception
#define EX_COMPONENT  1
#define EX_IO         2
#define EX_CONFIG     3
#define EX_MEMORY     4
#define EX_USER       9

/* The base class for all exception classes in openSMILE */
class cSMILException {
  protected:
    int code;
    int type;

  public:
    cSMILException(int _type, int _code=0) { type=_type; code=_code; }
    int getCode() const { return code; }
    int getType() const { return type; }

    // get Error message as human readable text
    virtual char *getText() const;

    virtual ~cSMILException() {}
};

/* generic exception, only contains an error code */
class cGenericException : public cSMILException {
  public:
    cGenericException(int _code) : cSMILException(EX_GENERIC, _code) { }
    virtual ~cGenericException() {}
};

/* memory exception, only contains an error code */
class cMemoryException : public cSMILException {
  public:
    cMemoryException(int _code) : cSMILException(EX_MEMORY, _code) { }
    virtual ~cMemoryException() {}
};

#define OUT_OF_MEMORY throw(cMemoryException(0))


/* Component Exception, base class for all exceptions with a text message and module name */
class cComponentException : public cSMILException {
  private:
    mutable char *tmp;

  protected:
    char *text;
    const char *module;

  public:
    cComponentException(char *t, const char *m=NULL):
      cSMILException(EX_COMPONENT),
      tmp(NULL),
      text(NULL)
      { module=m; text=t; logException(); }
    cComponentException(int _type, char *t, const char *m=NULL):
      cSMILException(_type),
      tmp(NULL),
      text(NULL)
      { module=m; text=t; logException(); }

    virtual char *getText() const override;

    // automatically log the exception to smileLog component
    virtual void logException();

    virtual ~cComponentException() {
      if (tmp!=NULL) free(tmp);
      if (text != NULL) free(text);
    }
};

// throw a component exception
#define COMP_ERR(...)  throw(cComponentException(myvprint(__VA_ARGS__), MODULE))

#define CE_PARSER  1
#define CE_INVALID 2
#define CE_MANAGER 3

/*
 ConfigException:
   raised when:
          a) errors occur during config file parsing (origin = CE_PARSER)
          b) a module detects a bogus configuration, invalid value, etc.  (origin = CE_INVALID)
          c) mandatory parameters are not specified, etc. (origin = CE_MANAGER)
*/
class cConfigException : public cComponentException {
  private:
    int origin;

  public:
    cConfigException(int _origin, char *t, const char *m=NULL) :
      cComponentException(EX_CONFIG,t,m),
      origin(_origin)
      { }
    int getOrigin() const { return origin; }
    virtual ~cConfigException() {}
};

// throw config exceptions:
#define CONF_INVALID_ERR(...)  throw(cConfigException(CE_INVALID, myvprint(__VA_ARGS__), MODULE))
#define CONF_PARSER_ERR(...)  throw(cConfigException(CE_PARSER, myvprint(__VA_ARGS__), MODULE))
#define CONF_MANAGER_ERR(...)  throw(cConfigException(CE_MANAGER, myvprint(__VA_ARGS__), MODULE))

class cIOException : public cComponentException {
  public:
    cIOException(char *t, int _code=0, const char *m=NULL) :
      cComponentException(EX_IO,t,m)
      { code = _code; }
    virtual ~cIOException() {}
};

#define IO_ERR(...)  throw(cIOException(FMT(__VA_ARGS__),0,MODULE));

class cUserException : public cComponentException {
  public:
    cUserException(char *t, int _code, const char *m=NULL) :
      cComponentException(EX_USER,t,m)
     { code = _code; }
    virtual ~cUserException() {}
};

#define USER_ERR(CODE, ...)  throw(cUserException(FMT(__VA_ARGS__),CODE,MODULE));


/**** typedefs *****************************/

typedef cGenericException GenericException;
typedef cMemoryException MemoryException;

typedef cComponentException ComponentException;
typedef cConfigException ConfigException;
typedef cIOException IOException;
typedef cUserException UserException;


#endif  // __EXCEPTIONS_HPP

