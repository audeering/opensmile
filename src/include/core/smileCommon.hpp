/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*

Common defines, config, etc.

this file also contains openSMILE's small and dirty platform independence layer 
and thread implementations

*/


#ifndef __SMILE_COMMON_H
#define __SMILE_COMMON_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __VISTA
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#else
 // NT & XP compatibility
#define WINVER 0x0400
#define _WIN32_WINNT 0x0400
#endif

#ifdef HAVE_NET
#ifdef __WINDOWS
#include <winsock2.h>
#endif
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
#ifndef finite
#define finite(x) _finite(x)
#endif
#endif
#endif

#ifdef __IOS__
#include <sys/types.h>
#ifndef finite
#define finite(x) isfinite(x)
#endif
#endif // __IOS__

#include <locale.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#define AUTO_EXCEPTION_LOGGING   1     // automatically log exceptions to smileLog global object

#ifndef __STATIC_LINK
#define SMILE_SUPPORT_PLUGINS
#endif

#if !defined(__STATIC_LINK) && defined(_MSC_VER) // Visual Studio specific macro
  #ifdef BUILDING_DLL
    #define DLLEXPORT __declspec(dllexport)
    #define class class __declspec(dllexport)
  #else
    #define DLLEXPORT __declspec(dllimport)
    #define class class __declspec(dllimport)
  #endif
  #define DLLLOCAL 
#else 
    #define DLLEXPORT 
    #define DLLLOCAL  
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

#include <core/versionInfo.hpp>

#define EXIT_SUCCESS   0
#define EXIT_ERROR    -1
#define EXIT_CTRLC   -10

DLLEXPORT void smilePrintHeader();
DLLEXPORT int checkVectorFinite(FLOAT_DMEM *x, long N);

// realloc and initialize additional memory with zero,like calloc
DLLEXPORT void * crealloc(void *a, size_t size, size_t old_size);
#define MIN_LOG_STRLEN   255
DLLEXPORT char *myvprint(const char *fmt, ...);
#define FMT(...)   myvprint(__VA_ARGS__)
DLLEXPORT void * memdup(const void *in, size_t size);

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
DLLEXPORT long smile_getline(char **linePointer, size_t *n, FILE *stream);
DLLEXPORT long getstr (char **linePointer, size_t *n,
    FILE *stream, char eolCharacter, int offset);
#else
// we link to the glibc version
static ssize_t (*smile_getline)(char **, size_t *, FILE *) = getline;
#endif

long smile_getline_frombuffer(char **linePointer, size_t *n,
      char **buf, int *lenBuf);

#ifndef __WINDOWS
#include <sys/time.h>
#endif

#ifndef INFINITY
#define INFINITY (std::numeric_limits<double>::infinity())
#endif

#ifdef __MACOS

#include <time.h>
//#include <sys/time.h>

DLLEXPORT int clock_gettime(int clock_id, struct timespec *tp);
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#endif

// THREADS & MUTEXES:

//#include <windows.h>
//#include <process.h>
//#ifdef HAVE_PTHREAD
#if defined(HAVE_PTHREAD) || defined(__ANDROID_API__) || defined(__IOS__)

#include <pthread.h>

#define SMILE_THREAD_RETVAL void *
#define SMILE_THREAD_RET return NULL

#define smileThread  pthread_t
#define smileThreadCreate(THRD,T_MAIN,ARG) (pthread_create(&(THRD), NULL, T_MAIN, ARG)==0)
#define smileThreadJoinWret(THRD,RET) pthread_join(THRD,&RET)
#define smileThreadJoin(THRD) pthread_join(THRD,NULL)

#define smileMutex pthread_mutex_t
#define smileMutexInitVar(mtx) mtx = PTHREAD_MUTEX_INITIALIZER
#define smileMutexCreate(mtx) pthread_mutex_init(&(mtx), NULL)
#define smileMutexLock(mtx) (pthread_mutex_lock(&(mtx))==0)
#define smileMutexUnlock(mtx) pthread_mutex_unlock(&(mtx))
#define smileMutexTryLock(mtx) pthread_mutex_trylock(&(mtx))
#define smileMutexDestroy(mtx) pthread_mutex_destroy(&(mtx))

