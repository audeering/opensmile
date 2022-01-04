/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#ifndef __COMPONENT_MANAGER_HPP
#define __COMPONENT_MANAGER_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <core/smileThread.hpp>
#include <chrono>

// this is the name of the configuration instance in the config file the component manager will search for:
#define CM_CONF_INST  "componentInstances"


// global component list: -----------------
class cComponentManager;
typedef sComponentInfo * (*registerFunction)(cConfigManager *_confman, cComponentManager *_compman, int _iteration);
typedef void (*unRegisterFunction)();

extern const registerFunction componentlist[];

// default number of initial component variable space and plugin variable space to allocate:
#define COMPONENTMANAGER_DEFAULT_NCOMPS  200

// Max. iterations for register type, register instance, configure, and finalise process of components
#define MAX_REG_ITER  4
#define MAX_REGI_ITER 2
#define MAX_CONF_ITER 4  // <- should be nComponents!!
#define MAX_FIN_ITER  4


// openSMILE component runner thread status:
#define THREAD_ACTIVE   0
#define THREAD_WAIT_A   1
#define THREAD_WAIT_B   2
#define THREAD_END      3
#define THREAD_INACTIVE 4

typedef struct {
  cComponentManager *obj;
  long long maxtick;
  int threadId;
  int status;
} sThreadData;


class cComponentManager {
protected:
  int printPlugins;
//#ifdef DEBUG
  int execDebug;
//#endif
#ifdef __WINDOWS
  HINSTANCE *handlelist;
#else
  void **handlelist;
#endif

  registerFunction * regFnlist;
  int nPluginHandles, nPluginHandlesAlloc;

  int loadPlugin(const char * path, const char * fname);
  int registerPlugins();
  int checkPluginFilename(const char * str);
  
  int pausedNotifyComponents(int threadId, int isPause);

public:
  static void registerType(cConfigManager *_confman);

  cComponentManager(cConfigManager *_confman, const registerFunction _clist[] = componentlist);               // create component manager

  cConfigManager *getConfigManager() const { return confman; }
  cSmileLogger *getLogger() const { return logger; }

  int compIsDm(const char *_compn);
  int ciRegisterComps(int _dm);
  int ciConfigureComps(int _dm);
  int ciFinaliseComps(int _dm, int *_n=NULL);
  int ciConfFinComps(int _dm, int *_n=NULL);

  void createInstances(int readConfig=1); // read config file and create instances as specified in config file
  void configureAndFinaliseComponents(); // usually called as part of createInstances

  // end-of-input handling:
  void setEOIcounterInComponents();
  void setEOI() {
    EOI++;
    EOIcondition = 1;
  }
  void unsetEOI() {
    EOIcondition = 0;
  }

  // check for EOIcondition
  int isEOI() { return EOIcondition; }
  // get the EOI counter
  int getEOIcounter() { return EOI; }

  long long tickLoopA(long long maxtick, int threadId, sThreadData *_data);
  void controlLoopA(void);

  long componentOnEmptyTickloop(long long threadId, long long tickNr);
  // call all components to process one time step ('tick')
  // out parameter nRun indicates how many components performed work during this tick,
  // out parameter nWaiting indicates how many components waited on external data during this tick
  void tick(int threadId, long long tickNr, long lastNrun, long tickResultCounts[NUM_TICK_RESULTS]);

  // this function calls tick() until 0 components run successfully, returns total number of ticks
  long long runSingleThreaded(long long maxtick=-1);

  // still a bit todo, basically same as runSingleThreaded with multiple threads, total number of ticks not yet fully supported!!
  long long runMultiThreaded(long long maxtick=-1);

  int addComponent(const char *_instname, const char *_type, const char *_ci=NULL, int _threadId=-1);  // create + register
  cSmileComponent * createComponent(const char *_name, const char *_type);
  cSmileComponent * createComponent(const char *_name, int n);

  // return number of registered component types
  int getNtypes() { return nCompTs; } 

  // filter=0: no filter, filter=1 : no abstract components, filter=2 non abstract and no non-Dmem (non-standalone) components (= sub-components)
  const char * getComponentType(int i, int filter=2, int *isAbstract=NULL, int *isNoDmem=NULL); // get name of component Type "i". abstract=1 = include abstract types (if abstract = 0 , then NULL will be returned if the type is abstract), or you may use the isAbstract flag, which is set by this function

  // get description of component "i"
  const char * getComponentDescr(int i); 

  int registerComponent(sComponentInfo *c, int noFree=0); // register a component type
  int findComponentType(const char *_type);
  int registerComponentTypes(const registerFunction _clist[]); // register component types by scanning given struct

