package com.example.performic

data class BenchmarkResult (
    val success: Boolean,
    val message: String,
    val cpuScore: Double? = null
)