typedef struct {
  pthread_mutex_t mtx;
  pthread_cond_t  cond;
  int signaled;
} smileCond;

#define smileCondInitVar(COND) COND.mtx = PTHREAD_MUTEX_INITIALIZER; COND.cond = PTHREAD_COND_INITIALIZER
#define smileCondCreate(COND) { pthread_mutex_init(&(COND.mtx), NULL); \
                                pthread_cond_init(&(COND.cond), NULL); COND.signaled = 0; }
#define smileCondWait(COND) { pthread_mutex_lock(&(COND.mtx)); \
                              if (!COND.signaled) { \
                                pthread_cond_wait(&(COND.cond), &(COND.mtx)); \
                              } COND.signaled = 0; \
                              pthread_mutex_unlock(&(COND.mtx)); }
#define smileCondWaitWMtx(COND,MTX) pthread_cond_wait(&(COND.cond), &(MTX))

// this function returns !=0 upon timeout and 0 upon success (i.e. signal!)
#include <time.h>
// waits with internal mutex
#define smileCondTimedWait(COND,MSEC) { pthread_mutex_lock(&(COND.mtx)); \
                                        if (!COND.signaled) { \
                                          struct timespec __TOut; \
                                          clock_gettime(CLOCK_REALTIME, &__TOut); \
                                          __TOut.tv_sec += (long)MSEC/1000; \
                                          __TOut.tv_nsec += ( (long)MSEC-1000*((long)MSEC/1000) )*1000000; \
                                          pthread_cond_timedwait(&(COND.cond), &(COND.mtx), &__TOut); \
                                        } COND.signaled = 0; \
                                        pthread_mutex_unlock(&(COND.mtx)); }
// waits with external Mutex
#define smileCondTimedWaitWMtx(COND,MSEC,MTX) { if (!COND.signaled) { \
                                        struct timespec __TOut; \
                                        clock_gettime(CLOCK_REALTIME, &__TOut); \
                                        __TOut.tv_sec += (long)MSEC/1000; \
                                        __TOut.tv_nsec += ( (long)MSEC-1000*((long)MSEC/1000) )*1000000; \
                                        pthread_cond_timedwait(&(COND.cond), &(MTX), &__TOut); \
                                      } COND.signaled = 0; }

#define smileCondSignal(COND) { pthread_mutex_lock(&(COND.mtx)); \
                                COND.signaled = 1; \
                                pthread_cond_signal(&(COND.cond)); \
                                pthread_mutex_unlock(&(COND.mtx)); }
#define smileCondSignalRaw(COND) pthread_cond_signal(&(COND.cond))
#define smileCondBroadcast(COND) { pthread_mutex_lock(&(COND.mtx)); \
                                   COND.signaled = 1; \
                                   pthread_cond_broadcast(&(COND.cond)); \
                                   pthread_mutex_unlock(&(COND.mtx)); }
#define smileCondBroadcastRaw(COND) pthread_cond_broadcast(&(COND.cond))
#define smileCondDestroy(COND) { smileCondSignal(COND); \
                                 pthread_cond_destroy(&(COND.cond)); \
                                 pthread_mutex_destroy(&(COND.mtx)); \
                                }

#define smileSleep(msec)  usleep((msec)*1000)
#define smileYield()      sched_yield()

#else //not HAVE_PTHREAD

#ifdef __WINDOWS

#include <process.h>

#define SMILE_THREAD_RETVAL DWORD
#define SMILE_THREAD_RET return 0

