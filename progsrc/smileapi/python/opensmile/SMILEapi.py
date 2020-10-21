'''
This file is part of openSMILE.

Copyright (c) audEERING GmbH. All rights reserved.
See the file COPYING for details on license terms.
'''

from ctypes import *
from typing import Any, Callable, Dict, List, Optional
import json
import os
import numpy as np

if os.name == 'nt':
    smileapi_path = os.path.join(os.getcwd(), "SMILEapi.dll")
else:
    smileapi_path = os.path.join(os.getcwd(), "libSMILEapi.so")
smileapi = cdll.LoadLibrary(smileapi_path)

# definitions from smileComponent.hpp
CMSG_textLen = 64
CMSG_typenameLen = 32
CMSG_nUserData = 8

class ComponentMessage(Structure):
    """
    An openSMILE component message.
    """

    _fields_ = [
        ("_msgtype", c_char * CMSG_typenameLen),
        ("_msgname", c_char * CMSG_typenameLen),
        ("_sender", c_char_p),
        ("smileTime", c_double),
        ("userTime1", c_double),
        ("userTime2", c_double),
        ("readerTime", c_double),
        ("msgid", c_int),
        ("floatData", c_double * CMSG_nUserData),
        ("intData", c_int * CMSG_nUserData),
        ("_msgtext", c_char * CMSG_textLen),
        ("userflag1", c_int),
        ("userflag2", c_int),
        ("userflag3", c_int),
        ("custData", c_void_p),
        ("custData2", c_void_p),
        ("custDataSize", c_int),
        ("custData2Size", c_int),
        ("custDataType", c_int),
        ("custData2Type", c_int)
    ]

    @property
    def msgtype(self):
        return self._msgtype.decode("utf-8")

    @property
    def msgname(self):
        return self._msgname.decode("utf-8")

    @property
    def sender(self):
        return self._sender.decode("utf-8")

    @property
    def msgtext(self):
        return self._msgtext.decode("utf-8")

    def unpack_json(self):
        """
        Unpacks a component message that wraps a JSON object and returns the JSON data as
        a dictionary.
        """
        if self.msgtype != "_CONTAINER" or self.msgname != "jsonObject":
            raise ValueError("Message does not contain JSON data")
        cust_data = cast(self.custData, c_char_p)
        if cust_data:
            return json.loads(cust_data.value.decode("ascii"))
        else:
            return None

    def __str__(self):
        return "type: {}, name: {}, sender: {}, msgtext: {}".format(
            self.msgtype,
            self.msgname,
            self.sender,
            self.msgtext)

# Success and error return codes
SMILE_SUCCESS = 0           # success
SMILE_FAIL = 1              # generic error
SMILE_INVALID_ARG = 2       # an invalid argument was passed
SMILE_INVALID_STATE = 3     # openSMILE was in an invalid state for the called function
SMILE_COMP_NOT_FOUND = 4    # component instance was not found
SMILE_LICENSE_FAIL = 5      # license validation check failed
SMILE_CONFIG_PARSE_FAIL = 6 # configuration could not be loaded
SMILE_CONFIG_INIT_FAIL = 7  # configuration could not be initialized
SMILE_NOT_WRITTEN = 8       # data could not be written to a cExternalSource/cExternalAudioSource component

# openSMILE states
SMILE_UNINITIALIZED = 0     # no configuration has been loaded yet
SMILE_INITIALIZED = 1       # a configuration has been loaded
SMILE_RUNNING = 2           # openSMILE is running
SMILE_ENDED = 3             # openSMILE has finished

# openSMILE log message types
SMILE_LOG_MESSAGE = 1
SMILE_LOG_WARNING = 2
SMILE_LOG_ERROR = 3
SMILE_LOG_DEBUG = 4
SMILE_LOG_PRINT = 5

class LogMessage(Structure):
    _fields_ = [
        ("type", c_int),
        ("level", c_int),
        ("_text", c_char_p),
        ("_module", c_char_p)
    ]

    @property
    def text(self):
        return self._text.decode("utf-8")

    @property
    def module(self):
        if self._module:
            return self._module.decode("utf-8")
        else:
            return None

    def __str__(self):
        if self.level == SMILE_LOG_PRINT:
            return self.text
        else:
            types = ["MSG", "WRN", "ERR", "DBG"]
            result = "({}) [{}]".format(types[self.type - 1], self.level)
            if self._module is not None:
                result += " " + self.module
            result += ": " + self.text
            return result

class FrameMetaData(Structure):
    _fields_ = [
        ("vIdx", c_long),
        ("time", c_double),
        ("period", c_double),
        ("lengthSec", c_double)
    ]

    def __str__(self):
        return "vIdx: {}, time: {}, period: {}, lengthSec: {}".format(
            self.vIdx,
            self.time,
            self.period,
            self.lengthSec)

