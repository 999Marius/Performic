package com.example.performic

import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.view.SurfaceHolder
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import com.example.performic.databinding.ActivityMainBinding
import com.example.performic.record.ThermalPoint
import com.github.mikephil.charting.charts.LineChart
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var benchmarkManager: BenchmarkManager
    private var gpuCallback: SurfaceHolder.Callback? = null

    // Holds the LIVE FPS data from the GPU test
    private val gpuFpsHistory = ArrayList<Double>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        supportActionBar?.hide()

        binding.surfaceViewGpu.setZOrderOnTop(true)
        binding.surfaceViewGpu.holder.setFormat(android.graphics.PixelFormat.OPAQUE)

        benchmarkManager = BenchmarkManager(this)

        binding.textDeviceName.text = "${Build.MANUFACTURER} ${Build.MODEL}".uppercase()

        binding.buttonStart.setOnClickListener { startBenchmark() }
        binding.buttonClose.setOnClickListener { resetUi() }

        resetUi()
    }

    private fun startBenchmark() {
        binding.layoutStart.visibility = View.GONE
        binding.layoutProgress.visibility = View.VISIBLE
        binding.textProgressStatus.text = "Benchmarking CPU & Memory..."

        benchmarkManager.prepareForBenchmark()

        // RUN CPU TEST
        benchmarkManager.runCoreBenchmarkWithMonitoring { cpuResult, thermalHistory ->
            runOnUiThread {
                if (cpuResult.success) {
                    // CPU Done -> Start GPU Test
                    runGpuTest(cpuResult, thermalHistory)
                } else {
                    binding.textProgressStatus.text = "Error: ${cpuResult.message}"
                }
            }
        }
    }

    private fun runGpuTest(cpuResult: BenchmarkResult, thermalHistory: List<ThermalPoint>) {
        binding.layoutStart.visibility = View.GONE
        binding.layoutProgress.visibility = View.GONE
        binding.layoutResults.visibility = View.GONE
        binding.layoutGpuRender.visibility = View.VISIBLE
        binding.surfaceViewGpu.visibility = View.VISIBLE

        binding.textLiveFps.text = "FPS: --"
        gpuFpsHistory.clear() // Clear old data

        benchmarkManager.fpsListener = object : BenchmarkManager.FpsCallback {
            override fun onFpsUpdate(fps: Int) {
                runOnUiThread {
                    binding.textLiveFps.text = "$fps FPS"
                    // Capture REAL GPU data for the graph
                    if (fps > 0) gpuFpsHistory.add(fps.toDouble())
                }
            }
        }

        if (gpuCallback != null) binding.surfaceViewGpu.holder.removeCallback(gpuCallback)

        gpuCallback = object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                Thread {
                    try {
                        val gpuScore = benchmarkManager.runGpuTest(holder.surface)
                        runOnUiThread {
                            benchmarkManager.cleanupAfterBenchmark()
                            // TEST COMPLETE: Show all graphs
                            showResults(cpuResult, gpuScore, thermalHistory)
                        }
                    } catch (e: Exception) {
                        e.printStackTrace()
                        runOnUiThread { resetUi() }
                    }
                }.start()
            }
            override fun surfaceChanged(h: SurfaceHolder, format: Int, w: Int, ht: Int) {}
            override fun surfaceDestroyed(h: SurfaceHolder) {}
        }
        binding.surfaceViewGpu.holder.addCallback(gpuCallback)
        if (binding.surfaceViewGpu.holder.surface.isValid) gpuCallback?.surfaceCreated(binding.surfaceViewGpu.holder)
    }

    private fun showResults(cpu: BenchmarkResult, gpu: Double, thermal: List<ThermalPoint>) {
        binding.rootContainer.setBackgroundColor(Color.parseColor("#121212"))

        if (gpuCallback != null) {
            binding.surfaceViewGpu.holder.removeCallback(gpuCallback)
            gpuCallback = null
        }
        binding.layoutGpuRender.visibility = View.GONE
        binding.surfaceViewGpu.visibility = View.GONE
        binding.layoutResults.visibility = View.VISIBLE

        fun d(v: Double?) = v ?: 0.0

        // CALCULATE OVERALL SCORE (Weighted Average)
        // Weights: Single-Core 20%, Multi-Core 35%, RAM 20%, GPU 25%
        val overallScore = (d(cpu.singleCore) * 0.20) +
                (d(cpu.multiCore) * 0.35) +
                (d(cpu.ramScore) * 0.20) +
                (gpu * 0.25)

        // 1. SET OVERALL SCORE (TOP OF PAGE)
        binding.resOverallScore.text = "%.0f".format(overallScore)

        // 2. SET INDIVIDUAL TEXT SCORES
        binding.resCpuSingle.text = "%.0f".format(d(cpu.singleCore))
        binding.resCpuMulti.text = "%.0f".format(d(cpu.multiCore))
        binding.resMemScore.text = "%.0f".format(d(cpu.ramScore))
        binding.resMemBandwidth.text = "%.2f GB/s".format(d(cpu.ramGBs))
        binding.resL1.text = "%.2f GB/s".format(d(cpu.l1GBs))
        binding.resL2.text = "%.2f GB/s".format(d(cpu.l2GBs))
        binding.resGpuScore.text = "%.0f".format(gpu)

        // 3. SET PROGRESS BARS
        binding.progCpuSingle.progress = ((d(cpu.singleCore) / 2000.0) * 100).toInt().coerceIn(0, 100)
        binding.progCpuMulti.progress = ((d(cpu.multiCore) / 10000.0) * 100).toInt().coerceIn(0, 100)
        binding.progGpu.progress = ((gpu / 5000.0) * 100).toInt().coerceIn(0, 100)

        // 4. SET THERMAL TEXT
        val startTemp = thermal.firstOrNull()?.temperature ?: 0f
        val endTemp = thermal.lastOrNull()?.temperature ?: 0f
        binding.resTempStart.text = "%.1f°".format(startTemp)
        binding.resTempPeak.text = "%.1f°".format(endTemp)
        binding.resTempRise.text = "+%.1f°".format(endTemp - startTemp)

        // 5. SETUP GRAPHS
        try {
            // A. Single Core Graph
            val singleData = if (cpu.singleCoreHistory.isNotEmpty()) cpu.singleCoreHistory else listOf(d(cpu.singleCore))
            setupChart(binding.chartCpu, singleData, "Single-Core Stability", "#00E5FF")

            // B. Multi Core Graph
            val multiData = if (cpu.multiCoreHistory.isNotEmpty()) cpu.multiCoreHistory else listOf(d(cpu.multiCore))
            setupChart(binding.chartCpuMulti, multiData, "Multi-Core Stability", "#AA00FF")

            // C. GPU FPS Graph
            val fpsData = if (gpuFpsHistory.isNotEmpty()) gpuFpsHistory else listOf(gpu / 100.0)
            setupChart(binding.chartGpuFps, fpsData, "GPU FPS", "#00E676")

            // D. Thermal Graph
            val tempData = thermal.map { it.temperature.toDouble() }
            setupChart(binding.chartThermal, tempData, "Temperature (°C)", "#FF5252")

        } catch (e: Exception) { e.printStackTrace() }
    }

    private fun resetUi() {
        if (gpuCallback != null) {
            binding.surfaceViewGpu.holder.removeCallback(gpuCallback)
            gpuCallback = null
        }
        binding.layoutResults.visibility = View.GONE
        binding.layoutProgress.visibility = View.GONE
        binding.layoutGpuRender.visibility = View.GONE
        binding.layoutStart.visibility = View.VISIBLE
    }

    private fun setupChart(chart: LineChart, dataValues: List<Double>, label: String, colorHex: String) {
        if (dataValues.isEmpty()) return
        val color = Color.parseColor(colorHex)
        val entries = dataValues.mapIndexed { index, value -> Entry(index.toFloat(), value.toFloat()) }

        val dataSet = LineDataSet(entries, label).apply {
            this.color = color
            setDrawCircles(true)      // Show points
            setDrawCircleHole(false)
            setCircleColor(color)
            circleRadius = 3f         // Small dots

            setDrawValues(false)      // Hide text numbers on graph (too messy with many points)

            lineWidth = 2f
            mode = LineDataSet.Mode.CUBIC_BEZIER // Smooth curves
            setDrawFilled(true)
            fillColor = color
            fillAlpha = 50
        }

        chart.apply {
            data = LineData(dataSet)
            description.isEnabled = false
            legend.textColor = Color.WHITE

            xAxis.textColor = Color.GRAY
            xAxis.setDrawGridLines(false)

            axisLeft.textColor = Color.GRAY
            axisLeft.setDrawGridLines(true)
            axisLeft.gridColor = Color.parseColor("#333333")

            axisRight.isEnabled = false // Hide right axis

            // Auto-scale axis to fit the "jitter"
            isScaleYEnabled = false
            invalidate()
        }
    }
}