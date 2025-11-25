package com.example.performic

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.example.performic.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var benchmarkManager: BenchmarkManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        benchmarkManager = BenchmarkManager(this)

        binding.buttonStart.setOnClickListener {
            binding.buttonStart.isEnabled = false
            binding.textViewResult.text = "Running Scientific Benchmark...\n"

            benchmarkManager.prepareForBenchmark()

            benchmarkManager.runCoreBenchmarkWithMonitoring { result, thermalHistory ->

                if (result.success) {
                    val startTemp = thermalHistory.firstOrNull()?.temperature ?: 0f
                    val endTemp = thermalHistory.lastOrNull()?.temperature ?: 0f
                    val heatRise = endTemp - startTemp
                    val riseSign = if (heatRise >= 0) "+" else ""


                    val scScore = result.singleCore ?: 0.0
                    val mcScore = result.multiCore ?: 0.0

                    val text = """
                        === BENCHMARK RESULTS ===
                        (Higher Score is Better)
                        (Baseline P30 Lite = 1000)
                        
                        Single-Core: ${String.format("%.0f", scScore)} Points
                        Multi-Core:  ${String.format("%.0f", mcScore)} Points
                        
                        ----------------------
                        Thermal Analysis:
                        Heat Rise: $riseSign${String.format("%.1f", heatRise)}°C
                        (Start: $startTemp°C -> End: $endTemp°C)
                    """.trimIndent()

                    binding.textViewResult.text = text
                } else {
                    binding.textViewResult.text = "Error: ${result.message}"
                }

                benchmarkManager.cleanupAfterBenchmark()
                binding.buttonStart.isEnabled = true
            }
        }
    }
}