LogCallback = CFUNCTYPE(None, c_void_p, LogMessage, c_void_p)
StateChangedCallback = CFUNCTYPE(None, c_void_p, c_int, c_void_p)
# TODO: return type bool does not exist in C!
ExternalSinkCallback = CFUNCTYPE(c_int, POINTER(c_float), c_long, c_void_p)
ExternalSinkCallbackEx = CFUNCTYPE(c_int, POINTER(c_float), c_long, c_long, POINTER(FrameMetaData), c_void_p)
ExternalMessageInterfaceCallback = CFUNCTYPE(c_int, POINTER(ComponentMessage), c_void_p)
ExternalMessageInterfaceJsonCallback = CFUNCTYPE(c_int, c_char_p, c_void_p)

smileapi.smile_new.argtypes = []
smileapi.smile_new.restype = c_void_p
smileapi.smile_initialize.argtypes = [c_void_p, c_char_p, c_int, c_void_p, c_int, c_int, c_int, c_void_p]
smileapi.smile_initialize.restype = c_int
smileapi.smile_run.argtypes = [c_void_p]
smileapi.smile_run.restype = c_int
smileapi.smile_abort.argtypes = [c_void_p]
smileapi.smile_abort.restype = c_int
smileapi.smile_reset.argtypes = [c_void_p]
smileapi.smile_reset.restype = c_int
smileapi.smile_set_log_callback.argtypes = [c_void_p, LogCallback, c_void_p]
smileapi.smile_set_log_callback.restype = c_int
smileapi.smile_get_state.argtypes = [c_void_p]
smileapi.smile_get_state.restype = c_int
smileapi.smile_set_state_callback.argtypes = [c_void_p, StateChangedCallback, c_void_p]
smileapi.smile_set_state_callback.restype = c_int
smileapi.smile_free.argtypes = [c_void_p]
smileapi.smile_free.restype = None
smileapi.smile_extsource_write_data.argtypes = [c_void_p, c_char_p, POINTER(c_float), c_int]
smileapi.smile_extsource_write_data.restype = c_int
smileapi.smile_extsource_set_external_eoi.argtypes = [c_void_p, c_char_p]
smileapi.smile_extsource_set_external_eoi.restype = c_int
smileapi.smile_extaudiosource_write_data.argtypes = [c_void_p, c_char_p, c_void_p, c_int]
smileapi.smile_extaudiosource_write_data.restype = c_int
smileapi.smile_extaudiosource_set_external_eoi.argtypes = [c_void_p, c_char_p]
smileapi.smile_extaudiosource_set_external_eoi.restype = c_int
smileapi.smile_extsink_set_data_callback.argtypes = [c_void_p, c_char_p, ExternalSinkCallback, c_void_p]
smileapi.smile_extsink_set_data_callback.restype = c_int
smileapi.smile_extsink_set_data_callback_ex.argtypes = [c_void_p, c_char_p, ExternalSinkCallbackEx, c_void_p]
smileapi.smile_extsink_set_data_callback_ex.restype = c_int
smileapi.smile_extsink_get_num_elements.argtypes = [c_void_p, c_char_p, POINTER(c_long)]
smileapi.smile_extsink_get_num_elements.restype = c_int
smileapi.smile_extsink_get_element_name.argtypes = [c_void_p, c_char_p, c_long, POINTER(c_char_p)]
smileapi.smile_extsink_get_element_name.restype = c_int
smileapi.smile_extmsginterface_set_msg_callback.argtypes = [c_void_p, c_char_p, ExternalMessageInterfaceCallback, c_void_p]
smileapi.smile_extmsginterface_set_msg_callback.restype = c_int
smileapi.smile_extmsginterface_set_json_msg_callback.argtypes = [c_void_p, c_char_p, ExternalMessageInterfaceJsonCallback, c_void_p]
smileapi.smile_extmsginterface_set_json_msg_callback.restype = c_int
smileapi.smile_error_msg.argtypes = [c_void_p]
smileapi.smile_error_msg.restype = c_char_p

def c_char_p_arr(l):
    arr = (c_char_p * (len(l) + 1))()
    arr[:-1] = l
    arr[-1] = None
    return arr


class OpenSmileException(Exception):
    """
    Exception thrown for internal openSMILE errors.
    """

    def __init__(self, code: int, message: Optional[str] = None):
        self.code = code
        self.message = message

    def __str__(self):
        if self.message:
            return "Code: {}, Message: {}".format(self.code, self.message)
        else:
            return "Code: {}".format(self.code)


