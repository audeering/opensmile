/*F***************************************************************************
 * This file is part of openSMILE.
 *
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#ifndef SMILEAPI_H
#define SMILEAPI_H

#ifdef __cplusplus
extern "C" {
#endif

// Success and error return codes.
typedef enum {
  SMILE_SUCCESS,           // success
  SMILE_FAIL,              // generic error
  SMILE_INVALID_ARG,       // an invalid argument was passed
  SMILE_INVALID_STATE,     // openSMILE was in an invalid state for the called function
  SMILE_COMP_NOT_FOUND,    // component instance was not found
  SMILE_LICENSE_FAIL,      // license validation check failed
  SMILE_CONFIG_PARSE_FAIL, // configuration could not be loaded
  SMILE_CONFIG_INIT_FAIL,  // configuration could not be initialized
  SMILE_NOT_WRITTEN,       // data could not be written to a cExternalSource/cExternalAudioSource component
} smileres_t;

// openSMILE states.
typedef enum {
  SMILE_UNINITIALIZED,     // no configuration has been loaded yet
  SMILE_INITIALIZED,       // a configuration has been loaded
  SMILE_RUNNING,           // openSMILE is running
  SMILE_ENDED              // openSMILE has finished
} smilestate_t;

// Handle to an openSMILE instance.
typedef struct smileobj_t_ smileobj_t;

// Type of log message. Equivalent to LOG_* definitions in smileLogger.hpp.
typedef enum {
  SMILE_LOG_MESSAGE = 1,
  SMILE_LOG_WARNING = 2,
  SMILE_LOG_ERROR = 3,
  SMILE_LOG_DEBUG = 4,
  SMILE_LOG_PRINT = 5
} smilelogtype_t;

// Represents an openSMILE log message.
typedef struct {
  smilelogtype_t type;     // type of log message
  int level;               // log level
  const char *text;        // message text
  const char *module;      // module where this message originates from, may be NULL
} smilelogmsg_t;

// Callback type for openSMILE log messages.
typedef void (*LogCallback)(smileobj_t *smileobj, smilelogmsg_t message, void *param);

// Callback type for signaling openSMILE state changes.
typedef void (*StateChangedCallback)(smileobj_t *smileobj, smilestate_t state, void *param);

// Callback type for data written to a cExternalSink component.
typedef bool (*ExternalSinkCallback)(const float *data, long vectorSize, void *param);
struct sExternalSinkMetaDataEx;
typedef bool (*ExternalSinkCallbackEx)(const float *data, long nT, long N, const sExternalSinkMetaDataEx *metaData, void *param);

// Callback type for messages received by a cExternalMessageInterface component (deprecated).
// This callback variant receives the messages as native struct.
class cComponentMessage;
typedef bool (*ExternalMessageInterfaceCallback)(const cComponentMessage *msg, void *param);

// Callback type for messages received by a cExternalMessageInterface component.
// This callback variant receives the messages formatted as JSON.
typedef bool (*ExternalMessageInterfaceJsonCallback)(const char *msg, void *param);

// Represents a config option name-value pair.
typedef struct {
  const char *name;
  const char *value;
} smileopt_t;

// Creates a new smileobj_t instance.
smileobj_t *smile_new();

// Initializes openSMILE with the provided config file and command-line options.
smileres_t smile_initialize(smileobj_t *smileobj, const char *configFile, int nOptions, const smileopt_t *options, int loglevel=2, int debug=0, int consoleOutput=0, const char *logFile=0);

// Starts processing and blocks until finished.
smileres_t smile_run(smileobj_t *smileobj);

// Requests abortion of the current run.
// Note that openSMILE does not immediately stop after this function returns.
// It might continue to run for a short while until the run method returns.
smileres_t smile_abort(smileobj_t *smileobj);

// Resets the internal state of openSMILE after a run has finished or was aborted.
// After resetting, you may call 'smile_run' again without the need to call 'smile_initialize' first.
// You must re-register any cExternalSink/cExternalMessageInterface callbacks, though.
smileres_t smile_reset(smileobj_t *smileobj);

// Registers a callback function to be invoked for each log message.
smileres_t smile_set_log_callback(smileobj_t *smileobj, LogCallback callback, void *param);

// Returns the current state of the openSMILE instance.
smilestate_t smile_get_state(smileobj_t *smileobj);

// Registers a callback function to be invoked for each state change of the openSMILE instance.
smileres_t smile_set_state_callback(smileobj_t *smileobj, StateChangedCallback callback, void *param);

// Frees any internal resources allocated by openSMILE.
void smile_free(smileobj_t *smileobj);

// Writes a data buffer to the specified instance of a cExternalSource component.
// Returns SMILE_SUCCESS if the data was written successfully. If no data could be written
// (e.g. because the internal buffer of the component is currently full), SMILE_NOT_WRITTEN is returned.
smileres_t smile_extsource_write_data(smileobj_t *smileobj, const char *componentName, const float *data, int nFrames);

// Signals the end of the input for the specified cExternalSource component instance.
// Attempts to write more data to the component after calling this method will fail.
smileres_t smile_extsource_set_external_eoi(smileobj_t *smileobj, const char *componentName);

// Writes a data buffer to the specified instance of a cExternalAudioSource component.
// The data must match the specified data format for the component (sample size, number of channels, etc.).
// Returns SMILE_SUCCESS if the data was written successfully. If no data could be written
// (e.g. because the internal buffer of the component is currently full), SMILE_NOT_WRITTEN is returned.
smileres_t smile_extaudiosource_write_data(smileobj_t *smileobj, const char *componentName, const void *data, int length);

// Signals the end of the input for the specified cExternalAudioSource component instance.
// Attempts to write more data to the component after calling this method will fail.
smileres_t smile_extaudiosource_set_external_eoi(smileobj_t *smileobj, const char *componentName);

// Sets the callback function for the specified cExternalSink component instance.
// The function will get called whenever another openSMILE component writes data to the cExternalSink component.
smileres_t smile_extsink_set_data_callback(smileobj_t *smileobj, const char *componentName, ExternalSinkCallback callback, void *param);
smileres_t smile_extsink_set_data_callback_ex(smileobj_t *smileobj, const char *componentName, ExternalSinkCallbackEx callback, void *param);

// Returns the number of elements for a specific component
smileres_t smile_extsink_get_num_elements(smileobj_t *smileobj, const char *componentName, long *numElements);
// Returns the names of the elements for a specific component
smileres_t smile_extsink_get_element_name(smileobj_t *smileobj, const char *componentName, long idx, const char **elementName);

// Sets the callback function for the specified cExternalMessageInterface component instance (deprecated).
// The function will get called whenever the component receives a message.
// This callback variant receives the messages as native struct.
smileres_t smile_extmsginterface_set_msg_callback(smileobj_t *smileobj, const char *componentName, ExternalMessageInterfaceCallback callback, void *param);

// Sets the callback function for the specified cExternalMessageInterface component instance.
// The function will get called whenever the component receives a message.
// This callback variant receives the messages formatted as JSON.
smileres_t smile_extmsginterface_set_json_msg_callback(smileobj_t *smileobj, const char *componentName, ExternalMessageInterfaceJsonCallback callback, void *param);

// Returns a description for the last error occurred, or NULL if no error message is available.
const char *smile_error_msg(smileobj_t *smileobj);

#ifdef __cplusplus
}
#endif

#endif
