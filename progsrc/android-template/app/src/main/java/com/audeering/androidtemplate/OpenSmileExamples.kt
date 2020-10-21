/**
 * Copyright (c) audEERING GmbH.
 * All rights reserved.
 *
 * Author: Hesam Sagha
 * E-mail: hsagha@audeering.com
 * Created: 07.02.20
 */

package com.audeering.androidtemplate

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.util.Log
import com.audeering.opensmile.CallbackExternalMessageInterfaceJson
import com.audeering.opensmile.CallbackExternalSink
import com.audeering.opensmile.OpenSmileAdapter
import com.audeering.opensmile.smileres_t
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import java.io.IOException
import kotlin.concurrent.thread

interface OpensmileInterface {
    fun started()
    fun ended()
    fun messageCallback(message: String)
    fun crashed(error: String)
    fun getCacheDirString(): String
    fun getOutputDirString(): String
}

abstract class OpenSmileExamples(var opensmileInterface: OpensmileInterface) {
    protected enum class MODE { SLES, EXTAUDIOSOURCE, EXTDATASOURCE, EXTSINK }

    companion object {
        val openSmileAssets = listOf(
                "testsles.conf",
                "testexternalaudio.conf",
                "testexternalsource.conf",
                "testexternalsink.conf",
                "testsmilemessage.conf")

        fun build(mode: String, opensmileInterface: OpensmileInterface): OpenSmileExamples {
            return when (mode) {
                "SLES" -> SLES(opensmileInterface)
                "EXTAUDIOSOURCE" -> RecordMic(opensmileInterface)
                "EXTDATASOURCE" -> WriteDataToOpenSMILE(opensmileInterface)
                "JSONCALLBACK" -> JSONCallback(opensmileInterface)
                "EXTSINK" -> DataCallback(opensmileInterface)
                else -> SLES(opensmileInterface)
            }
        }
    }

    protected var ose = OpenSmileAdapter()

    protected abstract val recordingMode: MODE
    protected abstract val text: String
    protected abstract val mainConfig: String
    protected abstract val params: Map<String, String>

    protected var jsonMessageListenerCallbacks: Map<String, Any?>? = null
    protected var dataListenerCallbacks: Map<String, Any?>? = null

    var isRunning = false

    open fun start() {
        start(mainConfig, params, jsonMessageListenerCallbacks, dataListenerCallbacks)
    }

    private fun start(mainConfig: String, params: Map<String, String?>?,
                      jsonMessageListenerCallbacks: Map<String, Any?>?,
                      dataListenerCallbacks: Map<String, Any?>?) {
        try {
            startRecordingAsync(
                    "${opensmileInterface.getCacheDirString()}/$mainConfig",
                    params,
                    jsonMessageListenerCallbacks,
                    dataListenerCallbacks)
        } catch (e: Exception) {
            log(e.message!!)
            opensmileInterface.crashed(e.message ?: "")
            isRunning = false
        }
        opensmileInterface.started()
    }


    private fun startRecordingAsync(mainConfig: String,
                                    params: Map<String, String?>?,
                                    callbacksMessageListener: Map<String, Any?>?,
                                    callbacksDataListener: Map<String, Any?>?,
                                    loglevel: Int = 3,
                                    debug: Int = 1,
                                    consoleOutput: Int = 1) {
        val configs = if (params == null) HashMap() else HashMap(params.toMutableMap())
        var state = ose.smile_initialize(mainConfig, configs, loglevel, debug, consoleOutput)
        throwIfNotSuccess(state, "initialize")
        callbacksMessageListener?.forEach {
            val cb = it.value as CallbackExternalMessageInterfaceJson
            state = ose.smile_extmsginterface_set_msg_callback(it.key, cb)
            throwIfNotSuccess(state, "smile_extmsginterface_set_msg_callback")
        }
        callbacksDataListener?.forEach {
            val cb = it.value as CallbackExternalSink
            state = ose.smile_extsink_set_data_callback(it.key, cb)
            throwIfNotSuccess(state, "smile_extsink_set_data_callback")
        }
        thread(start = true) {
            state = ose.smile_run()
            throwIfNotSuccess(state, "smile_run")
            isRunning = false
            opensmileInterface.ended()
        }
        isRunning = true
        Thread.sleep(100)
    }

    open fun stop() {
        ose.smile_abort()
        ose.smile_free()
    }

    private fun throwIfNotSuccess(state: smileres_t, text: String) {
        if (state != smileres_t.SMILE_SUCCESS)
            throw java.lang.Exception("$text failed!")
    }

    protected fun log(str: String) = Log.i(recordingMode.toString(), str)
}

class SLES(opensmileInterface: OpensmileInterface) : OpenSmileExamples(opensmileInterface) {
    override val recordingMode = MODE.SLES
    override val text = "OpenSMILE recording from SLES"
    override val mainConfig = "testsles.conf"
    override val params = mapOf("-O" to opensmileInterface.getOutputDirString() + "output_sles.wav")
}

