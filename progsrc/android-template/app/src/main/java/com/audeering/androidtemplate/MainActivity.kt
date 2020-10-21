/**
 * Copyright (c) audEERING GmbH.
 * All rights reserved.
 *
 * Author: Hesam Sagha
 * E-mail: hsagha@audeering.com
 * Created: 29.06.18
 */

package com.audeering.androidtemplate

import android.Manifest.permission.*
import android.content.pm.PackageManager.PERMISSION_DENIED
import android.media.MediaPlayer
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Button
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import kotlinx.android.synthetic.main.main.*
import java.io.File
import java.io.FileOutputStream
import kotlin.concurrent.thread

@Suppress("UNUSED_PARAMETER")
class MainActivity : AppCompatActivity(), OpensmileInterface {
    companion object {
        private const val TAG = "MainActivity"
        private val permissions = arrayOf(RECORD_AUDIO, WRITE_EXTERNAL_STORAGE, READ_EXTERNAL_STORAGE)
        private const val PERMISSION_REQUEST = 0
    }

    private var mediaPlayer: MediaPlayer? = null
    private var isPlaying = false
    private lateinit var allBtns: List<Button>
    private var osExample: OpenSmileExamples? = null

    private val osIsRunning: Boolean
        get() = osExample != null && osExample!!.isRunning

    private val sdcard = "/sdcard"
    private val slesAudioPath = "$sdcard/output_sles.wav"
    private val extRecAudioPath = "$sdcard/output_external.wav"
    private val extSrcFilePath = "$sdcard/output_external_source.csv"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.main)
        setupAssets()
        allBtns = arrayListOf(btn_end, btn_sles_rec, btn_sles_play,
                btn_ext_rec, btn_ext_play, btn_ext_source, btn_ext_show_file, btn_ext_message,
                btn_ext_sink)
    }

    private fun checkEnabling() {
        allBtns.forEach { it.isEnabled = false }
        val recordingSth = if (osExample == null) false else osExample!!.isRunning
        if (!recordingSth && mediaPlayer == null) {
            btn_ext_rec.isEnabled = true
            btn_ext_play.isEnabled = true
            btn_sles_play.isEnabled = true
            btn_sles_rec.isEnabled = true
            btn_ext_source.isEnabled = true
            btn_ext_message.isEnabled = true
            btn_ext_sink.isEnabled = true
        }
        if (recordingSth)
            btn_end.isEnabled = true
        if (File(slesAudioPath).exists() && !recordingSth)
            btn_sles_play.isEnabled = true
        if (File(extRecAudioPath).exists() && !recordingSth)
            btn_ext_play.isEnabled = true
        if (File(extSrcFilePath).exists() && !recordingSth)
            btn_ext_show_file.isEnabled = true
    }

    override fun started() = runOnUiThread {
        checkEnabling()
    }

    override fun ended() = runOnUiThread {
        checkEnabling()
        tv_status.text = "OpenSMILE ended"
    }

    override fun crashed(error: String) = runOnUiThread {
        tv_status.text = error
    }

    override fun messageCallback(message: String) = runOnUiThread {
        tv_data.text = message
    }

    override fun getCacheDirString() = cacheDir.absolutePath + "/"
    override fun getOutputDirString() = "$sdcard/"

    fun onBtnEndClicked(v: View?) = osExample?.stop()

    fun onBtnSlesClicked(v: View?) = runOpenSmile("SLES")

    fun onBtnExtRecClicked(v: View?) = runOpenSmile("EXTAUDIOSOURCE")

    fun onBtnExtSourceClicked(v: View?) = runOpenSmile("EXTDATASOURCE")

    fun onBtnExtMessageClicked(v: View?) = runOpenSmile("JSONCALLBACK")

    fun onBtnExtSinkClicked(v: View?) = runOpenSmile("EXTSINK")

    private fun runOpenSmile(mode: String) {
        if (osIsRunning) return
        osExample = OpenSmileExamples.build(mode, this)
        osExample?.start()
    }

    fun onBtnSlesPlayClicked(v: View?) {
        if (!isPlaying) playAudio(slesAudioPath)
    }

    fun onBtnExtRecPlayClicked(v: View?) {
        if (!isPlaying) playAudio(extRecAudioPath)
    }

    fun onBtnExtShowFileClicked(v: View?) {
        val file = File(extSrcFilePath)
        val text = file.readLines()
        var finalText = ""
        text.forEach { finalText += it + "\n" }
        tv_data.text = finalText
    }

    private fun playAudio(path: String) {
        mediaPlayer = MediaPlayer()
        mediaPlayer?.setDataSource(applicationContext, Uri.parse(path))
        mediaPlayer?.prepare()
        mediaPlayer?.setOnCompletionListener {
            mediaPlayer?.release()
            mediaPlayer = null
            checkEnabling()
        }
        mediaPlayer?.start()
        checkEnabling()
    }

    /**
     * show a loading popup while the openSMILE conf-files are being loaded
     * if the required permissions are not granted yet, it covers that, too
     */
    private fun setupAssets() {
        val grants = permissions.map { ContextCompat.checkSelfPermission(this, it) }
        if (grants.contains(PERMISSION_DENIED))
            ActivityCompat.requestPermissions(this, permissions, PERMISSION_REQUEST)
        else
            thread(start = true) { OpenSmileExamples.openSmileAssets.map { cacheAsset(it) } }
    }

    override fun onRequestPermissionsResult(request: Int, permissions: Array<String>, grants: IntArray) {
        val denied = request != PERMISSION_REQUEST || grants.contains(PERMISSION_DENIED)
        val msg = if (denied) "Permission denied" else "Permission granted"
        log(msg)
        setupAssets()
    }

    /**
     * copies a file to a given destination
     * @param filename the file to be copied
     * @param dst destination directory (default: cacheDir)
     */
    private fun cacheAsset(filename: String, dst: String = cacheDir.absolutePath) {
        val pathname = "$dst/$filename"
        val outfile = File(pathname).apply { parentFile?.mkdirs() }
        assets.open(filename).use { inputStream ->
            FileOutputStream(outfile).use { outputStream ->
                inputStream.copyTo(outputStream)
            }
        }
    }

    private fun log(str: String) = Log.i(TAG, str)
}
