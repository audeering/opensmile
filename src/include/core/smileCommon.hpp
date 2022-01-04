/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*

Common defines, config, etc.

this file also contains openSMILE's small and dirty platform independence layer

*/


#ifndef __SMILE_COMMON_H
#define __SMILE_COMMON_H

#ifdef __WINDOWS
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#endif

#ifdef _DEBUG
#define DEBUG
#endif
#ifdef __DEBUG
#define DEBUG
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <float.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef __WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <core/smileTypes.h>

#ifdef __WINDOWS
#ifndef __MINGW32
// we specify parameters here to make the defines "function-like" so that 
// they can be circumvented, if necessary, as in https://stackoverflow.com/a/13420838
#ifndef strcasecmp
#define strcasecmp(s1, s2) _stricmp(s1, s2)
#endif
#ifndef strncasecmp
#define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
#endif
#endif
#endif

#ifdef __IOS__
#include <sys/types.h>
#endif // __IOS__

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#define NEWLINE "\n"

#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#define AUTO_EXCEPTION_LOGGING   1     // automatically log exceptions to smileLog global object

#ifndef __STATIC_LINK
#define SMILE_SUPPORT_PLUGINS
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

__inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}

#endif

#define EXIT_SUCCESS   0
#define EXIT_ERROR    -1
#define EXIT_CTRLC   -10

void smilePrintHeader();
int checkVectorFinite(FLOAT_DMEM *x, long N);

// realloc and initialize additional memory with zero,like calloc
void * crealloc(void *a, size_t size, size_t old_size);
#define MIN_LOG_STRLEN   255
char *myvprint(const char *fmt, ...);
#define FMT(...)   myvprint(__VA_ARGS__)
void * memdup(const void *in, size_t size);

#ifdef __WINDOWS
// for the windows MINGW32 environment, we have to implement some GNU library functions ourselves...
//void bzero(void *s, size_t n);
#define bzero(s,n) memset(s,0,n)
#define __HAVENT_GNULIBS
#ifndef _MSC_VER
void GetSystemTimeAsFileTime(FILETIME*);
#endif
#endif

#if defined(__HAVENT_GNULIBS) || defined(__ANDROID__)
// for the windows MINGW32 environment and Mac,
// we have to implement some GNU library functions ourselves...
long smile_getline(char **linePointer, size_t *n, FILE *stream);
long getstr (char **linePointer, size_t *n,
    FILE *stream, char eolCharacter, int offset);
#else
// we link to the glibc version
static ssize_t (*smile_getline)(char **, size_t *, FILE *) = getline;
#endif

long smile_getline_frombuffer(char **linePointer, size_t *n,
      char **buf, int *lenBuf);

void smileCommon_fixLocaleEnUs();

// On Windows, only version Windows 10 v1511 and later support VT100 escape sequences for colored console output
// and they have to be enabled programmatically. 
void smileCommon_enableVTInWindowsConsole();

#include <core/exceptions.hpp>
#include <core/smileLogger.hpp>
#include <smileutil/smileUtil.h>

#endif  // __SMILE_COMMON_H