class RecordMic(opensmileInterface: OpensmileInterface) : OpenSmileExamples(opensmileInterface) {
    override val recordingMode = MODE.EXTAUDIOSOURCE
    override val text = "OpenSMILE recording from External Audio"
    override val mainConfig = "testexternalaudio.conf"
    override val params = mapOf("-O" to opensmileInterface.getOutputDirString() + "output_external.wav")

    companion object {
        private const val BufferElements2Rec = 1024 // 1024 elements == 2048 bytes
        private const val BytesPerElement = 2 // 2 bytes in 16bit format
        private const val RECORDER_SAMPLE_RATE = 16000
        private const val RECORDER_CHANNELS = AudioFormat.CHANNEL_IN_MONO
        private const val RECORDER_AUDIO_ENCODING = AudioFormat.ENCODING_PCM_16BIT
    }

    private var recorder: AudioRecord? = null
    private var recordingThread: Thread? = null

    private var recordingFlag = false

    override fun start() {
        super.start()
        recordingFlag = true
        startRecording()
    }

    private fun startRecording() {
        recorder = AudioRecord(MediaRecorder.AudioSource.MIC,
                RECORDER_SAMPLE_RATE, RECORDER_CHANNELS, RECORDER_AUDIO_ENCODING,
                BufferElements2Rec * BytesPerElement)
        recorder?.startRecording()
        recordingThread = Thread(Runnable { writeAudioDataToExternalAudioSource() }, "AudioRecorder Thread")
        recordingThread?.start()
    }

    private fun writeAudioDataToExternalAudioSource() {
        val sData = ByteArray(BufferElements2Rec)
        GlobalScope.launch {
            Thread.sleep(100)
            while (recordingFlag) {
                recorder?.read(sData, 0, BufferElements2Rec, AudioRecord.READ_BLOCKING)
                try {
                    ose.smile_extaudiosource_write_data("externalAudioSource", sData)
                } catch (e: IOException) {
                    e.printStackTrace()
                }
            }
        }
    }

    override fun stop() {
        recordingFlag = false
        ose.smile_extaudiosource_set_external_eoi("externalAudioSource")
        stopRecording()
        super.stop()
    }

    private fun stopRecording() {
        recorder ?: return
        recorder?.stop()
        recorder?.release()
        recorder = null
        recordingThread = null
    }
}

class WriteDataToOpenSMILE(opensmileInterface: OpensmileInterface) : OpenSmileExamples(opensmileInterface) {
    override val recordingMode = MODE.EXTDATASOURCE
    override val text = "Sending data to openSMILE and writing to csv file"
    override val mainConfig = "testexternalsource.conf"
    override val params = mapOf("-O" to opensmileInterface.getOutputDirString() + "output_external_source.csv")
    private var recordingFlag = false

    override fun start() {
        super.start()
        recordingFlag = true
        writeDataToExternalSource()
    }

    override fun stop() {
        recordingFlag = false
        ose.smile_extsource_set_external_eoi("externalSource")
        super.stop()
    }

    private fun writeDataToExternalSource() = GlobalScope.launch {
        Thread.sleep(100)
        while (recordingFlag) {
            val dummyData = FloatArray(8)
            for (i in 0 until 8) dummyData[i] = i + Math.random().toFloat()
            try {
                ose.smile_extsource_write_data("externalSource", dummyData)
            } catch (e: IOException) {
                e.printStackTrace()
            }
            Thread.sleep(100)
        }
    }
}

class JSONCallback(opensmileInterface: OpensmileInterface) : OpenSmileExamples(opensmileInterface) {
    override val recordingMode = MODE.EXTDATASOURCE
    override val text = "OpenSMILE results as External Message"
    override val mainConfig = "testsmilemessage.conf"
    override val params: Map<String, String> = mapOf()

    override fun start() {
        class CBEMI : CallbackExternalMessageInterfaceJson() {
            override fun onCalledExternalMessageInterfaceJsonCallback(msg: String?): Boolean {
                opensmileInterface.messageCallback(msg ?: "")
                return true
            }
        }
        jsonMessageListenerCallbacks = mapOf("externalMessageInterface" to CBEMI())
        super.start()
    }

    override fun stop() {
        ose.smile_extsource_set_external_eoi("externalSource")
        super.stop()
    }
}

class DataCallback(opensmileInterface: OpensmileInterface) : OpenSmileExamples(opensmileInterface) {
    override val recordingMode = MODE.EXTSINK
    override val text = "OpenSMILE results as Data Sink"
    override val mainConfig = "testexternalsink.conf"
    override val params: Map<String, String> = mapOf()

    override fun start() {
        class CBES : CallbackExternalSink() {
            override fun onCalledExternalSinkCallback(data: FloatArray?): Boolean {
                opensmileInterface.messageCallback(data?.contentToString() ?: "")
                return true
            }
        }
        dataListenerCallbacks = mapOf("externalSink" to CBES())
        super.start()
    }
}