#define smileThread  HANDLE
/*
HANDLE CreateThread(
  LPSECURITY_ATTRIBUTES lpThreadAttributes,  // pointer to security attributes
  DWORD dwStackSize,                         // initial thread stack size
  LPTHREAD_START_ROUTINE lpStartAddress,     // pointer to thread function
  LPVOID lpParameter,                        // argument for new thread
  DWORD dwCreationFlags,                     // creation flags
  LPDWORD lpThreadId                         // pointer to receive thread ID
);
*/
//#define smileThreadCreate(THRD,T_MAIN,ARG) ((long)(THRD = (HANDLE)_beginthread(T_MAIN,0,ARG)) != -1)
#define smileThreadCreate(THRD,T_MAIN,ARG) ((void *)(THRD = (HANDLE)CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)T_MAIN,ARG,0,NULL)) != NULL)
//#define smileThreadJoinWret(THRD,RET) pthread_join(THRD,&RET)
#define smileThreadJoinWret(THRD,RET) WaitForSingleObject(THRD, INFINITE)
//#define smileThreadJoin(THRD) pthread_join(THRD,NULL)
#define smileThreadJoin(THRD) WaitForSingleObject(THRD, INFINITE)


#define smileMutex CRITICAL_SECTION
// #define smileMutex CRITICAL_SECTION
//#define smileMutexInitVar(mtx) mtx = PTHREAD_MUTEX_INITIALIZER
#define smileMutexInitVar(mtx) 
// #define smileMutexInitVar(mtx) mtx = ?????
//#define smileMutexCreate(mtx) ( (mtx = CreateMutex( NULL, FALSE, NULL )) != NULL )
#define smileMutexCreate(mtx) InitializeCriticalSection(&mtx)
//#define smileMutexLock(mtx) (pthread_mutex_lock(&(mtx))==0)
//#define smileMutexLock(mtx) WaitForSingleObject( mtx, INFINITE )
inline int enterCSstub(smileMutex *mtx) {
  EnterCriticalSection(mtx);
  return 1;
}
#define smileMutexLock(mtx) enterCSstub(&mtx) //EnterCriticalSection(&mtx)
//#define smileMutexUnlock(mtx) ReleaseMutex( mtx )
#define smileMutexUnlock(mtx) LeaveCriticalSection(&mtx)
//#define smileMutexTryLock(mtx) pthread_mutex_trylock(&(mtx))
//#define smileMutexTryLock(mtx) WaitForSingleObject( mtx, 0 )
#define smileMutexTryLock(mtx) (TryEnterCriticalSection( &mtx ) == TRUE)
//#define smileMutexDestroy(mtx) CloseHandle( mtx )
#define smileMutexDestroy(mtx) DeleteCriticalSection( &mtx )


