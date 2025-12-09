#include "MemoryBenchmark.h"
#include "utils.h" // For ClobberMemory
#include <vector>
#include <chrono>
#include <cstring> // For memcpy
#include <algorithm> // For std::max
#include <android/log.h>

#define LOG_TAG "PerformicMem"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// --- CONFIGURATION ---
constexpr int SIZE_L1  = 32 * 1024;        // 32 KB (Fits in L1)
constexpr int SIZE_L2  = 512 * 1024;       // 512 KB (Fits in L2/L3)
constexpr int SIZE_RAM = 64 * 1024 * 1024; // 64 MB (Forces RAM access)

constexpr int ITERATIONS_CACHE = 50000; // Run many times because cache is fast
constexpr int ITERATIONS_RAM   = 500;   // Run fewer times because RAM is slow

MemoryBenchmark::MemoryScores MemoryBenchmark::runMemorySuite() {
    LOGD("--- STARTING MEMORY BENCHMARK ---");

    // 1. Measure L1 Cache
    double l1GBs = measureBandwidth(SIZE_L1);
    LOGD("L1 Cache Speed: %.2f GB/s", l1GBs);

    // 2. Measure L2 Cache
    double l2GBs = measureBandwidth(SIZE_L2);
    LOGD("L2 Cache Speed: %.2f GB/s", l2GBs);

    // 3. Measure RAM (DRAM)
    double ramGBs = measureBandwidth(SIZE_RAM);
    LOGD("RAM Speed: %.2f GB/s", ramGBs);

    // --- SCORING ---
    // Baseline: P30 Lite (LPDDR4X)
    // Est. P30 RAM: ~7 GB/s. Est. L1: ~50 GB/s.
    // We heavily weight RAM speed because that affects User Experience the most.

    double refRam = 7.0; // P30 Lite Baseline GB/s
    double ramScore = (ramGBs / refRam) * 1000.0;

    // Add a small bonus for cache speed
    double cacheBonus = (l1GBs / 100.0) * 100.0;

    return { l1GBs, l2GBs, ramGBs, ramScore + cacheBonus };
}

double MemoryBenchmark::measureBandwidth(int bufferSize) {
    // 1. Allocate Source and Dest buffers
    std::vector<uint8_t> src(bufferSize, 1);
    std::vector<uint8_t> dest(bufferSize, 0);

    // Determine iterations based on size
    // (Run more iterations for small buffers to get accurate time)
    int iterations = (bufferSize < 1024*1024) ? ITERATIONS_CACHE : ITERATIONS_RAM;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        // The Core Operation: Memory Copy
        std::memcpy(dest.data(), src.data(), bufferSize);

        // Prevent compiler optimization
        ClobberMemory();
    }

    auto end = std::chrono::high_resolution_clock::now();
    double durationSec = std::chrono::duration<double>(end - start).count();

    // Calculate Data Transferred
    // Total Bytes = Size * Iterations
    long long totalBytes = (long long)bufferSize * iterations;

    // GB/s = (Bytes / 10^9) / Seconds
    double throughputGBs = (double)totalBytes / 1e9 / durationSec;

    return throughputGBs;
}