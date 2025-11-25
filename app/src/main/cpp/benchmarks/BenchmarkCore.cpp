#include "../BenchmarkCore.h"
#include <android/log.h>
#include <unistd.h>
#include <android/api-level.h>
#include <dlfcn.h>
#include <string>
#include <sstream> // <--- REQUIRED for the fix
#include "cpu_benchmark/CpuBenchmark.h"

#define LOG_TAG "PerformicCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Function pointer types for Thermal API
typedef void* (*AThermal_acquireManager_t)();
typedef int (*AThermal_getCurrentThermalStatus_t)(void*);
typedef void (*AThermal_releaseManager_t)(void*);

enum {
    ATHERMAL_STATUS_NONE = 0,
    ATHERMAL_STATUS_LIGHT = 1,
    ATHERMAL_STATUS_MODERATE = 2,
    ATHERMAL_STATUS_SEVERE = 3,
    ATHERMAL_STATUS_CRITICAL = 4,
    ATHERMAL_STATUS_EMERGENCY = 5,
    ATHERMAL_STATUS_SHUTDOWN = 6,
};

std::string BenchmarkCore::runFullBenchmark() {
    LOGI("BenchmarkCore: Starting full benchmark.");

    if (!isDeviceCoolEnough()) {
        LOGI("WARNING: Device is hot. Performance may be throttled.");
    }

    CpuBenchmark cpu_test;

    CpuBenchmark::Scores results = cpu_test.runFullSuite();

    std::stringstream ss;
    ss << "{";
    ss << "\"success\":true, ";
    ss << "\"message\":\"Benchmark complete!\", ";

    ss << "\"singleCore\":" << results.singleCoreScore << ", ";
    ss << "\"multiCore\":" << results.multiCoreScore;
    ss << "}";

    std::string json_result = ss.str();
    LOGI("BenchmarkCore: Generated JSON: %s", json_result.c_str());

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

