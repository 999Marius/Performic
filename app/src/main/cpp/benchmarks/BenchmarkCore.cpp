#include "../BenchmarkCore.h"
#include <android/log.h>
#include <unistd.h>
#include <android/api-level.h>
#include <dlfcn.h>
#include <string>
#include <vector>     // <--- Added for std::vector
#include <sstream>    // <--- REQUIRED for stringstream
#include "cpu_benchmark/CpuBenchmark.h"
#include "memoty_benchmark/MemoryBenchmark.h"

#define LOG_TAG "PerformicCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Function pointer types for Thermal API
typedef void* (*AThermal_acquireManager_t)();
typedef int (*AThermal_getCurrentThermalStatus_t)(void*);
typedef void (*AThermal_releaseManager_t)(void*);

enum {
    ATHERMAL_STATUS_NONE = 0,
    ATHERMAL_STATUS_LIGHT = 1,
};

// --- HELPER FUNCTION ---
// Converts a C++ vector<double> into a JSON string "[1.0, 2.0, 3.0]"
// This is now strictly internal to this file.
static std::string vectorToJsonArray(const std::vector<double>& vec) {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        ss << vec[i];
        if (i < vec.size() - 1) {
            ss << ",";
        }
    }
    ss << "]";
    return ss.str();
}

std::string BenchmarkCore::runFullBenchmark() {
    LOGI("BenchmarkCore: Starting full benchmark.");

    if (!isDeviceCoolEnough()) {
        LOGI("WARNING: Device is hot. Performance may be throttled.");
    }

    // 1. Run CPU Suite (Returns Scores + History Vectors)
    CpuBenchmark cpu_test;
    CpuBenchmark::Scores results = cpu_test.runFullSuite();

    // 2. Run Memory Suite
    MemoryBenchmark mem_test;
    MemoryBenchmark::MemoryScores memResults = mem_test.runMemorySuite();

    // 3. Build JSON
    std::stringstream ss;
    ss << "{";
    ss << "\"success\":true, ";
    ss << "\"message\":\"Benchmark complete!\", ";

    // Scores
    ss << "\"singleCore\":" << results.singleCoreScore << ", ";
    ss << "\"multiCore\":" << results.multiCoreScore << ", ";

    // RAM
    ss << "\"ramScore\":" << memResults.memoryScore << ", ";
    ss << "\"ramGBs\":" << memResults.ramThroughput << ", ";
    ss << "\"l1GBs\":"    << memResults.l1Throughput << ", ";
    ss << "\"l2GBs\":"    << memResults.l2Throughput << ", ";

    // --- NEW: Inject the History Arrays ---
    ss << "\"singleCoreHistory\":" << vectorToJsonArray(results.singleCoreHistory) << ", ";
    ss << "\"multiCoreHistory\":" << vectorToJsonArray(results.multiCoreHistory);

    ss << "}";

    std::string json_result = ss.str();
    // LOGI("BenchmarkCore: Generated JSON: %s", json_result.c_str()); // Uncomment to debug JSON

    return json_result;
}

bool BenchmarkCore::isDeviceCoolEnough() {
    if (android_get_device_api_level() < 30) return true;

    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (!libandroid) return true;

    auto acquireManager = (AThermal_acquireManager_t)dlsym(libandroid, "AThermal_acquireManager");
    auto getCurrentStatus = (AThermal_getCurrentThermalStatus_t)dlsym(libandroid, "AThermal_getCurrentThermalStatus");
    auto releaseManager = (AThermal_releaseManager_t)dlsym(libandroid, "AThermal_releaseManager");

    if (!acquireManager || !getCurrentStatus || !releaseManager) {
        dlclose(libandroid);
        return true;
    }

    void* thermalManager = acquireManager();
    if (!thermalManager) {
        dlclose(libandroid);
        return true;
    }

    int status = getCurrentStatus(thermalManager);
    releaseManager(thermalManager);
    dlclose(libandroid);

    LOGI("Current thermal status: %d", status);
    return (status < ATHERMAL_STATUS_LIGHT);
}