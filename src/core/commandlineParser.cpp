/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*
 openSMILE commandline parser


*/


#include <core/commandlineParser.hpp>

#define MODULE "commandlineParser"

cCommandlineParser::cCommandlineParser(int argc, const char **argv) :
  argc(argc), argv(argv)
{
}

sCmdlineOpt &cCommandlineParser::addOpt(const char *name, char abbr, const char *description, eCmdlineOptType type, bool argMandatory, bool isMandatory)
{
  if (name == NULL)
    COMP_ERR("addOpt: cannot add commandlineParser option with name==NULL!");
  if (strcmp(name, "h") == 0 || abbr == 'h')
    COMP_ERR("option -h is reserved for show usage internally! please choose another name in your code! sorry..");

  options.emplace_back(name, abbr, description, type, argMandatory, isMandatory);
  return options.back();
}

void cCommandlineParser::addBoolean(const char *name, char abbr, const char *description, bool dflt, bool argMandatory, bool isMandatory)
{
  sCmdlineOpt &opt = addOpt(name, abbr, description, eCmdlineOptType::Boolean, argMandatory, isMandatory);
  opt.setBoolean(dflt);
}

void cCommandlineParser::addInt(const char *name, char abbr, const char *description, int dflt, bool argMandatory, bool isMandatory)
{
  sCmdlineOpt &opt = addOpt(name, abbr, description, eCmdlineOptType::Int, argMandatory, isMandatory);
  opt.setInt(dflt);
  opt.setDouble(dflt); // we also set it as double in case someone tries to access the value as such
}

void cCommandlineParser::addDouble(const char *name, char abbr, const char *description, double dflt, bool argMandatory, bool isMandatory)
{
  sCmdlineOpt &opt = addOpt(name, abbr, description, eCmdlineOptType::Double, argMandatory, isMandatory);
  opt.setDouble(dflt);
  opt.setInt((int)dflt); // we also set it as int in case someone tries to access the value as such
}

void cCommandlineParser::addStr(const char *name, char abbr, const char *description, const char *dflt, bool argMandatory, bool isMandatory)
{
  sCmdlineOpt &opt = addOpt(name, abbr, description, eCmdlineOptType::Str, argMandatory, isMandatory);
  opt.setStr(dflt);
}

sCmdlineOpt *cCommandlineParser::findOpt(const char *name)
{
  if (name == NULL)
    return NULL;

  // search by name
  for (auto &opt : options) {
    if (opt.name == name)
      return &opt;
  }

  // search by abbreviation
  if (strlen(name) == 1) {
    char c = name[0];
    for (auto &opt : options) {
      if (opt.abbreviation == c)
        return &opt;
    }
  }

  return NULL;
}

const sCmdlineOpt *cCommandlineParser::findOpt(const char *name) const
{
  return const_cast<cCommandlineParser*>(this)->findOpt(name);
}

bool cCommandlineParser::getBoolean(const char *name) const
{
  const sCmdlineOpt *opt = findOpt(name);
  if (opt == NULL) {
    COMP_ERR("boolean commandline argument '%s' not found!", name);
  }
  if (opt->type != eCmdlineOptType::Boolean) { 
    COMP_ERR("requested commandline argument '%s' is not of type boolean!", name);
  }
  return opt->getBoolean();
}

int cCommandlineParser::getInt(const char *name) const
{
  const sCmdlineOpt *opt = findOpt(name);
  if (opt == NULL) {
    COMP_ERR("int commandline argument '%s' not found!", name);
  }
  if (opt->type != eCmdlineOptType::Int) {
    COMP_ERR("requested commandline argument '%s' is not of type int!", name);
  }
  return opt->getInt();
}

double cCommandlineParser::getDouble(const char *name) const
{
  const sCmdlineOpt *opt = findOpt(name);
  if (opt == NULL) {
    COMP_ERR("double commandline argument '%s' not found!", name);
  }
  if (opt->type != eCmdlineOptType::Double) {
    COMP_ERR("requested commandline argument '%s' is not of type double!", name);
  }
  return opt->getDouble();
}

const char * cCommandlineParser::getStr(const char *name) const
{
  const sCmdlineOpt *opt = findOpt(name);
  if (opt == NULL) {
    COMP_ERR("string commandline argument '%s' not found!", name);
  }
  if (opt->type != eCmdlineOptType::Str) {
    COMP_ERR("requested commandline argument '%s' is not of type string!", name);
  }
  return opt->getStr();
}

bool cCommandlineParser::isSet(const char *name) const
{
  const sCmdlineOpt *opt = findOpt(name);
  return opt != NULL && opt->isSet;
}

bool cCommandlineParser::optionExists(const char *name) const {
  return findOpt(name) != NULL;
}

