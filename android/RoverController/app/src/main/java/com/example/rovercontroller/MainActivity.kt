package com.example.rovercontroller

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import java.io.BufferedOutputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.util.concurrent.Executors

class MainActivity : AppCompatActivity() {

    companion object {
        /*
         * USB + `adb reverse tcp:5000 tcp:5000`  →  127.0.0.1
         * Android emulator                       →  10.0.2.2
         * Same Wi-Fi as the PC                   →  PC LAN IP, e.g. 192.168.1.20
         */
        private const val HOST = "127.0.0.1"
        private const val PORT = 5000
        private const val HOLD_REPEAT_MS = 100L
    }

    private lateinit var statusText: TextView
    private lateinit var buttonForward: Button
    private lateinit var buttonBackward: Button
    private lateinit var buttonLeft: Button
    private lateinit var buttonRight: Button
    private lateinit var buttonStop: Button

    private val connectExecutor = Executors.newSingleThreadExecutor()
    private val sendExecutor = Executors.newSingleThreadExecutor()
    private val handler = Handler(Looper.getMainLooper())

    @Volatile private var socket: Socket? = null
    @Volatile private var output: BufferedOutputStream? = null
    @Volatile private var connected = false

    private var heldCommand: Char? = null
    private var holdRunnable: Runnable? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        statusText = findViewById(R.id.statusText)
        buttonForward = findViewById(R.id.buttonForward)
        buttonBackward = findViewById(R.id.buttonBackward)
        buttonLeft = findViewById(R.id.buttonLeft)
        buttonRight = findViewById(R.id.buttonRight)
        buttonStop = findViewById(R.id.buttonStop)

        setupControls()
        connectLoop()
    }

    private fun setupControls() {
        setHoldButton(buttonForward, 'F')
        setHoldButton(buttonBackward, 'B')
        setHoldButton(buttonLeft, 'L')
        setHoldButton(buttonRight, 'R')

        buttonStop.setOnClickListener {
            stopHold()
            sendCommand('S')
            buttonStop.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY)
        }
    }

    private fun setHoldButton(button: Button, command: Char) {
        button.setOnTouchListener { view, event ->
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    view.isPressed = true
                    view.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY)
                    startHold(command)
                    true
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                    view.isPressed = false
                    stopHold()
                    true
                }
                else -> true
            }
        }
    }

    private fun startHold(command: Char) {
        stopHoldRepeater()
        heldCommand = command
        sendCommand(command)

        val repeater = object : Runnable {
            override fun run() {
                if (heldCommand == command) {
                    sendCommand(command)
                    handler.postDelayed(this, HOLD_REPEAT_MS)
                }
            }
        }
        holdRunnable = repeater
        handler.postDelayed(repeater, HOLD_REPEAT_MS)
    }

    private fun stopHold() {
        stopHoldRepeater()
        sendCommand('S')
    }

    private fun stopHoldRepeater() {
        heldCommand = null
        holdRunnable?.let { handler.removeCallbacks(it) }
        holdRunnable = null
    }

    private fun connectLoop() {
        connectExecutor.execute {
            while (!isFinishing && !Thread.currentThread().isInterrupted) {
                if (!connected) {
                    try {
                        val newSocket = Socket()
                        newSocket.tcpNoDelay = true
                        newSocket.keepAlive = true
                        newSocket.connect(InetSocketAddress(HOST, PORT), 1500)

                        socket = newSocket
                        output = BufferedOutputStream(newSocket.getOutputStream())
                        connected = true
                        updateStatus(true, "USB LINK: CONNECTED")
                    } catch (exception: Exception) {
                        connected = false
                        closeConnection()
                        updateStatus(
                            false,
                            "DISCONNECTED  ${HOST}:${PORT}\n${exception.message ?: ""}"
                        )
                        try {
                            Thread.sleep(1000)
                        } catch (_: InterruptedException) {
                            break
                        }
                    }
                }

                try {
                    Thread.sleep(250)
                } catch (_: InterruptedException) {
                    break
                }
            }
        }
    }

    private fun sendCommand(command: Char) {
        sendExecutor.execute {
            if (!connected) {
                return@execute
            }
            try {
                val stream = output ?: return@execute
                stream.write(byteArrayOf(command.code.toByte()))
                stream.flush()
            } catch (_: Exception) {
                connected = false
                closeConnection()
                updateStatus(false, "USB LINK: DISCONNECTED")
            }
        }
    }

    private fun closeConnection() {
        try { output?.close() } catch (_: Exception) {}
        try { socket?.close() } catch (_: Exception) {}
        output = null
        socket = null
        connected = false
    }

    private fun updateStatus(isConnected: Boolean, text: String) {
        handler.post {
            statusText.text = text
            statusText.setTextColor(
                if (isConnected) {
                    android.graphics.Color.rgb(0, 230, 118)
                } else {
                    android.graphics.Color.rgb(255, 82, 82)
                }
            )
        }
    }

    override fun onPause() {
        stopHold()
        super.onPause()
    }

    override fun onDestroy() {
        stopHold()
        closeConnection()
        connectExecutor.shutdownNow()
        sendExecutor.shutdownNow()
        super.onDestroy()
    }
}