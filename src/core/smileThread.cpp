/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/******************************************************************************/

 /*

Platform-independent abstraction layer around threading, mutexes and condition variables

 */


#include <core/smileThread.hpp>

#ifndef __WINDOWS
#include <time.h>
#include <unistd.h>

#ifdef __MACOS
int clock_gettime(int clock_id, struct timespec *tp);
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#endif

bool smileThreadCreate(smileThread &thrd, smileThreadFunc func, void *arg)
{
  return pthread_create(&thrd, NULL, func, arg) == 0;
}

void smileThreadJoin(smileThread &thrd)
{
  pthread_join(thrd, NULL);
}

void smileMutexInitVar(smileMutex &mtx)
{
  mtx = PTHREAD_MUTEX_INITIALIZER;
}

bool smileMutexCreate(smileMutex &mtx)
{
  return pthread_mutex_init(&mtx, NULL) == 0;
}

bool smileMutexLock(smileMutex &mtx)
{
  return pthread_mutex_lock(&mtx) == 0;
}

bool smileMutexUnlock(smileMutex &mtx)
{
  return pthread_mutex_unlock(&mtx) == 0;
}

bool smileMutexTryLock(smileMutex &mtx)
{
  return pthread_mutex_trylock(&mtx) == 0;
}

bool smileMutexDestroy(smileMutex &mtx)
{
  return pthread_mutex_destroy(&mtx) == 0;
}

void smileCondInitVar(smileCond &cond)
{
  cond.mtx = PTHREAD_MUTEX_INITIALIZER;
  cond.cond = PTHREAD_COND_INITIALIZER;
  cond.signaled = 0;
}

void smileCondCreate(smileCond &cond)
{
  pthread_mutex_init(&cond.mtx, NULL);
  pthread_cond_init(&cond.cond, NULL);
  cond.signaled = 0;
}

void smileCondWait(smileCond &cond)
{
  pthread_mutex_lock(&cond.mtx);
  while (!cond.signaled) {
    pthread_cond_wait(&cond.cond, &cond.mtx);
  }
  cond.signaled = 0;
  pthread_mutex_unlock(&cond.mtx);
}

void smileCondWaitWMtx(smileCond &cond, smileMutex &mtx)
{
  while (!cond.signaled) {
    pthread_cond_wait(&cond.cond, &mtx);
  }
  cond.signaled = 0;
}

void smileCondTimedWait(smileCond &cond, long msec)
{
  pthread_mutex_lock(&cond.mtx);
  while (!cond.signaled) {
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    time.tv_sec += msec/1000;
    time.tv_nsec += ( msec-1000*(msec/1000) )*1000000;
    pthread_cond_timedwait(&cond.cond, &cond.mtx, &time);
  }
  cond.signaled = 0;
  pthread_mutex_unlock(&cond.mtx);
}

void smileCondTimedWaitWMtx(smileCond &cond, long msec, smileMutex &mtx)
{ 
  while (!cond.signaled) {
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    time.tv_sec += msec/1000;
    time.tv_nsec += ( msec-1000*(msec/1000) )*1000000;
    pthread_cond_timedwait(&cond.cond, &mtx, &time);
  }
  cond.signaled = 0;
}

void smileCondSignal(smileCond &cond)
{
  pthread_mutex_lock(&cond.mtx);
  cond.signaled = 1;
  pthread_cond_signal(&cond.cond);
  pthread_mutex_unlock(&cond.mtx);
}

void smileCondSignalRaw(smileCond &cond)
{
  pthread_cond_signal(&cond.cond);
}

void smileCondBroadcast(smileCond &cond)
{
  pthread_mutex_lock(&cond.mtx);
  cond.signaled = 1;
  pthread_cond_broadcast(&cond.cond);
  pthread_mutex_unlock(&cond.mtx);
}

void smileCondBroadcastRaw(smileCond &cond)
{
  pthread_cond_broadcast(&cond.cond);
}

void smileCondDestroy(smileCond &cond)
{
  smileCondSignal(cond);
  pthread_cond_destroy(&cond.cond);
  pthread_mutex_destroy(&cond.mtx);
}

void smileSleep(long msec)
{
  usleep(msec*1000);
}

void smileYield()
{
  sched_yield();
}

#else

bool smileThreadCreate(smileThread &thrd, smileThreadFunc func, void *arg)
{
  thrd = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL);
  return thrd != NULL;
} 

void smileThreadJoin(smileThread &thrd)
{
  WaitForSingleObject(thrd, INFINITE);
}

void smileMutexInitVar(smileMutex &mtx)
{
}

bool smileMutexCreate(smileMutex &mtx)
{
  InitializeCriticalSection(&mtx);
  return true;
}

bool smileMutexLock(smileMutex &mtx)
{
  EnterCriticalSection(&mtx);
  return true;
}

bool smileMutexUnlock(smileMutex &mtx)
{
  LeaveCriticalSection(&mtx);
  return true;
}

bool smileMutexTryLock(smileMutex &mtx)
{
  return TryEnterCriticalSection(&mtx) == TRUE;
}

bool smileMutexDestroy(smileMutex &mtx)
{
  DeleteCriticalSection(&mtx);
  return true;
}

void smileCondInitVar(smileCond &cond)
{
  InitializeCriticalSection(&cond.mtx);
  InitializeConditionVariable(&cond.cond);
  cond.signaled = 0;
}

void smileCondCreate(smileCond &cond)
{
  InitializeCriticalSection(&cond.mtx);
  InitializeConditionVariable(&cond.cond);
  cond.signaled = 0;
}

void smileCondWait(smileCond &cond)
{
  EnterCriticalSection(&cond.mtx);
  while (!cond.signaled) {
    SleepConditionVariableCS(&cond.cond, &cond.mtx, INFINITE);
  }
  cond.signaled = 0;
  LeaveCriticalSection(&cond.mtx);
}

void smileCondWaitWMtx(smileCond &cond, smileMutex &mtx)
{
  while (!cond.signaled) {
    SleepConditionVariableCS(&cond.cond, &mtx, INFINITE);
  }
  cond.signaled = 0;
}

void smileCondTimedWait(smileCond &cond, long msec)
{
  EnterCriticalSection(&cond.mtx);
  while (!cond.signaled) {
    SleepConditionVariableCS(&cond.cond, &cond.mtx, msec);
  }
  cond.signaled = 0;
  LeaveCriticalSection(&cond.mtx);
}

void smileCondTimedWaitWMtx(smileCond &cond, long msec, smileMutex &mtx)
{
  while (!cond.signaled) {
    SleepConditionVariableCS(&cond.cond, &cond.mtx, msec);
  }
  cond.signaled = 0;
}

void smileCondSignal(smileCond &cond)
{
  EnterCriticalSection(&cond.mtx);
  cond.signaled = 1;
  WakeConditionVariable(&cond.cond);
  LeaveCriticalSection(&cond.mtx);
}

void smileCondSignalRaw(smileCond &cond)
{
  WakeConditionVariable(&cond.cond);
}

void smileCondBroadcast(smileCond &cond)
{
  EnterCriticalSection(&cond.mtx);
  cond.signaled = 1;
  WakeAllConditionVariable(&cond.cond);
  LeaveCriticalSection(&cond.mtx);
}

void smileCondBroadcastRaw(smileCond &cond)
{
  WakeAllConditionVariable(&cond.cond);
}

void smileCondDestroy(smileCond &cond)
{
  smileCondSignal(cond);
  DeleteCriticalSection(&cond.mtx);
}

void smileSleep(long msec)
{
  Sleep(msec);
}

void smileYield()
{
  SwitchToThread();
}

#endif