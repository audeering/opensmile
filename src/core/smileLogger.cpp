/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/******************************************************************************/

 /*

 openSMILE Message Logging

 */


#include <core/smileLogger.hpp>
#include <time.h>

// include android native logging functionality
#ifdef __ANDROID__
#include <android/log.h>
#endif  //  __ANDROID__

#ifdef __WINDOWS
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

// Global logger variable (used by exception classes for automatically logging...)
thread_local cSmileLogger *SMILE_LOG_GLOBAL = NULL;

#define MODULE "smileLogger"

/********************* class implementation ***********************************/

cSmileLogger::cSmileLogger(int _loglevel, const char * _logfile, int _append, int _stde) :
  stde(_stde),
  logf(NULL),
  silence(0),
  _enableLogPrint(1),
  callback(NULL)
{
  if (_logfile != NULL) {
    logfile = strdup(_logfile);
    openLogfile(_append);
  } else { 
    logfile = NULL; 
  }

  if (_loglevel >= 0) {
    ll_msg = ll_wrn = ll_err = ll_dbg = _loglevel;
  } else {
    ll_msg = ll_wrn = ll_err = ll_dbg = 0;
  }

  smileMutexCreate(logmsgMtx);

  // only output colored log messages if the platform supports it
  // currently only enabled for Linux and Windows 10 version 1511 or later (where ENABLE_VIRTUAL_TERMINAL_PROCESSING is defined)
#if (!defined(__WINDOWS) || defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING)) && !defined(__ANDROID__) && !defined(__IOS__)
  coloredOutput = true;
#else
  coloredOutput = false;
#endif

  // only output colored log messages if stderr is not piped to a file or process
  coloredOutput &= isatty(fileno(stderr)) != 0;
}

cSmileLogger::cSmileLogger(int loglevel_msg, int loglevel_wrn, int loglevel_err, int loglevel_dbg, const char *_logfile, int _append, int _stde) :
  cSmileLogger(0, _logfile, _append, _stde)
{
  ll_msg = loglevel_msg;
  ll_wrn = loglevel_wrn;
  ll_err = loglevel_err;
  ll_dbg = loglevel_dbg;
}

cSmileLogger::~cSmileLogger()
{
  // if this instance has been set as the global logger, we have to unset it
  if (SMILE_LOG_GLOBAL == this)
    SMILE_LOG_GLOBAL = NULL;

  smileMutexLock(logmsgMtx);
  closeLogfile();
  if (logfile != NULL) free(logfile);
  smileMutexUnlock(logmsgMtx);
  smileMutexDestroy(logmsgMtx);
}

/* opens the logfile */
void cSmileLogger::openLogfile(int append)
{
  if (logfile == NULL) return;
  if (logf) { fclose(logf); logf=NULL; }
  if (append) {
    logf = fopen(logfile,"a");
  } else {
    logf = fopen(logfile,"w");
  }
  if (logf == NULL) {
    throw IOException(FMT("cannot open logfile for writing!"),0,MODULE);
  }
}

void cSmileLogger::closeLogfile()
{
  if (logf != NULL) { fclose(logf); logf = NULL; }
}

void cSmileLogger::setLogLevel(int _type, int level)
{
  switch(_type) {
    case LOG_ALL:
      ll_msg = ll_wrn = ll_err = ll_dbg = level;
      break;
    case LOG_MESSAGE: ll_msg = level; break;
    case LOG_WARNING: ll_wrn = level; break;
    case LOG_ERROR:   ll_err = level; break;
    case LOG_DEBUG:   ll_dbg = level; break;
    default:
      throw ComponentException(FMT("invalid log level type (%i) specified in call to setLogLevel",level),MODULE);
  }
}

void cSmileLogger::setLogLevel(int level)
{
  setLogLevel(LOG_ALL,level);
}

void cSmileLogger::setLogFile(const char *file, int _append)
{
  if (file != NULL) {
    if (logfile) {
      free(logfile); logfile = NULL;
    }
    logfile = strdup(file);
    openLogfile(_append);
  }
}

