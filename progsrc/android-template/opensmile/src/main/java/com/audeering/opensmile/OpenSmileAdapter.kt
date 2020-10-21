/**
 * Copyright (c) audEERING GmbH.
 * All rights reserved.
 *
 * Author: Hesam Sagha
 * E-mail: hsagha@audeering.com
 * Created: 20.12.19
 */

package com.audeering.opensmile

class OpenSmileAdapter {
    private lateinit var smileobj: SWIGTYPE_p_smileobj_t_

    fun smile_get_state() = OpenSMILE.smile_get_state(smileobj)

    fun smile_extaudiosource_set_external_eoi(componentName: String) =
            OpenSMILE.smile_extaudiosource_set_external_eoi(smileobj, componentName)

    fun smile_extsource_set_external_eoi(componentName: String) =
            OpenSMILE.smile_extsource_set_external_eoi(smileobj, componentName)

    fun smile_extsource_write_data(componentName: String, data: FloatArray) =
            OpenSMILE.smile_extsource_write_data(smileobj, componentName, data)

    fun smile_extaudiosource_write_data(componentName: String, data: ByteArray) =
            OpenSMILE.smile_extaudiosource_write_data(smileobj, componentName, data)

    fun smile_initialize(configFile: String,
                         nOptions: HashMap<String, String?>,
                         loglevel: Int,
                         debug: Int,
                         consoleOutput: Int): smileres_t {

        smileobj = OpenSMILE.smile_new()
        return OpenSMILE.smile_initialize(smileobj,
                configFile, nOptions,
                loglevel, debug, consoleOutput, null)
    }

    fun smile_extmsginterface_set_msg_callback(
            componentName: String, callbackDataListener: CallbackExternalMessageInterfaceJson
    ) =
            OpenSMILE.smile_extmsginterface_set_json_msg_callback(
                    smileobj,
                    componentName,
                    callbackDataListener.createExternalMessageInterfaceJsonCallback(),
                    CallbackExternalMessageInterfaceJson.getCPtr(callbackDataListener)
            )

    fun smile_extsink_set_data_callback(componentName: String,
                                        callbackExternalSink: CallbackExternalSink
    ) =
            OpenSMILE.smile_extsink_set_data_callback(
                    smileobj,
                    componentName,
                    callbackExternalSink.createExternalSinkCallback(),
                    CallbackExternalSink.getCPtr(callbackExternalSink)
            )

    fun smile_reset() = OpenSMILE.smile_reset(smileobj)

    fun smile_abort() = OpenSMILE.smile_abort(smileobj)

    fun smile_free() = OpenSMILE.smile_free(smileobj)

    fun smile_run() = OpenSMILE.smile_run(smileobj)
}