package com.example.performic

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.BatteryManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.WindowManager
import androidx.annotation.RequiresApi
import com.example.performic.record.ThermalPoint
import com.google.gson.Gson
import java.util.Collections
import java.util.concurrent.atomic.AtomicBoolean

class BenchmarkManager(private val context: Context) {

    private external fun runNativeBenchmark(): String

    private fun getCurrentBatteryTemp(): Float {
        val intent = context.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
        val tempInt = intent?.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, 0) ?: 0
        return tempInt / 10.0f
    }

    fun runCoreBenchmarkWithMonitoring(
        onComplete: (BenchmarkResult, List<ThermalPoint>) -> Unit
    ) {
        val thermalHistory = Collections.synchronizedList(ArrayList<ThermalPoint>())
        val isBenchmarkRunning = AtomicBoolean(true)
        val startTime = System.currentTimeMillis()

        // ---------------------------------------------------------
        // THREAD 1: TEMPERATURE MONITOR
        // ---------------------------------------------------------
        val monitoringThread = Thread {
            Log.d("BenchmarkManager", "Monitoring Thread Started.")
            while (isBenchmarkRunning.get()) {
                val currentMs = System.currentTimeMillis() - startTime
                val currentTemp = getCurrentBatteryTemp()

                thermalHistory.add(ThermalPoint(currentMs, currentTemp))
                Log.v("BenchmarkManager", "Monitor: T=${currentMs}ms, Temp=${currentTemp}C")

                try {
                    Thread.sleep(5000)
                } catch (e: InterruptedException) {
                    break
                }
            }
            Log.d("BenchmarkManager", "Monitoring Thread Stopped.")
        }

        val benchmarkThread = Thread {
            Log.d("BenchmarkManager", "Benchmark Thread Started.")

            monitoringThread.start()

            val jsonResultFromCpp = runNativeBenchmark()

            isBenchmarkRunning.set(false)

            try {
                monitoringThread.join()
            } catch (e: InterruptedException) {
                e.printStackTrace()
            }

            Log.d("BenchmarkManager", "Native result: $jsonResultFromCpp")
            val result = try {
                Gson().fromJson(jsonResultFromCpp, BenchmarkResult::class.java)
            } catch (e: Exception) {
                BenchmarkResult(false, "JSON Parsing Error: ${e.message}")
            }

            Handler(Looper.getMainLooper()).post {
                onComplete(result, ArrayList(thermalHistory))
            }
        }

        benchmarkThread.start()
    }

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    fun prepareForBenchmark() {
        Log.d("BenchmarkManager", "Preparing for benchmark.")
        val activity = context as? Activity ?: return

        activity.window?.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val gameManager = context.getSystemService(android.app.GameManager::class.java)
            if (gameManager.gameMode != android.app.GameManager.GAME_MODE_PERFORMANCE) {
                Log.w("BenchmarkManager", "Warning: Device is not in GAME_MODE_PERFORMANCE.")
            }
            gameManager.setGameState(android.app.GameState(true, android.app.GameState.MODE_GAMEPLAY_UNINTERRUPTIBLE))
        }
    }

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    fun cleanupAfterBenchmark() {
        Log.d("BenchmarkManager", "Cleaning up after benchmark.")
        val activity = context as? Activity ?: return
        activity.window?.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val gameManager = context.getSystemService(android.app.GameManager::class.java)
            gameManager.setGameState(android.app.GameState(false, android.app.GameState.MODE_UNKNOWN))
        }
    }

    companion object {
        init {
            System.loadLibrary("performic")
        }
    }
}