// formatting of log message
char *cSmileLogger::fmtLogMsg(int itype, char *t, int level, const char *m, bool colored)
{
  if (t == NULL) return NULL;
  if (itype==LOG_PRINT) {
    return myvprint("%s",t);
  } else {
    static const char *types[] = { "MSG", "WRN", "ERR", "DBG" };
    static const char *typesColored[] = {
      "\033[1;34mMSG\033[0m", 
      "\033[1;33mWRN\033[0m", 
      "\033[1;31mERR\033[0m",     
      "\033[1;37mDBG\033[0m"
    };
    if (!colored) {
      if (m!=NULL) {
        return myvprint("(%s) [%i] %s: %s",types[itype-1],level,m,t);
      } else {
        return myvprint("(%s) [%i]: %s",types[itype-1],level,t);
      }
    } else {
      if (m!=NULL) {
        return myvprint("(%s) [%i] \033[1;37m%s\033[0m: %s",typesColored[itype-1],level,m,t);
      } else {
        return myvprint("(%s) [%i]: %s",typesColored[itype-1],level,t);
      }
    }
  }
}

// main log message dispatcher
void cSmileLogger::logMsg(int itype, char *s, int level, const char *m)
{
  if (silence) return;

  // check loglevel and type
  if (itype < 1 || itype > 5) return;
  if ((itype == LOG_PRINT && level > ll_msg) ||
      (itype == LOG_MESSAGE && level > ll_msg) ||
      (itype == LOG_ERROR && level > ll_err) ||
      (itype == LOG_WARNING && level > ll_wrn) ||
      (itype == LOG_DEBUG && level > ll_dbg)) {
    // ignore the message since the log level is below the level of the message
    free(s);
    return;
  }

  // trim any newline character from the end of the message
  if (*s != '\0' && s[strlen(s) - 1] == '\n') {
    s[strlen(s) - 1] = '\0';
  }

  smileMutexLock(logmsgMtx);

  if (logf != NULL || stde) {
    // format log message
    char *msg = fmtLogMsg(itype,s,level,m);

    // write to file
    if (logf != NULL) {
      if (itype != LOG_PRINT || _enableLogPrint) {
        bool printTimestamp = itype != LOG_PRINT;
        writeMsgToFile(msg, printTimestamp);
      }
    }
    
    // print to console
    if (stde) {
      if (coloredOutput) {
        char *coloredMsg = fmtLogMsg(itype,s,level,m,true);
        printMsgToConsole(coloredMsg);
        free(coloredMsg);
      } else {
        printMsgToConsole(msg);
      }
    }

    if (msg != NULL) free(msg);
  }

  // invoke log callback function
  if (callback != NULL) {
    callback(itype, level, s, m);
  }

  free(s);

  smileMutexUnlock(logmsgMtx);
}

// print message to console, without a timestamp
void cSmileLogger::printMsgToConsole(const char *msg)
{
  if (msg != NULL) {
#if defined(__ANDROID__) && !defined(__STATIC_LINK)
    __android_log_print(ANDROID_LOG_INFO, "opensmile", "%s", msg);
#else
    fprintf(stderr,"%s\n",msg);
    fflush(stderr);
#endif
  }
}

// write a log message to current logfile (if open)
// optionally prepends the line with a date and timestamp
void cSmileLogger::writeMsgToFile(const char *msg, bool printTimestamp)
{
  if (printTimestamp) {
    // date string
    time_t t;
    time(&t);
    struct tm *ti;
    ti = localtime(&t);
    fprintf(logf,"[ %.2i.%.2i.%.4i - %.2i:%.2i:%.2i ]\n    ",
      ti->tm_mday, ti->tm_mon+1, ti->tm_year+1900,
      ti->tm_hour, ti->tm_min, ti->tm_sec
    );
  }
  // log message
  fprintf(logf,"%s\n",msg);
  fflush(logf);
}

void cSmileLogger::useForCurrentThread()
{
  // it is correct to not delete the previous global logger here because it is not owned by SMILE_LOG_GLOBAL
  SMILE_LOG_GLOBAL = this;
}