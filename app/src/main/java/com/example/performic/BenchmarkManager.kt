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

    // =========================================================================
    // NATIVE INTERFACE
    // =========================================================================

    // C++ returns a JSON string containing the scores AND the real history arrays
    private external fun runNativeBenchmark(): String
    private external fun runGpuBenchmark(surface: android.view.Surface): Double

    companion object {
        init {
            System.loadLibrary("performic")
        }
    }

    // =========================================================================
    // LISTENERS
    // =========================================================================

    interface FpsCallback {
        fun onFpsUpdate(fps: Int)
    }
    var fpsListener: FpsCallback? = null

    fun onFpsUpdate(fps: Int) {
        fpsListener?.onFpsUpdate(fps)
    }

    private fun getCurrentBatteryTemp(): Float {
        val intent = context.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
        val tempInt = intent?.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, 0) ?: 0
        return tempInt / 10.0f
    }

    // =========================================================================
    // BENCHMARK LOGIC
    // =========================================================================

    fun runCoreBenchmarkWithMonitoring(
        onComplete: (BenchmarkResult, List<ThermalPoint>) -> Unit
    ) {
        val thermalHistory = Collections.synchronizedList(ArrayList<ThermalPoint>())
        val isBenchmarkRunning = AtomicBoolean(true)
        val startTime = System.currentTimeMillis()

        // 1. THERMAL MONITOR THREAD
        val monitoringThread = Thread {
            Log.d("Performic", "Monitor Started.")
            while (isBenchmarkRunning.get()) {
                val currentMs = System.currentTimeMillis() - startTime
                val currentTemp = getCurrentBatteryTemp()
                thermalHistory.add(ThermalPoint(currentMs, currentTemp))
                try {
                    Thread.sleep(500) // 0.5s intervals for better resolution
                } catch (e: InterruptedException) {
                    break
                }
            }
        }

        // 2. MAIN BENCHMARK THREAD
        val benchmarkThread = Thread {
            Log.d("Performic", "Native Benchmark Started.")
            monitoringThread.start()

            // --- CALL C++ (BLOCKING) ---
            // C++ runs the loop (20x), collects REAL data, and returns JSON
            val jsonResultFromCpp = runNativeBenchmark()
            // ---------------------------

            isBenchmarkRunning.set(false) // Stop monitor
            try { monitoringThread.join() } catch (e: Exception) {}

            Log.d("Performic", "Raw JSON: $jsonResultFromCpp")

            // Parse the JSON directly into the Result object
            // The JSON already contains "singleCoreHistory": [100, 102, 98...]
            val result = try {
                Gson().fromJson(jsonResultFromCpp, BenchmarkResult::class.java)
            } catch (e: Exception) {
                BenchmarkResult(false, "JSON Parsing Error: ${e.message}")
            }

            // --- REMOVED THE FAKE GENERATOR HERE ---
            // We now use the 'result' exactly as it came from C++

            // Return to UI
            Handler(Looper.getMainLooper()).post {
                onComplete(result, ArrayList(thermalHistory))
            }
        }

        benchmarkThread.start()
    }

    fun runGpuTest(surface: android.view.Surface): Double {
        return runGpuBenchmark(surface)
    }

    // =========================================================================
    // SYSTEM PREP
    // =========================================================================
    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    fun prepareForBenchmark() {
        val activity = context as? Activity ?: return
        activity.runOnUiThread {
            activity.window?.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val gameManager = context.getSystemService(android.app.GameManager::class.java)
            try {
                gameManager.setGameState(android.app.GameState(true, android.app.GameState.MODE_GAMEPLAY_UNINTERRUPTIBLE))
            } catch (e: Exception) {}
        }
    }

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    fun cleanupAfterBenchmark() {
        val activity = context as? Activity ?: return
        activity.runOnUiThread {
            activity.window?.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val gameManager = context.getSystemService(android.app.GameManager::class.java)
            try {
                gameManager.setGameState(android.app.GameState(false, android.app.GameState.MODE_UNKNOWN))
            } catch (e: Exception) {}
        }
    }
}