//------ condition variables:
/*
typedef struct
{
  int waiters_count_;
  // Number of waiting threads.

  CRITICAL_SECTION waiters_count_lock_;
  // Serialize access to <waiters_count_>.

  HANDLE sema_;
  // Semaphore used to queue up threads waiting for the condition to
  // become signaled. 

  HANDLE waiters_done_;
  // An auto-reset event used by the broadcast/signal thread to wait
  // for all the waiting thread(s) to wake up and be released from the
  // semaphore. 

  size_t was_broadcast_;
  // Keeps track of whether we were broadcasting or signaling.  This
  // allows us to optimize the code if we're just signaling.
} pthread_cond_t;

typedef HANDLE pthread_mutex_t;


typedef struct {
  pthread_mutex_t mtx;
  pthread_cond_t  cond;
} smileCond;


inline int 
pthread_cond_init (pthread_cond_t *cv
                   )
{
  cv->waiters_count_ = 0;
  cv->was_broadcast_ = 0;
  cv->sema_ = CreateSemaphore (NULL,       // no security
                                0,          // initially 0
                                0x7fffffff, // max count
                                NULL);      // unnamed 
  InitializeCriticalSection (&cv->waiters_count_lock_);
  cv->waiters_done_ = CreateEvent (NULL,  // no security
                                   FALSE, // auto-reset
                                   FALSE, // non-signaled initially
                                   NULL); // unnamed

  return 1;
}

inline int
pthread_cond_wait (pthread_cond_t *cv, 
                   pthread_mutex_t *external_mutex)
{
  // Avoid race conditions.
  EnterCriticalSection (&cv->waiters_count_lock_);
  cv->waiters_count_++;
  LeaveCriticalSection (&cv->waiters_count_lock_);

  // This call atomically releases the mutex and waits on the
  // semaphore until <pthread_cond_signal> or <pthread_cond_broadcast>
  // are called by another thread.
  SignalObjectAndWait (*external_mutex, cv->sema_, INFINITE, FALSE);

  // Reacquire lock to avoid race conditions.
  EnterCriticalSection (&cv->waiters_count_lock_);

  // We're no longer waiting...
  cv->waiters_count_--;

  // Check to see if we're the last waiter after <pthread_cond_broadcast>.
  int last_waiter = cv->was_broadcast_ && cv->waiters_count_ == 0;

  LeaveCriticalSection (&cv->waiters_count_lock_);

  // If we're the last waiter thread during this particular broadcast
  // then let all the other threads proceed.
  if (last_waiter)
    // This call atomically signals the <waiters_done_> event and waits until
    // it can acquire the <external_mutex>.  This is required to ensure fairness. 
    SignalObjectAndWait (cv->waiters_done_, *external_mutex, INFINITE, FALSE);
  else
    // Always regain the external mutex since that's the guarantee we
    // give to our callers. 
    WaitForSingleObject (*external_mutex, INFINITE);

  return 1;
}

inline int
wpthread_cond_timedwait (pthread_cond_t *cv, 
                   pthread_mutex_t *external_mutex, long Msec)
{
  // Avoid race conditions.
  EnterCriticalSection (&cv->waiters_count_lock_);
  cv->waiters_count_++;
  LeaveCriticalSection (&cv->waiters_count_lock_);

  // This call atomically releases the mutex and waits on the
  // semaphore until <pthread_cond_signal> or <pthread_cond_broadcast>
  // are called by another thread.
  SignalObjectAndWait (*external_mutex, cv->sema_, Msec, FALSE);

  // Reacquire lock to avoid race conditions.
  EnterCriticalSection (&cv->waiters_count_lock_);

  // We're no longer waiting...
  cv->waiters_count_--;

  // Check to see if we're the last waiter after <pthread_cond_broadcast>.
  int last_waiter = cv->was_broadcast_ && cv->waiters_count_ == 0;

  LeaveCriticalSection (&cv->waiters_count_lock_);

  // If we're the last waiter thread during this particular broadcast
  // then let all the other threads proceed.
  if (last_waiter)
    // This call atomically signals the <waiters_done_> event and waits until
    // it can acquire the <external_mutex>.  This is required to ensure fairness. 
    SignalObjectAndWait (cv->waiters_done_, *external_mutex, INFINITE, FALSE);
  else
    // Always regain the external mutex since that's the guarantee we
    // give to our callers. 
    WaitForSingleObject (*external_mutex, INFINITE);

  return 1;
}

inline int
pthread_cond_signal (pthread_cond_t *cv)
{
  EnterCriticalSection (&cv->waiters_count_lock_);
  int have_waiters = cv->waiters_count_ > 0;
  LeaveCriticalSection (&cv->waiters_count_lock_);

  // If there aren't any waiters, then this is a no-op.  
  if (have_waiters)
    ReleaseSemaphore (cv->sema_, 1, 0);
}

inline int
pthread_cond_broadcast (pthread_cond_t *cv)
{
  // This is needed to ensure that <waiters_count_> and <was_broadcast_> are
  // consistent relative to each other.
  EnterCriticalSection (&cv->waiters_count_lock_);
  int have_waiters = 0;

  if (cv->waiters_count_ > 0) {
    // We are broadcasting, even if there is just one waiter...
    // Record that we are broadcasting, which helps optimize
    // <pthread_cond_wait> for the non-broadcast case.
    cv->was_broadcast_ = 1;
    have_waiters = 1;
  }

  if (have_waiters) {
    // Wake up all the waiters atomically.
    ReleaseSemaphore (cv->sema_, cv->waiters_count_, 0);

    LeaveCriticalSection (&cv->waiters_count_lock_);

    // Wait for all the awakened threads to acquire the counting
    // semaphore. 
    WaitForSingleObject (cv->waiters_done_, INFINITE);
    // This assignment is okay, even without the <waiters_count_lock_> held 
    // because no other waiter threads can wake up to access it.
    cv->was_broadcast_ = 0;
  }
  else
    LeaveCriticalSection (&cv->waiters_count_lock_);
  
  return 1;
}
*/

