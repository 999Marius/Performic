package com.example.performic

import android.app.Activity
import android.content.Context
import android.os.Build
import android.util.Log
import android.view.WindowManager
import androidx.annotation.RequiresApi
import com.google.gson.Gson

class BenchmarkManager(private val context: Context) {

    private external fun runNativeBenchmark(): String


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
            Log.d("BenchmarkManager", "Set GameState to UNINTERRUPTIBLE.")
        }
    }

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    fun cleanupAfterBenchmark() {
        Log.d("BenchmarkManager", "Cleaning up after benchmark.")
        val activity = context as? Activity ?: return

        activity.window?.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val gameManager = context.getSystemService(android.app.GameManager::class.java)
            gameManager.setGameState(android.app.GameState(false, android.app.GameState.MODE_GAMEPLAY_UNINTERRUPTIBLE))
            Log.d("BenchmarkManager", "Set GameState back to normal.")
        }
    }


    fun runCoreBenchmark(): BenchmarkResult {
        val jsonResultFromCpp = runNativeBenchmark()
        Log.d("BenchmarkManager", "Native result: $jsonResultFromCpp")
        return Gson().fromJson(jsonResultFromCpp, BenchmarkResult::class.java)
    }

    companion object {
        init {
            System.loadLibrary("performic")
        }
    }
}