int cCommandlineParser::parse(bool ignoreDuplicates, bool ignoreUnknown)
{
  bool showusage = false;

  // foreach argv element with(--) or (-), find the thing in opt
  SMILE_DBG(3,"about to parse %i commandline options ",argc-1);
  for (int i = 1; i < argc; i++) {
    SMILE_DBG(5,"parsing option %i : '%s'",i,argv[i]);

    if ((unsigned char)argv[i][0] > 127) {
      SMILE_ERR(0, "Please don't be lazy and copy & paste the commandlines from the SMILE book PDF. Your commandline contains invalid ASCII characters (probably incorrect '-'es) as a result of this and might not be parsed correctly! Type the commandline from scratch to avoid errors.");
      SMILE_ERR(0, "The offending argument is: '%s'", argv[i]);
      COMP_ERR("Parse error.");
    }

    // skip if it's no option, i.e. doesn't start with a hyphen
    if (argv[i][0] != '-') 
      continue;

    // usage (-h)
    if (strcmp(argv[i], "-h") == 0) {
      showusage = 1;
      continue;
    }

    const char *o = argv[i]+1;

    // skip second hyphen if present
    if (o[0] == '-') o++;

    sCmdlineOpt *opt = findOpt(o);

    if (opt == NULL) { // not found!
      if (!ignoreUnknown) {
        SMILE_ERR(0,"parse: unknown option '%s' on commandline!",argv[i]);
      }
      continue;
    }

    if (opt->isSet) { // option specified twice...!
      if (!ignoreDuplicates) {  // if ignoreDuplicates, we suppress this error message
        SMILE_ERR(0,"duplicate option '%s' on commandline (ignoring duplicate!) ",o);
      }
      if (opt->numArgs > 0) {
        i += opt->numArgs;
      }
      continue;
    }

    // now look for value....
    if (i+1 < argc) {
      bool optCand = false;
      if (argv[i+1][0] == '-')
        optCand = true;  // argument might be an option...
      
      // parse value according to type...
      switch (opt->type) {
        case eCmdlineOptType::Boolean: {
          if (!optCand) {  // explicit value given
            i++; // next parameter of commandline
            if (argv[i][0] == '0' || argv[i][0] == 'N' || argv[i][0] == 'n' || argv[i][0] == 'F' || argv[i][0] == 'f') {
              opt->setBoolean(false);
            } else if (argv[i][0] == '1' || argv[i][0] == 'Y' || argv[i][0] == 'y' || argv[i][0] == 'T' || argv[i][0] == 't') {
              opt->setBoolean(true);
            } else {
              COMP_ERR("invalid value specified for commandline option '%s', expected 0 or 1",o);
            }
            opt->isSet = true;
            opt->numArgs = 1;
          } else {
            if (!opt->getBoolean())
              opt->setBoolean(true);  // invert boolean default, if option is given
            else {
              SMILE_WRN(4, "setting boolean option '%s' inverts default (true) to false.", argv[i]);
              opt->setBoolean(false);
            }
            opt->isSet = true;
            opt->numArgs = 0;
          }          
          break;
        }
        case eCmdlineOptType::Int: {
          errno = 0;
          char *ep;
          // ep will point to the char after the last char that could be parsed successfully,
          // i.e. if *ep is not the null-terminating char, the string was not parsed to the end
          int v = strtol(argv[i+1], &ep, 10);
          if (optCand && (ep == argv[i+1] || *ep != '\0')) { // option... no value given
            if (opt->argMandatory) {
              COMP_ERR("option '%s' requires an argument!",o);
            } else {
              opt->isSet = true;
              opt->numArgs = 0;
            }
          } else {
            if (ep == argv[i+1] || *ep != '\0') COMP_ERR("invalid value specified for commandline option '%s'",o);
            if (errno == ERANGE) COMP_ERR("value specified for commandline option '%s' is out of range",o);
            opt->setInt(v);
            opt->isSet = true;
            opt->numArgs = 1;
            i++; // next parameter of commandline
          }
          break;
        }
        case eCmdlineOptType::Double: {
          errno = 0;
          char *ep;
          // ep will point to the char after the last char that could be parsed successfully,
          // i.e. if *ep is not the null-terminating char, the string was not parsed to the end
          double vd = strtod(argv[i+1], &ep);
          if (optCand && (ep == argv[i+1] || *ep != '\0')) { // option... no value given
            if (opt->argMandatory) {
              COMP_ERR("option '%s' requires an argument!",o);
            } else {
              opt->isSet = true;
              opt->numArgs = 0;
            }
          } else {
            if (ep == argv[i+1] || *ep != '\0') COMP_ERR("invalid value specified for commandline option '%s'",o);
            if (errno == ERANGE) COMP_ERR("value specified for commandline option '%s' is out of range",o);
            opt->setDouble(vd);
            opt->isSet = true;
            opt->numArgs = 1;
            i++; // next parameter of commandline
          }
          break;
        }
        case eCmdlineOptType::Str: {
          if (optCand) {  // bad situation... look for option
            const char *o2 = argv[i+1]+1;
            if (o2[0] == '-') o2++;
            const sCmdlineOpt *opt2 = findOpt(o2);
            if (opt2 != NULL) { // next option
              if (opt->argMandatory) {
                COMP_ERR("option '%s' requires an argument!",o);
              } else {
                opt->isSet = true;
                opt->numArgs = 0;
              }
            } else { // string value
              i++; // next parameter of commandline
              opt->setStr(argv[i]);
              opt->isSet = true;
              opt->numArgs = 1;
            }
          } else { // string value
            i++; // next parameter of commandline
            opt->setStr(argv[i]);
            opt->isSet = true;
            opt->numArgs = 1;
          }
          break;
        }
        default:
          COMP_ERR("unknown option type (%i) encountered... this actually cannot be!",opt->type);
      }
    } else {  // no value... check if one is required... 
      if (opt->argMandatory) {
        COMP_ERR("option '%s' requires an argument!",o);
      } else {
        // process boolean option, if boolean, else ignore...
        if (opt->type == eCmdlineOptType::Boolean) {
          if (!opt->getBoolean()) {
            opt->setBoolean(true);  // invert boolean default, if option is given
          } else {
            SMILE_WRN(4, "setting boolean option '%s' inverts default (true) to false.", argv[i]);
            opt->setBoolean(false);
          }
        }
        opt->isSet = true;
        opt->numArgs = 0;
      }
    }    
  }

  if (showusage) {
    showUsage(argc >= 1 ? argv[0] : NULL);
    return -1;
  }
  
  // now check if all mandatory parameters have been specified...
  bool paramMissing = false;
  for (auto &opt : options) {
    if (!opt.isSet && opt.isMandatory) { 
      SMILE_ERR(0,"mandatory commandline option '%s' was not specified!",opt.name.c_str()); 
      paramMissing = true;
    }
  }
  if (paramMissing) {
    showUsage(argc >= 1 ? argv[0] : NULL);
    SMILE_ERR(0,"missing options on the commandline!");
    return -1;
  }
  
  return 0;
}

