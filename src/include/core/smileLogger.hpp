/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/******************************************************************************/

/*

openSMILE Message Logging


Log Levels for Messages:
0 . important messages, always displayed
1 . informative messages concerning functionality
2 . program status information (what we are currently doing, not very detailed though)
3 . more detailed status, report on success of operations
4 . component initialisation messages

Log Levels for Debug Messages:
0.  only very obvious information, status, etc.
1.
2.  warnings
3.  detailed information (not in internal loops)
4.
5.  detailed information from internal loops (e.g. every line from config file)
6.  extended details in internal loops

Log Levels for Errors:

Log Levels for Warnings:

*/


#ifndef __SMILE_LOGGER_HPP
#define __SMILE_LOGGER_HPP

#include <core/smileCommon.hpp>
#include <core/smileThread.hpp>
#include <functional>
#ifdef __ANDROID__
#include <android/log.h>
#endif

#define LOG_ALL     0
#define LOG_MESSAGE 1
#define LOG_WARNING 2
#define LOG_ERROR   3
#define LOG_DEBUG   4
#define LOG_PRINT   5

using SmileLogCallback = std::function<void (int type, int level, const char *text, const char *module)>;

class cSmileLogger {
  private:
    smileMutex logmsgMtx;
    char *logfile;
    FILE *logf;
    int stde;    // flag that indicates whether stderr output is enabled or not
    int silence; // if set, logger will NOT produce ANY output!
    int _enableLogPrint;  // set, if you want log messages ('print') to be duplicated to the logfile
    int ll_msg;
    int ll_wrn;
    int ll_err;
    int ll_dbg;
    bool coloredOutput;
    SmileLogCallback callback;

    void openLogfile(int append=0);
    void closeLogfile();

    // formatting of log message
    char *fmtLogMsg(int itype, char *t, int level, const char *m, bool colored = false);

    // write a log message to current logfile (if open)
    // optionally prepends the line with a date and timestamp
    void writeMsgToFile(const char *msg, bool printTimestamp);

    // print message to console, without a timestamp
    void printMsgToConsole(const char *msg);

    // main log message dispatcher
    void logMsg(int itype, char *s, int level, const char *m);    

  public:
    // loglevel is the OVERALL loglevel, logfile == NULL: output to stderr
    cSmileLogger(int _loglevel=0, const char *_logfile=NULL, int _append=0, int _stde=1); 
    cSmileLogger(int loglevel_msg, int loglevel_wrn, int loglevel_err, int loglevel_dbg, const char *_logfile=NULL, int _append=0, int _stde=1);
    ~cSmileLogger();

    void setLogLevel(int level);
    void setLogLevel(int _type, int level);
    void setLogFile(const char *file, int _append = 0);
    void setConsoleOutput(int enabled) { stde = enabled; }
    void setColoredOutput(bool enabled) { coloredOutput = enabled; }
    void setCallback(SmileLogCallback callback) { this->callback = callback; }

    int getLogLevel_msg() const { return ll_msg; }
    int getLogLevel_wrn() const { return ll_wrn; }
    int getLogLevel_err() const { return ll_err; }
    int getLogLevel_dbg() const { return ll_dbg; }
    
    void enableLogPrint() { _enableLogPrint = 1; } // enable printing of 'print' messages to log file (they are by default only written to the console)
    void muteLogger() { silence=1; }  // surpress all log messages
    void unmuteLogger() { silence=0; }  // back to normal logging

    void useForCurrentThread(); // sets the thread-local SMILE_LOG_GLOBAL variable to this instance

    /* similar to message, only that no formatting is performed and no extra information is printed
       the messages also do NOT go to the logfile by default (except if _enableLogPrint is set)
    */
		void log(int type, char *s, int level=0, const char *module=NULL)
		  { logMsg(type,s,level,module); }

		void print(char *s, int level=0)
		  { logMsg(LOG_PRINT,s,level,NULL); }

		void message(char *s, int level=0, const char *module=NULL)
		  { logMsg(LOG_MESSAGE,s,level,module); }

		void warning(char *s, int level=0, const char *module=NULL)
		  { logMsg(LOG_WARNING,s,level,module); }

		void error(char *s, int level=0, const char *module=NULL)
		  { logMsg(LOG_ERROR,s,level,module); }

