package com.example.performic

data class BenchmarkResult(
    val success: Boolean,
    val message: String,
    val singleCore: Double? = null,
    val multiCore: Double? = null,

    val ramScore: Double? = null,
    val ramGBs: Double? = null,
    val l1GBs: Double? = null, // NEW
    val l2GBs: Double? = null,

    val singleCoreHistory: List<Double> = emptyList(),
    val multiCoreHistory: List<Double> = emptyList()
)

