package com.example.performic

import android.annotation.SuppressLint
import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import androidx.annotation.RequiresApi
import com.example.performic.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var benchmarkManager: BenchmarkManager

    @SuppressLint("DefaultLocale")
    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        benchmarkManager = BenchmarkManager(this)

        binding.buttonStart.setOnClickListener {
            binding.textViewResult.text = "Running..."
            binding.buttonStart.isEnabled = false

            benchmarkManager.prepareForBenchmark()

            Thread {
                val result = benchmarkManager.runCoreBenchmark()

                runOnUiThread {
                    val displayText = if(result.success && result.cpuScore != null){
                        val formattedScore = String.format("%.2f", result.cpuScore)
                        "${result.message}\nCPU Score: ${formattedScore}ms"
                    }else{
                        result.message
                    }
                    binding.textViewResult.text = result.message
                    benchmarkManager.cleanupAfterBenchmark()
                    binding.buttonStart.isEnabled = true
                }
            }.start()
        }
    }
}