    #ifdef DEBUG
    void debug(char *s, int level=0, const char *module=NULL)
      { logMsg(LOG_DEBUG,s,level,module); }
    #else
    void debug(char *s, int level=0, const char *module=NULL) 
      { if (s!=NULL) free(s); }
    #endif
};

// Global logger variable used by logging macros.
// This is a thread-local variable to support running multiple instances of openSMILE through SMILEapi and logging to instance-specific loggers.
// This may be NULL if accessed by a new thread for which cSmileLogger::useForCurrentThread has not been called yet.
#define SMILE_LOG_GLOBAL smileLog
extern thread_local cSmileLogger *SMILE_LOG_GLOBAL;

// The "I" logger functions (e.g. SMILE_IMSG) are meant to be used within cSmileComponent descendants. They will print the actual instance name instead of the component name
#ifdef SMILE_LOG_GLOBAL

#define SMILE_PRINT(...) { if (SMILE_LOG_GLOBAL != NULL) SMILE_LOG_GLOBAL->print(FMT(__VA_ARGS__)); }
#define SMILE_PRINTL(level, ...) { if (SMILE_LOG_GLOBAL != NULL && level <= SMILE_LOG_GLOBAL->getLogLevel_msg()) SMILE_LOG_GLOBAL->print(FMT(__VA_ARGS__), level); }
#define SMILE_MSG(level, ...) { if (SMILE_LOG_GLOBAL != NULL && level <= SMILE_LOG_GLOBAL->getLogLevel_msg()) SMILE_LOG_GLOBAL->message(FMT(__VA_ARGS__), level, MODULE); }
#define SMILE_IMSG(level, ...) { if (SMILE_LOG_GLOBAL != NULL && level <= SMILE_LOG_GLOBAL->getLogLevel_msg()) { char *__mm = FMT("instance '%s'",getInstName()); SMILE_LOG_GLOBAL->message(FMT(__VA_ARGS__), level, __mm); free(__mm); } }
#define SMILE_ERR(level, ...) { if (SMILE_LOG_GLOBAL != NULL && level <= SMILE_LOG_GLOBAL->getLogLevel_err()) SMILE_LOG_GLOBAL->error(FMT(__VA_ARGS__), level, MODULE); }
#define SMILE_IERR(level, ...) { if (SMILE_LOG_GLOBAL != NULL && level <= SMILE_LOG_GLOBAL->getLogLevel_err()) { char *__mm = FMT("instance '%s'",getInstName()); SMILE_LOG_GLOBAL->error(FMT(__VA_ARGS__), level, __mm); free(__mm); } }
#define SMILE_WRN(level, ...) { if (SMILE_LOG_GLOBAL != NULL && level <= SMILE_LOG_GLOBAL->getLogLevel_wrn()) SMILE_LOG_GLOBAL->warning(FMT(__VA_ARGS__), level, MODULE); }
#define SMILE_IWRN(level, ...) { if (SMILE_LOG_GLOBAL != NULL && level <= SMILE_LOG_GLOBAL->getLogLevel_wrn()) { char *__mm = FMT("instance '%s'",getInstName()); SMILE_LOG_GLOBAL->warning(FMT(__VA_ARGS__), level, __mm); free(__mm); } }

#ifdef DEBUG
#define SMILE_DBG(level, ...) { if (SMILE_LOG_GLOBAL != NULL && level <= SMILE_LOG_GLOBAL->getLogLevel_dbg()) SMILE_LOG_GLOBAL->debug(FMT(__VA_ARGS__), level, MODULE); }
#define SMILE_IDBG(level, ...) { if (SMILE_LOG_GLOBAL != NULL && level <= SMILE_LOG_GLOBAL->getLogLevel_dbg()) { char *__mm = FMT("instance '%s'",getInstName()); SMILE_LOG_GLOBAL->debug(FMT(__VA_ARGS__), level, __mm); free(__mm); } }
#else
#define SMILE_DBG(level, ...)
#define SMILE_IDBG(level, ...)
#endif // DEBUG

#else

#define SMILE_PRINT(...)
#define SMILE_PRINTL(level, ...)
#define SMILE_MSG(level, ...)
#define SMILE_IMSG(level, ...)
#define SMILE_ERR(level, ...)
#define SMILE_IERR(level, ...)
#define SMILE_WRN(level, ...)
#define SMILE_IWRN(level, ...)
#define SMILE_DBG(level, ...)
#define SMILE_IDBG(level, ...)

#endif // SMILE_LOG_GLOBAL

#endif // __SMILE_LOGGER_HPP
