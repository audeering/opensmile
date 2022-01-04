/*F***************************************************************************
 * This file is part of openSMILE.
 *
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

/*

C API wrapper for libopensmile

*/

#include <smileapi/SMILEapi.h>

#include <core/smileCommon.hpp>
#include <core/configManager.hpp>
#include <core/commandlineParser.hpp>
#include <core/componentManager.hpp>

#include <iocore/externalSink.hpp>
#include <iocore/externalSource.hpp>
#include <iocore/externalAudioSource.hpp>
#include <other/externalMessageInterface.hpp>

#include <locale.h>
#include <memory>

#if FLOAT_DMEM_NUM != FLOAT_DMEM_FLOAT
  #error SMILEapi is only compatible with float as FLOAT_DMEM type
#endif

#define MODULE "SMILEapi"

struct smileobj_t_ {
  smilestate_t state;
  void *stateCallbackParam;
  StateChangedCallback stateCallback;
  LogCallback logCallback;
  void *logCallbackParam;
  cSmileLogger *logger;
  cConfigManager *configManager;
  cComponentManager *cMan;
  std::string lastError;

  smileobj_t_() : state(SMILE_UNINITIALIZED), stateCallback(NULL), stateCallbackParam(NULL),
    logCallback(NULL), logCallbackParam(NULL), logger(NULL), configManager(NULL), cMan(NULL), lastError() {}
};

static smileres_t smile_fail_with(smileobj_t *smileobj, smileres_t result, const std::string &msg)
{
  if (smileobj != NULL)
    smileobj->lastError = msg;
  return result;
}

static smileres_t smile_fail_with(smileobj_t *smileobj, smileres_t result, const char *msg)
{
  if (smileobj != NULL)
    smileobj->lastError = msg != NULL ? std::string(msg) : std::string();
  return result;
}

#define SMILE_FAIL_WITH(result, msg) return smile_fail_with(smileobj, result, msg);

static void smile_set_state(smileobj_t *smileobj, smilestate_t state)
{
  smileobj->state = state;
  if (smileobj->stateCallback != NULL)
    smileobj->stateCallback(smileobj, state, smileobj->stateCallbackParam);
}

smileobj_t *smile_new()
{
  return new (std::nothrow) smileobj_t();
}