  // filter=0: no filter, filter=1 : no abstract components, filter=2 non abstract and no non-Dmem (non-standalone) components (= sub-components)
  int printComponentList(int filter=2, int details=1);
  void exportComponentList();

  int registerComponentInstance(cSmileComponent * _component, const char *_typename, int _threadId=-1);  // register a component, return value: component id
  void unregisterComponentInstance(int id, int noDM=0);  // unregister and free component object
  int findComponentInstance(const char *_compname) const;
  cSmileComponent * getComponentInstance(int n);
  const char * getComponentInstanceType(int n);
  cSmileComponent * getComponentInstance(const char *_compname);
  const char * getComponentInstanceType(const char *_compname);

  // send inter component messages directly to component:
  int sendComponentMessage(const char *_compname, cComponentMessage *_msg);
  // send inter component messages directly to component:
  int sendComponentMessage(int compid, cComponentMessage *_msg);

  // get time in seconds (accurate up to the millisecond or microsecond (depending on the system)) since creation if componentManager
  double getSmileTime();

  void resetInstances(void);  // delete all component instances and reset componentManger to state before createInstances
  void waitForAllThreads(int threadID);
  void waitForController(int threadID, int stage);

  /* pause tick loop:
how=0  :  resume tick loop
how=1  :  notify components, halt loop in a sleep
how=2  :  notify components,  wait for nRun = 0 (force NOT EOI!), then put loop to sleep
timeout :  if how=2, then max number of ticks of nRun>0 , after which pause request is discarded and EOI is assumed again when nRun=0  (irrelevant when how=1)
*/
  void pause(int how=1, int timeout=20);

  // resumes the tick loop from paused/sleep mode
  void resume() { 
    pause(0,0); 
  }

  // requests program loop termination (works for single and multithreaded)
  void requestAbort();

  // returns 1 if the tick loop is in an aborting state
  // (i.e. after calling requestAbort()) - returns 0 otherwise
  int isAbort();

  // signal to external sources or online sources that data is available
  // this will wake the tick loop from a sleep (if it is currently waiting
  // for new data)
  void signalDataAvailable();

  int isReady() {
    return ready;
  }

  ~cComponentManager();              // unregister and free all component objects

private:
  cConfigManager *confman;
  cSmileLogger *logger;
  void myFetchConfig(); // read config from confman

  int printLevelStats;
  int profiling;
  int printFinalLevelStates;

  std::chrono::time_point<std::chrono::steady_clock> startTime;

  int nCompTs, nCompTsAlloc;
  sComponentInfo *compTs; // component types

  int ready;    // flag that indicates if all components are set up and ready...
  int isConfigured;
  int isFinalised;
  int EOI;            // EOI iteration counter
  int EOIcondition;   // EOI flag

  //long long tickNr;
  int nComponents, nComponentsAlloc;
  int nActiveComponents;  // number of non-passive (i.e. non-dataMemory components)
  int nThreads;
  int threadPriority; /* default thread priority, or single thread priority */

  int lastComponent;
  cSmileComponent ** component;   // array of component instances
  char **componentInstTs;    // component instance corresponding type names
  int  * componentThreadId;

  long messageCounter;
  int oldSingleIterationTickLoop;

  long long pauseStartNr;
  int tickLoopPaused; // 0: not paused, 1: paused in mode 1, 2: paused in mode 2
  int tickLoopPauseTimeout;
  smileCond pauseCond;
  smileMutex pauseMtx;

  smileCond     dataAvailableCond;

  smileMutex    messageMtx;
  smileMutex    waitEndMtx;
  smileCond     waitEndCond;
  smileCond     waitControllerCond;
  int waitEndCnt;

  // multi thread sync variables:
  smileMutex    syncCondMtx;
  smileMutex    abortMtx;
  smileCond     syncCond;
  smileCond     controlCond;
  int nWaiting, nProbe, nActive;
  int runStatus, compRunFlag, probeFlag, controllerPresent; // communication flags between controller thread and component runner threads
  int *runFlag;
  int globalRunState;
  int abortRequest, endOfLoop;

  int getNextComponentId();

  void printExecDebug(int threadId); // prints debug output when the execDebug option is enabled in the config
  
  // Pause the current tick loop (by either sleeping or polling, specified via tickLoopPaused variable) 
  // and notify all associated components about pause/resume correctly. Returns whether the tick loop
  // should continue running or stop.
  bool pauseThisTickLoop(int threadID, int nRun, long long tickNr);
};


#endif  // __COMPONENT_MANAGER_HPP
