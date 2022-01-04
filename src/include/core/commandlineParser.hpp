/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#ifndef COMMANDLINE_PARSER_HPP
#define COMMANDLINE_PARSER_HPP

#include <core/smileCommon.hpp>
#include <string>
#include <vector>
#include <errno.h>

enum class eCmdlineOptType {
  Boolean,
  Int,
  Double,
  Str
};

struct sCmdlineOpt {
  std::string name;        // name of the option
  char abbreviation;       // single-character short-form of the option, set to '\0' for none
  std::string description; // optional description
  eCmdlineOptType type;    // data type of the option value
private:
  int dfltInt;             // int or boolean value, only valid if type == eCmdlineOptType::Int
  double dfltDouble;       // double value, only valid if type == eCmdlineOptType::Double
  std::string dfltStr;     // string value, only valid if type == eCmdlineOptType::Str
public:
  bool argMandatory;       // whether after the option name mandatory arguments follow
  bool isMandatory;        // whether the option needs to be specified
  bool isSet;              // whether the option was parsed
  int numArgs;             // number of arguments that followed the option. Always 0 or 1.

  sCmdlineOpt(const char *name, char abbreviation, const char *description, eCmdlineOptType type, bool argMandatory, bool isMandatory) :
    name(name), abbreviation(abbreviation), description(description != NULL ? description : ""), type(type), dfltInt(0), dfltDouble(0.0), dfltStr(),
    argMandatory(argMandatory), isMandatory(isMandatory), isSet(false), numArgs(0) {}

  bool getBoolean() const { return dfltInt != 0; }
  int getInt() const { return dfltInt; }
  double getDouble() const { return dfltDouble; }
  const char *getStr() const { return dfltStr.c_str(); }

  void setBoolean(bool value) { dfltInt = value ? 1 : 0; }
  void setInt(int value) { dfltInt = value; }
  void setDouble(double value) { dfltDouble = value; }
  void setStr(const char *value) { 
    if (value != NULL) 
      dfltStr = value;
    else
      dfltStr.clear();
  }
};

class cCommandlineParser {
  private:
    int argc;
    const char **argv;
    std::vector<sCmdlineOpt> options;

    sCmdlineOpt &addOpt(const char *name, char abbr, const char *description, eCmdlineOptType type, bool argMandatory, bool isMandatory);
    sCmdlineOpt *findOpt(const char *name); // returns pointer to option with the specified name or NULL if not found
    const sCmdlineOpt *findOpt(const char *name) const;    

  public:
    cCommandlineParser(int argc, const char **argv);

    void addBoolean(const char *name, char abbr, const char *description, bool dflt=false, bool argMandatory=false, bool isMandatory=false);
    void addInt(const char *name, char abbr, const char *description, int dflt=0, bool argMandatory=true, bool isMandatory=false);
    void addDouble(const char *name, char abbr, const char *description, double dflt=0.0, bool argMandatory=true, bool isMandatory=false);
    void addStr(const char *name, char abbr, const char *description, const char *dflt=NULL, bool argMandatory=true, bool isMandatory=false);

    void showUsage(const char *binname=NULL);

    int parse(bool ignoreDuplicates=false, bool ignoreUnknown=true); // return value: 0 on normal parse, -1 if usage was requested with '-h' 
                                                                     // (in this case the application should terminate after parse() has finished)

    bool getBoolean(const char *name) const;
    int getInt(const char *name) const;
    double getDouble(const char *name) const;
    const char *getStr(const char *name) const;

    bool isSet(const char *name) const;
    bool optionExists(const char *name) const;
};

#endif // COMMANDLINE_PARSER_HPP