smileres_t smile_initialize(smileobj_t *smileobj, const char *configFile, int nOptions, const smileopt_t *options, int loglevel, int debug, int consoleOutput, const char *logFile)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (configFile == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "configFile argument must not be null");
  if (smileobj->state != SMILE_UNINITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE has already been initialized");

  try {
    // this is required to ensure config files are parsed with the correct locale
    // however, it overrides the locale globally in the whole application
    // we should adapt the config file parser to make use of the improved locale handling in C++
    smileCommon_fixLocaleEnUs();

    std::unique_ptr<cSmileLogger> logger { new cSmileLogger(1) };

    logger->useForCurrentThread();
    logger->setLogFile(logFile, 0);
    logger->setConsoleOutput(consoleOutput);
    logger->setColoredOutput(false);
    logger->setLogLevel(loglevel);
#ifdef DEBUG
    if (!debug)
      logger->setLogLevel(LOG_DEBUG, 0);
#endif    
    logger->setCallback([=](int type, int level, const char *text, const char *module) {
      // copy smileobj->logCallback to avoid race condition
      auto logCallback = smileobj->logCallback;
      if (logCallback != NULL) {
        smilelogmsg_t message{static_cast<smilelogtype_t>(type), level, text, module};
        logCallback(smileobj, message, smileobj->logCallbackParam);
      }
    });

    // convert 'options' to the format that cCommandlineParser expects (SMILExtract -key1 value1 ...)
    std::vector<std::string> cmdArgs;
    cmdArgs.reserve(nOptions * 2 + 1);
    cmdArgs.push_back("SMILExtract");
    for (int i = 0; i < nOptions; i++) {
      cmdArgs.push_back("-" + std::string(options[i].name));
      if (options[i].value != NULL)
        cmdArgs.push_back(std::string(options[i].value));
    }
    // convert elements in cmdArgs to const char *
    std::vector<const char *> cmdArgs2;
    cmdArgs2.reserve(cmdArgs.size());
    for (const auto &arg : cmdArgs)
      cmdArgs2.push_back(arg.c_str());

    cCommandlineParser cmdline(cmdArgs2.size(), cmdArgs2.data());
    if (cmdline.parse() == -1) {
      SMILE_FAIL_WITH(SMILE_FAIL, "command-line could not be parsed");
    }

    SMILE_MSG(2,"openSMILE starting!");
    SMILE_MSG(2,"config file is: %s", configFile);

    /* we declare componentManager before configManager because
       configManager must be deleted BEFORE componentManager
       (since component Manager unregisters plugin DLLs which might have allocated configTypes, etc.) */
    std::unique_ptr<cComponentManager> cMan;
    std::unique_ptr<cConfigManager> configManager;
    configManager = std::unique_ptr<cConfigManager>(new cConfigManager(&cmdline));
    cMan = std::unique_ptr<cComponentManager>(new cComponentManager(configManager.get(),componentlist));

    // before parsing the config file, we temporarily
    // override the default numerical format
    std::string origLocale = setlocale(LC_NUMERIC, NULL);
    setlocale(LC_NUMERIC, "en_US.UTF8");

    try {
      cFileConfigReader * reader = new cFileConfigReader(configFile, -1, &cmdline);
      configManager->addReader(reader);      
      configManager->readConfig();
      setlocale(LC_NUMERIC, origLocale.c_str());
    } catch (const cConfigException& ex) {
      setlocale(LC_NUMERIC, origLocale.c_str());
      SMILE_FAIL_WITH(SMILE_CONFIG_PARSE_FAIL, ex.getText());
    }

    /* re-parse the command-line to include options created in the config file */
    cmdline.parse(true, false); // warn if unknown options are detected on the commandline

    try {
      /* create all instances specified in the config file */
      cMan->createInstances(0); // 0 = do not read config (we already did that above..)
    } catch (const cConfigException& ex) {
      SMILE_FAIL_WITH(SMILE_CONFIG_INIT_FAIL, ex.getText());
    }

    smileobj->logger = logger.release();
    smileobj->cMan = cMan.release();
    smileobj->configManager = configManager.release();

    smile_set_state(smileobj, SMILE_INITIALIZED);

    return SMILE_SUCCESS;
  } catch (const cSMILException& ex) {
    SMILE_FAIL_WITH(SMILE_FAIL, ex.getText());
  } catch (...) {
    SMILE_FAIL_WITH(SMILE_FAIL, "Unknown exception");
  }
}

smileres_t smile_run(smileobj_t *smileobj)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (smileobj->state != SMILE_INITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be initialized first");

  // smile_run is very likely getting called from a dedicated thread,
  // so we need to set the global logger for it 
  smileobj->logger->useForCurrentThread();

  smile_set_state(smileobj, SMILE_RUNNING);

  try {
    smileobj->cMan->runMultiThreaded(-1);
  } catch (const cSMILException& ex) {
    SMILE_FAIL_WITH(SMILE_FAIL, ex.getText());
  } catch (...) {
    SMILE_FAIL_WITH(SMILE_FAIL, "Unknown exception");
  }

  smile_set_state(smileobj, SMILE_ENDED);

  return SMILE_SUCCESS;
}

smileres_t smile_abort(smileobj_t *smileobj)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  // we also allow calling this function in the ENDED state as it may be impossible
  // for users of the library to guarantee that the run hasn't ended yet
  if (smileobj->state != SMILE_RUNNING && smileobj->state != SMILE_ENDED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be in the running state for aborting");

  // in case this function gets called from a new thread, we need to set the global logger for it
  smileobj->logger->useForCurrentThread();

  if (smileobj->state == SMILE_RUNNING)
    smileobj->cMan->requestAbort();

  return SMILE_SUCCESS;
}

smileres_t smile_reset(smileobj_t *smileobj)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (smileobj->state != SMILE_ENDED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be in the ended state for resetting");

  // in case this function gets called from a new thread, we need to set the global logger for it
  smileobj->logger->useForCurrentThread();

  try {
    smileobj->cMan->resetInstances();
    smileobj->cMan->createInstances(0);
  } catch (const cSMILException& ex) {
    SMILE_FAIL_WITH(SMILE_FAIL, ex.getText());
  } catch (...) {
    SMILE_FAIL_WITH(SMILE_FAIL, "Unknown exception");
  }

  smile_set_state(smileobj, SMILE_INITIALIZED);

  return SMILE_SUCCESS;
}

