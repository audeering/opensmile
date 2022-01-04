/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

/* openSMILE plugin loader */

#include <core/smileCommon.hpp>
#include <core/smileLogger.hpp>
#include <core/componentManager.hpp>


//++ include all your component headers here: ----------------
#include <exampleGuipluginSink.hpp>
#include <simpleVisualiserGUI.hpp>
#include <classifierResultGUI.hpp>

//++ ---------------------------------------------------------

// dll export for msvc++
#ifdef _MSC_VER 
#define DLLEXPORT __declspec(dllexport)
#define DLLLOCAL
#else
#define DLLEXPORT
#define DLLLOCAL
#endif


#define MODULE "pluginLoader"

static DLLLOCAL const registerFunction complist[] = {
//++ add your component register functions here: -------------
  cExampleGuipluginSink::registerComponent,
  cSimpleVisualiserGUI::registerComponent,
  cClassifierResultGUI::registerComponent,

//++ ---------------------------------------------------------
  NULL  // last element must always be NULL to terminate the list !!
};

sComponentInfo *master=NULL;

//Library:
extern "C" DLLEXPORT sComponentInfo * registerPluginComponent(cConfigManager *_confman, cComponentManager *_compman) {
  registerFunction f;
  sComponentInfo *cur = NULL, *last = NULL;
  int i=0;
  f = complist[i];
  while (f!=NULL) {
    cur = (f)(_confman, _compman);
    cur->next = NULL;
    if (i==0) master = cur;
    else { last->next = cur; }
    last = cur;
    f=complist[++i];
  }
  return master;
}

/*extern "C" DLLEXPORT sComponentInfo * registerPluginComponent(cConfigManager *_confman, cComponentManager *_compman) {
  return (sComponentInfo *)complist;
}*/


// this function frees all memory allocated within the scope of the dll ...
extern "C" DLLEXPORT void unRegisterPluginComponent() {
  while (master != NULL) {
    sComponentInfo *tmp = master;
    if (master->next != NULL) master = master->next;
    delete tmp;
  }
}