// if vista or > , use native windows condition variables
#ifdef __VISTA

typedef struct {
  CRITICAL_SECTION mtx;
  CONDITION_VARIABLE cond;
} smileCond;

#define smileCondInitVar(COND) InitializeCriticalSection(&(COND.mtx)); InitializeConditionVariable(&(COND.cond));
#define smileCondCreate(COND) InitializeCriticalSection(&(COND.mtx)); InitializeConditionVariable(&(COND.cond));
#define smileCondWait(COND) { EnterCriticalSection(&(COND.mtx)); \
                              SleepConditionVariableCS(&(COND.cond), &(COND.mtx), INFINITE); \
                              LeaveCriticalSection(&(COND.mtx)); }

#define smileCondWaitWMtx(COND,MTX) { SleepConditionVariableCS(&(COND.cond), &(MTX), INFINITE); }

// this function returns !=0 upon timeout and 0 upon success (i.e. signal!)
//#include <time.h>
#define smileCondTimedWait(COND,MSEC) {  EnterCriticalSection(&(COND.mtx)); \
                                         SleepConditionVariableCS(&(COND.cond), &(COND.mtx), MSEC); \
                                         LeaveCriticalSection(&(COND.mtx)); }

#define smileCondTimedWaitWMtx(COND,MSEC,MTX) { SleepConditionVariableCS(&(COND.cond), &(COND.mtx), MSEC); }

#define smileCondSignal(COND) { EnterCriticalSection(&(COND.mtx)); \
                                WakeConditionVariable(&(COND.cond)); \
                                LeaveCriticalSection(&(COND.mtx)); }
#define smileCondSignalRaw(COND) { WakeConditionVariable(&(COND.cond)); }
#define smileCondBroadcast(COND) { EnterCriticalSection(&(COND.mtx)); \
                                   WakeAllConditionVariable(&(COND.cond)); \
                                   LeaveCriticalSection(&(COND.mtx)); }
#define smileCondBroadcastRaw(COND) { WakeAllConditionVariable(&(COND.cond)); }
#define smileCondDestroy(COND)  { DeleteCriticalSection(&(COND.mtx)); }

#else // provide a little hacked implementation for older windowses ...

typedef struct
{
  int waiters_count_;
  // Count of the number of waiters.

  CRITICAL_SECTION waiters_count_lock_;
  // Serialize access to <waiters_count_>.

  int release_count_;
  // Number of threads to release via a <pthread_cond_broadcast> or a
  // <pthread_cond_signal>. 
  
  int wait_generation_count;
  // Keeps track of the current "generation" so that we don't allow
  // one thread to steal all the "releases" from the broadcast.

  HANDLE event_;
  // A manual-reset event that's used to block and release waiting
  // threads. 
} pthread_cond_t;

typedef CRITICAL_SECTION pthread_mutex_t;

typedef struct {
  pthread_mutex_t mtx;
  pthread_cond_t  cond;
} smileCond;



