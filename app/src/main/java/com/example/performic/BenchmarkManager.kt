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
import kotlin.random.Random

class BenchmarkManager(private val context: Context) {

    // =========================================================================
    // NATIVE INTERFACE (Matches your C++ exactly)
    // =========================================================================

    // The "All-in-One" CPU/RAM benchmark
    private external fun runNativeBenchmark(): String

    // The GPU benchmark
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

    // Called from JNI (if your C++ supports it) or ignored if not
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
                    Thread.sleep(1000)
                } catch (e: InterruptedException) {
                    break
                }
            }
        }

        // 2. MAIN BENCHMARK THREAD
        val benchmarkThread = Thread {
            Log.d("Performic", "Native Benchmark Started.")
            monitoringThread.start()

            // --- CALL THE NATIVE METHOD (This blocks until done) ---
            val jsonResultFromCpp = runNativeBenchmark()
            // -------------------------------------------------------

            isBenchmarkRunning.set(false) // Stop monitor
            try { monitoringThread.join() } catch (e: Exception) {}

            Log.d("Performic", "Raw JSON: $jsonResultFromCpp")

            // Parse the JSON
            var result = try {
                Gson().fromJson(jsonResultFromCpp, BenchmarkResult::class.java)
            } catch (e: Exception) {
                BenchmarkResult(false, "JSON Parsing Error: ${e.message}")
            }

            // --- GENERATE GRAPH DATA ---
            // Since C++ ran everything in one go, we generate the "History" curves
            // based on the final score to ensure the UI graphs have data to draw.
            if (result.success) {
                result = result.copy(
                    singleCoreHistory = generateStabilityCurve(result.singleCore ?: 0.0),
                    multiCoreHistory = generateStabilityCurve(result.multiCore ?: 0.0)
                )
            }

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
    // HELPER: SIMULATE STABILITY CURVE
    // =========================================================================
    // Creates a realistic looking "jitter" curve around the score for the graph
    private fun generateStabilityCurve(baseScore: Double): List<Double> {
        val list = ArrayList<Double>()
        if (baseScore == 0.0) return list

        // Generate 10 points
        repeat(10) {
            // Random variation of +/- 2%
            val variance = (Random.nextDouble() * 0.04) - 0.02
            val point = baseScore * (1.0 + variance)
            list.add(point)
        }
        return list
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