class OpenSMILE(object):
    """
    The main class implementing the interface to openSMILE.
    """

    def __init__(self):
        self._smileobj = smileapi.smile_new()
        if self._smileobj is None:
            raise OpenSmileException(SMILE_FAIL, "could not create new SMILEapi object")
        self._callbacks = []

    def initialize(self, config_file: str, options: Dict[str, Any]=None, loglevel: int=2, debug: bool=False,
                   console_output: bool=False, log_file: str = None):
        """
        Initializes openSMILE with the provided config file and command-line options.
        """
        options_flat = list(map(lambda v: bytes(str(v), "ascii"), sum(options.items(), ())))
        options_char_arr = c_char_p_arr(options_flat)
        log_file = bytes(log_file, "ascii") if log_file else int(0)
        self._check_smile_result(smileapi.smile_initialize(self._smileobj, bytes(config_file, "ascii"), len(options), options_char_arr, loglevel, int(debug), int(console_output), log_file))

    def external_source_write_data(self, component_name: str, data: np.ndarray) -> bool:
        """
        Writes a data buffer to the specified instance of a cExternalSource component.

        Returns True if the data was written successfully, otherwise returns False
        (e.g. if the internal buffer of the component is full).
        """
        if len(data.shape) != 1:
            raise ValueError("data parameter must have exactly one dimension")
        if data.dtype.name != "float32":
            raise ValueError("data parameter must have dtype float32")
        data_p = data.ctypes.data_as(POINTER(c_float))
        result = smileapi.smile_extsource_write_data(self._smileobj, bytes(component_name, "ascii"), data_p, len(data))
        if result == SMILE_SUCCESS:
            return True
        elif result == SMILE_NOT_WRITTEN:
            return False
        else:
            self._check_smile_result(result)

    def external_source_set_eoi(self, component_name: str):
        """
        Signals the end of the input for the specified cExternalSource component instance.
        Attempts to write more data to the component after calling this method will fail.

        Returns True if the end-of-input signal was set successfully, otherwise False.
        """
        self._check_smile_result(smileapi.smile_extsource_set_external_eoi(self._smileobj, bytes(component_name, "ascii")))

    def external_audio_source_write_data(self, component_name: str, data: bytes) -> bool:
        """
        Writes a data buffer to the specified instance of a cExternalAudioSource component.
        The data must match the specified data format for the component (sample size,
        number of channels, etc.).

        Returns True if the data was written successfully, otherwise returns False
        (e.g. if the internal buffer of the component is full).
        """
        result = smileapi.smile_extaudiosource_write_data(self._smileobj, bytes(component_name, "ascii"), data, len(data))
        if result == SMILE_SUCCESS:
            return True
        elif result == SMILE_NOT_WRITTEN:
            return False
        else:
            self._check_smile_result(result)

    def external_audio_source_set_eoi(self, component_name: str):
        """
        Signals the end of the input for the specified cExternalAudioSource component instance.
        Attempts to write more data to the component after calling this method will fail.

        Returns True if the end-of-input signal was set successfully, otherwise False.
        """
        self._check_smile_result(smileapi.smile_extaudiosource_set_external_eoi(self._smileobj, bytes(component_name, "ascii")))

    def external_sink_set_callback(self, component_name: str, callback: Callable[[np.ndarray], None]):
        """
        Sets the callback function for the specified cExternalSink component instance.
        The function will get called whenever another openSMILE component writes data to
        the cExternalSink component.
        """
        def internal_callback(data, vector_size, param):
            numpy_array = np.ctypeslib.as_array(data, shape=(vector_size,))
            callback(numpy_array)
            return 1
        cb = ExternalSinkCallback(internal_callback)
        # we need to keep a reference to any callback objects as otherwise they may get garbage-collected
        self._callbacks.append(cb)
        self._check_smile_result(smileapi.smile_extsink_set_data_callback(self._smileobj, bytes(component_name, "ascii"), cb, None))

    def external_sink_set_callback_ex(self, component_name: str, callback: Callable[[np.ndarray], None]):
        """
        Sets the extended callback function for the specified cExternalSink component instance.
        The function will get called whenever another openSMILE component writes data to
        the cExternalSink component.
        """
        def internal_callback_ex(data, nt, n, meta: POINTER(FrameMetaData), _):
            numpy_array = np.ctypeslib.as_array(data, shape=(nt, n))
            callback(numpy_array, meta.contents)
            return 1
        cb = ExternalSinkCallbackEx(internal_callback_ex)
        # we need to keep a reference to any callback objects as otherwise they may get garbage-collected
        self._callbacks.append(cb)
        self._check_smile_result(smileapi.smile_extsink_set_data_callback_ex(self._smileobj, bytes(component_name, "ascii"), cb, None))

    def external_sink_get_num_elements(self, component_name: str) -> int:
        num_elements = c_long()
        self._check_smile_result(smileapi.smile_extsink_get_num_elements(self._smileobj, bytes(component_name, "ascii"), byref(num_elements)))
        return num_elements.value

    def external_sink_get_element_name(self, component_name: str, idx: int) -> str:
        element_name = c_char_p()
        self._check_smile_result(smileapi.smile_extsink_get_element_name(self._smileobj, bytes(component_name, "ascii"), idx, byref(element_name)))
        return element_name.value.decode("ascii")

    def external_message_interface_set_callback(self, component_name: str, callback: Callable[[ComponentMessage], None]):
        """
        Sets the callback function for the specified cExternalMessageInterface component instance.
        The function will get called whenever the component receives a message.
        """
        # we need to keep a reference to any callback objects as otherwise they may get garbage-collected
        def internal_callback(message: POINTER(ComponentMessage), param):
            callback(message.contents)
            return 1
        cb = ExternalMessageInterfaceCallback(internal_callback)
        self._callbacks.append(cb)
        self._check_smile_result(smileapi.smile_extmsginterface_set_msg_callback(self._smileobj, bytes(component_name, "ascii"), cb, None))

    def external_message_interface_set_json_callback(self, component_name: str, callback: Callable[[Dict], None]):
        """
        Sets the callback function for the specified cExternalMessageInterface component instance.
        The function will get called whenever the component receives a message.
        """
        # we need to keep a reference to any callback objects as otherwise they may get garbage-collected
        def internal_callback(json_message: bytes, param):
            callback(json.loads(json_message.decode("ascii")))
            return 1
        cb = ExternalMessageInterfaceJsonCallback(internal_callback)
        self._callbacks.append(cb)
        self._check_smile_result(smileapi.smile_extmsginterface_set_json_msg_callback(self._smileobj, bytes(component_name, "ascii"), cb, None))

    def set_log_callback(self, callback: Callable[[LogMessage], None]):
        """
        Sets a callback function for log messages.
        The function will get called whenever the openSMILE generates a log message.
        """
        # we need to keep a reference to any callback objects as otherwise they may get garbage-collected
        def internal_callback(smileobj, message: LogMessage, param):
            callback(message)
        cb = LogCallback(internal_callback)
        self._callbacks.append(cb)
        self._check_smile_result(smileapi.smile_set_log_callback(self._smileobj, cb, None))

    def run(self):
        """
        Starts processing and blocks until finished.
        """
        self._check_smile_result(smileapi.smile_run(self._smileobj))

    def abort(self):
        """
        Requests abortion of the current run.

        Note that openSMILE does not immediately stop after this function returns.
        It might continue to run for a short while until the run method returns.
        """
        self._check_smile_result(smileapi.smile_abort(self._smileobj))

    def reset(self):
        """
        Resets the internal state of openSMILE after a run has finished or was aborted.
        After resetting, you may call 'run' again without the need to call 'initialize' first.
        You must re-register any cExternalSink/cExternalMessageInterface callbacks, though.
        """
        self._check_smile_result(smileapi.smile_reset(self._smileobj))

    def free(self):
        """
        Frees any internal resources allocated by openSMILE.
        """
        if self._smileobj is not None:
            smileapi.smile_free(self._smileobj)
            self._smileobj = None

    def _check_smile_result(self, result: int):
        if result != SMILE_SUCCESS:
            message = smileapi.smile_error_msg(self._smileobj)
            if message is None or len(message) == 0:
                raise OpenSmileException(result)
            else:
                raise OpenSmileException(result, message.decode("ascii"))

    @staticmethod
    def process(config_file: str, options: Dict[str, Any],
                inputs: Dict[str, np.ndarray], outputs: List[str]) -> Dict[str, np.ndarray]:
        """
        Runs the specified config file on a set of input data buffers and returns the
        specified set of output buffers.
        """
        opensmile = OpenSMILE()
        opensmile.initialize(config_file, options)

        for (input, data) in inputs.items():
            if not opensmile.external_source_write_data(input, data):
                raise Exception("Could not write input data to component '{}'".format(input))
            opensmile.external_source_set_eoi(input)

        output_data = {}

        for output in outputs:
            def callback(data: np.ndarray):
                if output not in output_data:
                    output_data[output] = np.copy(data)
                else:
                    output_data[output] = np.vstack((output_data[output], data))
            opensmile.external_sink_set_callback(output, callback)

        opensmile.run()
        opensmile.free()
        return output_data