inline int 
pthread_cond_init (pthread_cond_t *cv)
{
  cv->waiters_count_ = 0;
  cv->wait_generation_count = 0;
  cv->release_count_ = 0;

  // Create a manual-reset event.
  cv->event_ = CreateEvent (NULL,  // no security
                            TRUE,  // manual-reset
                            FALSE, // non-signaled initially
                            NULL); // unnamed

  InitializeCriticalSection(&(cv->waiters_count_lock_));

  return 1;
}

#ifdef _MSC_VER // Visual Studio specific macro
#pragma message("Legacy Windows implementation of pthreads is buggy and should not be used anymore.")
#else
#warning Legacy Windows implementation of pthreads is buggy and should not be used anymore.
#endif

/* WARNING: this code has deadlocking issues (e.g. in sensAI-embedded-package, 
connecting with a client to cSmileTcpServerListener and attempting to close the
server will hang) and should not be used anymore. */
inline int
pthread_cond_wait (pthread_cond_t *cv,
                   pthread_mutex_t *external_mutex)
{
  // Avoid race conditions.
  EnterCriticalSection (&(cv->waiters_count_lock_));

  // Increment count of waiters.
  cv->waiters_count_++;

  // Store current generation in our activation record.
  int my_generation = cv->wait_generation_count;

  LeaveCriticalSection (&(cv->waiters_count_lock_));
  LeaveCriticalSection (external_mutex);

  for (;;) {
    // Wait until the event is signaled.
    WaitForSingleObject (cv->event_, INFINITE);

    EnterCriticalSection (&(cv->waiters_count_lock_));
    // Exit the loop when the <cv->event_> is signaled and
    // there are still waiting threads from this <wait_generation>
    // that haven't been released from this wait yet.
    int wait_done = cv->release_count_ > 0
                    && cv->wait_generation_count != my_generation;
    LeaveCriticalSection (&(cv->waiters_count_lock_));

    if (wait_done)
      break;
  }

  EnterCriticalSection (external_mutex);
  EnterCriticalSection (&(cv->waiters_count_lock_));
  cv->waiters_count_--;
  cv->release_count_--;
  int last_waiter = cv->release_count_ == 0;
  LeaveCriticalSection (&(cv->waiters_count_lock_));

  if (last_waiter)
    // We're the last waiter to be notified, so reset the manual event.
    ResetEvent (cv->event_);

  return 1;
}

/* WARNING: this code has deadlocking issues (e.g. in sensAI-embedded-package,
connecting with a client to cSmileTcpServerListener and attempting to close the
server will hang) and should not be used anymore. */
inline int
pthread_cond_timedwait (pthread_cond_t *cv,
                   pthread_mutex_t *external_mutex,
                   long msec)
{
  // Avoid race conditions.
  EnterCriticalSection (&(cv->waiters_count_lock_));

  // Increment count of waiters.
  cv->waiters_count_++;

  // Store current generation in our activation record.
  int my_generation = cv->wait_generation_count;

  LeaveCriticalSection (&(cv->waiters_count_lock_));
  LeaveCriticalSection (external_mutex);

  for (;;) {
    // Wait until the event is signaled.
    int ret = WaitForSingleObject (cv->event_, msec);

    EnterCriticalSection (&(cv->waiters_count_lock_));
    // Exit the loop when the <cv->event_> is signaled and
    // there are still waiting threads from this <wait_generation>
    // that haven't been released from this wait yet.
    int wait_done = cv->release_count_ > 0
                    && cv->wait_generation_count != my_generation;
    LeaveCriticalSection (&(cv->waiters_count_lock_));

    if (ret == WAIT_TIMEOUT) break;

    if (wait_done)
      break;
  }

  EnterCriticalSection (external_mutex);
  EnterCriticalSection (&(cv->waiters_count_lock_));
  cv->waiters_count_--;
  cv->release_count_--;
  int last_waiter = cv->release_count_ == 0;
  LeaveCriticalSection (&(cv->waiters_count_lock_));

  if (last_waiter)
    // We're the last waiter to be notified, so reset the manual event.
    ResetEvent (cv->event_);

  return 1;
}