void cCommandlineParser::showUsage(const char *binname)
{
  smilePrintHeader();

  if (binname == NULL) {
    SMILE_PRINT("Usage: SMILExtract [-option (value)] ...");
  } else {
    SMILE_PRINT("Usage: %s [-option (value)] ...", binname);
  }

  SMILE_PRINT(" ");
  SMILE_PRINT(" -h  Show this usage information");
  SMILE_PRINT(" ");

  for (int i = 0; i < options.size(); i++) {
    const char *placeholder;

    switch (options[i].type) {
      case eCmdlineOptType::Boolean:
        placeholder = options[i].argMandatory ? "<boolean 0/1>" : "[boolean 0/1]";
        break;
      case eCmdlineOptType::Int:
        placeholder = options[i].argMandatory ? "<integer>" : "[integer]";
        break;
      case eCmdlineOptType::Double:
        placeholder = options[i].argMandatory ? "<float>" : "[float]";
        break;
      case eCmdlineOptType::Str:
        placeholder = options[i].argMandatory ? "<string>" : "[string]";
        break;
      default:
        COMP_ERR("showUsage: unknown option type encountered! (type=%i for opt # %i)",options[i].type,i);
    }

    if (options[i].abbreviation != '\0') {
      SMILE_PRINT(" -%c, -%s %s",options[i].abbreviation,options[i].name.c_str(),placeholder);
    } else {
      SMILE_PRINT(" -%s %s",options[i].name.c_str(),placeholder);
    }

    if (!options[i].description.empty()) {
      SMILE_PRINT("     %s",options[i].description.c_str());
    }

    switch (options[i].type) {
      case eCmdlineOptType::Boolean:
        SMILE_PRINT("     (default: %i)",options[i].getBoolean() ? 1 : 0);
        break;
      case eCmdlineOptType::Int:
        SMILE_PRINT("     (default: %i)",options[i].getInt());
        break;
      case eCmdlineOptType::Double:
        SMILE_PRINT("     (default: %f)",options[i].getDouble());
        break;
      case eCmdlineOptType::Str:
        SMILE_PRINT("     (default: '%s')",options[i].getStr());
        break;
      default:
        COMP_ERR("showUsage: unknown option type encountered! (type=%i for opt # %i)",options[i].type,i);
    }

    SMILE_PRINT(" ");
  }
}