smileres_t smile_set_log_callback(smileobj_t *smileobj, LogCallback callback, void *param)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");

  smileobj->logCallback = callback;
  smileobj->logCallbackParam = param;

  return SMILE_SUCCESS;
}

smilestate_t smile_get_state(smileobj_t *smileobj)
{
  if (smileobj == NULL)
    return SMILE_UNINITIALIZED;

  return smileobj->state;
}

smileres_t smile_set_state_callback(smileobj_t *smileobj, StateChangedCallback callback, void *param)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");

  smileobj->stateCallback = callback;
  smileobj->stateCallbackParam = param;

  return SMILE_SUCCESS;
}

void smile_free(smileobj_t *smileobj)
{
  if (smileobj != NULL) {
    if (smileobj->configManager != NULL)
      delete smileobj->configManager;
    if (smileobj->cMan != NULL)
      delete smileobj->cMan;
    if (smileobj->logger != NULL)
      delete smileobj->logger;  
    delete smileobj;
  }
}

smileres_t smile_extsource_write_data(smileobj_t *smileobj, const char *componentName, const FLOAT_DMEM *data, int nFrames)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (componentName == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "componentName argument must not be null");
  if (data == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "data argument must not be null");
  if (smileobj->state == SMILE_UNINITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be initialized first");

  cSmileComponent *component = smileobj->cMan->getComponentInstance(componentName);

  if (component == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component does not exist");

  cExternalSource *externalSource = dynamic_cast<cExternalSource*>(component);

  if (externalSource == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component is not of type cExternalSource");

  return externalSource->writeData(data, nFrames) ? SMILE_SUCCESS : SMILE_NOT_WRITTEN;
}

smileres_t smile_extsource_set_external_eoi(smileobj_t *smileobj, const char *componentName)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (componentName == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "componentName argument must not be null");
  if (smileobj->state == SMILE_UNINITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be initialized first");

  cSmileComponent *component = smileobj->cMan->getComponentInstance(componentName);

  if (component == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component does not exist");

  cExternalSource *externalSource = dynamic_cast<cExternalSource*>(component);

  if (externalSource == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component is not of type cExternalSource");

  externalSource->setExternalEOI();

  return SMILE_SUCCESS;
}

smileres_t smile_extaudiosource_write_data(smileobj_t *smileobj, const char *componentName, const void *data, int length)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (componentName == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "componentName argument must not be null");
  if (data == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "data argument must not be null");
  if (smileobj->state == SMILE_UNINITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be initialized first");

  cSmileComponent *component = smileobj->cMan->getComponentInstance(componentName);

  if (component == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component does not exist");

  cExternalAudioSource *externalAudioSource = dynamic_cast<cExternalAudioSource*>(component);

  if (externalAudioSource == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component is not of type cExternalAudioSource");

  return externalAudioSource->writeData(data, length) ? SMILE_SUCCESS : SMILE_NOT_WRITTEN;
}

smileres_t smile_extaudiosource_set_external_eoi(smileobj_t *smileobj, const char *componentName)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (componentName == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "componentName argument must not be null");
  if (smileobj->state == SMILE_UNINITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be initialized first");

  cSmileComponent *component = smileobj->cMan->getComponentInstance(componentName);

  if (component == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component does not exist");

  cExternalAudioSource *externalAudioSource = dynamic_cast<cExternalAudioSource*>(component);

  if (externalAudioSource == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component is not of type cExternalAudioSource");

  externalAudioSource->setExternalEOI();

  return SMILE_SUCCESS;
}

smileres_t smile_extsink_set_data_callback(smileobj_t *smileobj, const char *componentName, ExternalSinkCallback callback, void *param)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (componentName == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "componentName argument must not be null");
  if (smileobj->state == SMILE_UNINITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be initialized first");

  cSmileComponent *component = smileobj->cMan->getComponentInstance(componentName);

  if (component == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component does not exist");

  cExternalSink *externalSink = dynamic_cast<cExternalSink*>(component);

  if (externalSink == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component is not of type cExternalSink");

  externalSink->setDataCallback(callback, param);

  return SMILE_SUCCESS;
}

smileres_t smile_extsink_set_data_callback_ex(smileobj_t *smileobj, const char *componentName, ExternalSinkCallbackEx callback, void *param)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (componentName == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "componentName argument must not be null");
  if (smileobj->state == SMILE_UNINITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be initialized first");

  cSmileComponent *component = smileobj->cMan->getComponentInstance(componentName);

  if (component == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component does not exist");

  cExternalSink *externalSink = dynamic_cast<cExternalSink*>(smileobj->cMan->getComponentInstance(componentName));

  if (externalSink == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component is not of type cExternalSink");

  externalSink->setDataCallbackEx(callback, param);

  return SMILE_SUCCESS;
}

smileres_t smile_extsink_get_num_elements(smileobj_t *smileobj, const char *componentName, long *numElements)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (componentName == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "componentName argument must not be null");
  if (numElements == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "numElements argument must not be null");
  if (smileobj->state == SMILE_UNINITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be initialized first");

  cSmileComponent *component = smileobj->cMan->getComponentInstance(componentName);

  if (component == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component does not exist");

  cExternalSink *externalSink = dynamic_cast<cExternalSink*>(component);

  if (externalSink == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component is not of type cExternalSink");

  *numElements = externalSink->getNumElements();
  return SMILE_SUCCESS;
}

smileres_t smile_extsink_get_element_name(smileobj_t *smileobj, const char *componentName, long idx, const char **elementName)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (componentName == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "componentName argument must not be null");
  if (elementName == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "elementName argument must not be null");
  if (smileobj->state == SMILE_UNINITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be initialized first");

  cSmileComponent *component = smileobj->cMan->getComponentInstance(componentName);

  if (component == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component does not exist");

  cExternalSink *externalSink = dynamic_cast<cExternalSink*>(component);

  if (externalSink == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component is not of type cExternalSink");

  if (idx < 0 || idx >= externalSink->getNumElements())
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "idx argument out of range");

  *elementName = externalSink->getElementName(idx);
  return SMILE_SUCCESS;
}

smileres_t smile_extmsginterface_set_msg_callback(smileobj_t *smileobj, const char *componentName, ExternalMessageInterfaceCallback callback, void *param)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (componentName == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "componentName argument must not be null");
  if (smileobj->state == SMILE_UNINITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be initialized first");

  cSmileComponent *component = smileobj->cMan->getComponentInstance(componentName);

  if (component == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component does not exist");

  cExternalMessageInterface *externalMsgInterface = dynamic_cast<cExternalMessageInterface*>(component);

  if (externalMsgInterface == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component is not of type cExternalMessageInterface");

  externalMsgInterface->setMessageCallback(callback, param);

  return SMILE_SUCCESS;
}

smileres_t smile_extmsginterface_set_json_msg_callback(smileobj_t *smileobj, const char *componentName, ExternalMessageInterfaceJsonCallback callback, void *param)
{
  if (smileobj == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "smileobj argument must not be null");
  if (componentName == NULL)
    SMILE_FAIL_WITH(SMILE_INVALID_ARG, "componentName argument must not be null");
  if (smileobj->state == SMILE_UNINITIALIZED)
    SMILE_FAIL_WITH(SMILE_INVALID_STATE, "openSMILE must be initialized first");

  cSmileComponent *component = smileobj->cMan->getComponentInstance(componentName);

  if (component == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component does not exist");

  cExternalMessageInterface *externalMsgInterface = dynamic_cast<cExternalMessageInterface*>(component);

  if (externalMsgInterface == NULL)
    SMILE_FAIL_WITH(SMILE_COMP_NOT_FOUND, "specified component is not of type cExternalMessageInterface");

  externalMsgInterface->setJsonMessageCallback(callback, param);

  return SMILE_SUCCESS;
}

const char *smile_error_msg(smileobj_t *smileobj)
{
  if (smileobj == NULL || smileobj->lastError.empty())
    return NULL;

  return smileobj->lastError.c_str();
}
