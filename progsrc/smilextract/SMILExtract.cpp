/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

/*

This is the main commandline application

*/

#include <core/smileCommon.hpp>

#include <core/configManager.hpp>
#include <core/commandlineParser.hpp>
#include <core/componentManager.hpp>
#include <memory>

#define MODULE "SMILExtract"


/************** Ctrl+C signal handler **/
#include  <signal.h>

cComponentManager *cmanGlob = NULL;

void INThandler(int);
int ctrlc = 0;

void INThandler(int sig)
{
  signal(sig, SIG_IGN);
  if (cmanGlob != NULL) cmanGlob->requestAbort();
  signal(SIGINT, INThandler);
  ctrlc = 1;
}
/*******************************************/


int main(int argc, const char *argv[])
{
  try {

    smileCommon_fixLocaleEnUs();
    smileCommon_enableVTInWindowsConsole();

    // set up the smile logger
    cSmileLogger logger;
    logger.useForCurrentThread();
    logger.setLogLevel(1);
    logger.setConsoleOutput(true);

    // commandline parser:
    cCommandlineParser cmdline(argc,argv);
    cmdline.addStr("configfile", 'C', "Path to openSMILE config file", "smile.conf");
    cmdline.addInt("loglevel", 'l', "Verbosity level (0-9)", 2);
#ifdef DEBUG
    cmdline.addBoolean("debug", 'd', "Show debug messages", 0);
#endif
    cmdline.addInt("nticks", 't', "Number of ticks to process (-1 = infinite) (only works for single-thread processing, i.e. nThreads=1)", -1);
    cmdline.addBoolean("components", 'L', "Show component list", 0);
    cmdline.addStr("configHelp", 'H', "Show documentation of registered config types (if an argument is given, show only documentation for config types beginning with the name given in the argument)", NULL, 0);
    cmdline.addStr("configDflt", 0, "Show default config section templates for each config type (if an argument is given, show only documentation for config types beginning with the name given in the argument, OR for a list of components in conjunctions with the 'cfgFileTemplate' option enabled)", NULL, 0);
    cmdline.addBoolean("cfgFileTemplate", 0, "Print a complete template config file for a configuration containing the components specified in a comma separated string as argument to the 'configDflt' option", 0);
    cmdline.addBoolean("cfgFileDescriptions", 0, "Include description in config file templates.", 0);
    cmdline.addBoolean("ccmdHelp", 'c', "Show custom commandline option help (those specified in config file)", 0);
    cmdline.addBoolean("exportHelp", 0, "Print detailed documentation of registered config types in JSON format.", 0);
    cmdline.addStr("logfile", 0, "Set path of log file");
    cmdline.addBoolean("noconsoleoutput", 0, "Don't output any messages to the console (log file is not affected by this option)", 0);
    cmdline.addBoolean("appendLogfile", 0, "Append log messages to an existing logfile instead of overwriting the logfile at every start", 0);

    int help = 0;
    if (cmdline.parse() == -1) {
      logger.setLogLevel(0);
      help = 1;
    }
    if (argc <= 1) {
      printf("\nNo commandline options were given.\nPlease run 'SMILExtract -h' to see usage information!\n\n");
      return 10;
    }

    if (help == 1)
      return EXIT_SUCCESS;

    if (cmdline.isSet("logfile")) {
      logger.setLogFile(cmdline.getStr("logfile"),cmdline.getBoolean("appendLogfile"));
    } else {
      logger.setLogFile((const char *)NULL,0);
    }
    logger.setConsoleOutput(!cmdline.getBoolean("noconsoleoutput"));
    logger.setLogLevel(cmdline.getInt("loglevel"));
    SMILE_MSG(2,"openSMILE starting!");

#ifdef DEBUG 
    if (!cmdline.getBoolean("debug"))
      logger.setLogLevel(LOG_DEBUG, 0);
#endif

    SMILE_MSG(2,"config file is: %s", cmdline.getStr("configfile"));

    /* we declare componentManager before configManager because
       configManager must be deleted BEFORE componentManager
       (since component Manager unregisters plugin DLLs which might have allocated configTypes, etc.) */
    std::unique_ptr<cComponentManager> cMan;
    std::unique_ptr<cConfigManager> configManager;
    configManager = std::unique_ptr<cConfigManager>(new cConfigManager(&cmdline));
    cMan = std::unique_ptr<cComponentManager>(new cComponentManager(configManager.get(),componentlist));

    if (cmdline.isSet("configHelp")) {
      const char *selStr = cmdline.getStr("configHelp");
      configManager->printTypeHelp(1/*!!! -> 1*/,selStr,0);
      help = 1;
    }
    if (cmdline.isSet("configDflt")) {
      int fullMode = 0; 
      int wDescr = 0;
      if (cmdline.getBoolean("cfgFileTemplate")) fullMode=1;
      if (cmdline.getBoolean("cfgFileDescriptions")) wDescr=1;
      const char *selStr = cmdline.getStr("configDflt");
      configManager->printTypeDfltConfig(selStr,1,fullMode,wDescr);
      help = 1;
    }
    if (cmdline.getBoolean("exportHelp")) {
      cMan->exportComponentList();
      help = 1;
    }
    if (cmdline.getBoolean("components")) {
      cMan->printComponentList();
      help = 1;
    }

    if (help==1) {
      return EXIT_ERROR; 
    }

    // TODO: read config here and print ccmdHelp...
    // add the file config reader:
    cFileConfigReader * reader = new cFileConfigReader(
        cmdline.getStr("configfile"), -1, &cmdline);
    configManager->addReader(reader);
    configManager->readConfig();

    /* re-parse the command-line to include options created in the config file */
    cmdline.parse(true, false); // warn if unknown options are detected on the commandline
    if (cmdline.getBoolean("ccmdHelp")) {
      cmdline.showUsage();
      return EXIT_ERROR;
    }

    /* create all instances specified in the config file */
    cMan->createInstances(0); // 0 = do not read config (we already did that above..)

    /*
    MAIN TICK LOOP :
    */
    cmanGlob = cMan.get();
    signal(SIGINT, INThandler); // install Ctrl+C signal handler

    bool run = true;

    /* run single or multi-threaded, depending on componentManager config in config file */
    long long nTicks = 0;
    if (run) {
      nTicks = cMan->runMultiThreaded(cmdline.getInt("nticks"));
    }
  } catch (...) {
    return EXIT_ERROR; 
  } 

  if (ctrlc) return EXIT_CTRLC;
  return EXIT_SUCCESS;
}