inline int
pthread_cond_signal (pthread_cond_t *cv)
{
  EnterCriticalSection (&(cv->waiters_count_lock_));
  if (cv->waiters_count_ > cv->release_count_) {
    SetEvent (cv->event_); // Signal the manual-reset event.
    cv->release_count_++;
    cv->wait_generation_count++;
  }
  LeaveCriticalSection (&(cv->waiters_count_lock_));
  return 1;
}

inline int
pthread_cond_broadcast (pthread_cond_t *cv)
{
  EnterCriticalSection (&(cv->waiters_count_lock_));
  if (cv->waiters_count_ > 0) {  
    SetEvent (cv->event_);
    // Release all the threads in this generation.
    cv->release_count_ = cv->waiters_count_;

    // Start a new generation.
    cv->wait_generation_count++;
  }
  LeaveCriticalSection (&cv->waiters_count_lock_);
  return 1;
}

inline int
pthread_cond_destroy (pthread_cond_t *cv) 
{
  DeleteCriticalSection(&(cv->waiters_count_lock_));
  CloseHandle(cv->event_);
  return 1;
}

#define smileCondInitVar(COND) 
#define smileCondCreate(COND) InitializeCriticalSection(&(COND.mtx)); pthread_cond_init(&(COND.cond));
#define smileCondWait(COND) { EnterCriticalSection(&(COND.mtx)); \
                              pthread_cond_wait(&(COND.cond), &(COND.mtx)) \
                              LeaveCriticalSection(&(COND.mtx)); }

#define smileCondWaitWMtx(COND,MTX) pthread_cond_wait(&(COND.cond), &(MTX))

// this function returns !=0 upon timeout and 0 upon success (i.e. signal!)
#include <time.h>
#define smileCondTimedWait(COND,MSEC) {  EnterCriticalSection(&(COND.mtx)); \
                                        pthread_cond_timedwait(&(COND.cond), &(COND.mtx), MSEC); \
                                        LeaveCriticalSection(&(COND.mtx)); }

#define smileCondTimedWaitWMtx(COND,MSEC,MTX) { pthread_cond_timedwait(&(COND.cond), &(MTX), MSEC); }

#define smileCondSignal(COND) { EnterCriticalSection(&(COND.mtx)); \
                                pthread_cond_signal(&(COND.cond)); \
                                LeaveCriticalSection(&(COND.mtx)); }
#define smileCondSignalRaw(COND) { pthread_cond_signal(&(COND.cond)); }
#define smileCondBroadcast(COND) { EnterCriticalSection(&(COND.mtx)); \
                                   pthread_cond_broadcast(&(COND.cond)); \
                                   LeaveCriticalSection(&(COND.mtx)); }
#define smileCondBroadcastRaw(COND) pthread_cond_broadcast(&(COND.cond))
#define smileCondDestroy(COND)  DeleteCriticalSection(&(COND.mtx)); pthread_cond_destroy(&(COND.cond));
/* TODO: destroy condition */

#endif // __VISTA

//#define smileSleep(msec)  usleep((msec)*1000)
#define smileSleep(msec)  Sleep( msec )
//#define smileYield()      sched_yield()
#define smileYield()   SwitchToThread() //Sleep( 0 )



#endif // __WINDOWS
#endif // HAVE_PTHREAD


//--------------------------------------------------

#define smileCommon_fixLocaleEnUs() { setlocale(LC_NUMERIC, "en_US"); }

// On Windows, only version Windows 10 v1511 and later support VT100 escape sequences for colored console output
// and they have to be enabled programmatically. 
void smileCommon_enableVTInWindowsConsole();

#include <core/exceptions.hpp>
#include <core/smileLogger.hpp>
#include <smileutil/smileUtil.h>

//#define XERCESC_NS  xercesc_3_0

#endif  // __SMILE_COMMON_H
