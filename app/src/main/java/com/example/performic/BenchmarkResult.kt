package com.example.performic

data class BenchmarkResult(
    val success: Boolean,
    val message: String,
    val singleCore: Double? = null,
    val multiCore: Double? = null
)

