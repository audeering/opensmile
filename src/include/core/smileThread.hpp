/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*

Platform-independent abstraction layer around threading, mutexes and condition variables

*/


#ifndef __SMILE_THREAD_HPP
#define __SMILE_THREAD_HPP

#ifndef __WINDOWS
// on all platforms other than Windows, we rely on POSIX threads

#include <pthread.h>

using SMILE_THREAD_RETVAL = void *;
#define SMILE_THREAD_RET return NULL

using smileThread = pthread_t;
using smileMutex = pthread_mutex_t;

typedef struct {
  pthread_mutex_t mtx;
  pthread_cond_t cond;
  int signaled;
} smileCond;

#else
// on Windows, we use Windows-specific functions (available on Vista and higher)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using SMILE_THREAD_RETVAL = DWORD;
#define SMILE_THREAD_RET return 0

using smileThread = HANDLE;
using smileMutex = CRITICAL_SECTION;

typedef struct {
  CRITICAL_SECTION mtx;
  CONDITION_VARIABLE cond;
  int signaled;
} smileCond;

#endif

typedef SMILE_THREAD_RETVAL smileThreadFunc(void *arg);

// thread functions
bool smileThreadCreate(smileThread &thrd, smileThreadFunc func, void *arg);
void smileThreadJoin(smileThread &thrd);

// mutex functions
void smileMutexInitVar(smileMutex &mtx);
bool smileMutexCreate(smileMutex &mtx);
bool smileMutexLock(smileMutex &mtx);
bool smileMutexUnlock(smileMutex &mtx);
bool smileMutexTryLock(smileMutex &mtx);
bool smileMutexDestroy(smileMutex &mtx);

// condition variable functions
void smileCondInitVar(smileCond &cond);
void smileCondCreate(smileCond &cond);
void smileCondWait(smileCond &cond);
void smileCondWaitWMtx(smileCond &cond, smileMutex &mtx);
void smileCondTimedWait(smileCond &cond, long msec); // waits with internal mutex
void smileCondTimedWaitWMtx(smileCond &cond, long msec, smileMutex &mtx); // waits with external Mutex
void smileCondSignal(smileCond &cond);
void smileCondSignalRaw(smileCond &cond);
void smileCondBroadcast(smileCond &cond);
void smileCondBroadcastRaw(smileCond &cond);
void smileCondDestroy(smileCond &cond);

// miscellaneous functions
void smileSleep(long msec);
void smileYield();

#endif  // __SMILE_THREAD_HPP