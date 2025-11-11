#include "../BenchmarkCore.h"
#include <android/log.h>
#include <unistd.h>
#include <android/api-level.h>
#include <dlfcn.h>
#include "cpu_benchmark/CpuBenchmark.h"
#define LOG_TAG "PerformicCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Function pointer types
typedef void* (*AThermal_acquireManager_t)();
typedef int (*AThermal_getCurrentThermalStatus_t)(void*);
typedef void (*AThermal_releaseManager_t)(void*);

// Thermal status enum values
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
        LOGI("Device is too hot. Aborting benchmark.");
        return "{\"success\":false, \"message\":\"Device is too hot. Please let it cool down and try again.\"}";
    }

    LOGI("Device is cool. Running placeholder workload...");
    CpuBenchmark cpu_test;
    double cpu_score_ms = cpu_test.run();
    LOGI("BenchmarkCore: CPU test completed. Average time: %f ms", cpu_score_ms);


    LOGI("BenchmarkCore: Completed.");
    return "{\"success\":true, \"message\":\"Benchmark complete! Thermal state: OK\"}";
}

bool BenchmarkCore::isDeviceCoolEnough() {
    // Runtime check for API level 30
    if (android_get_device_api_level() < 30) {
        LOGI("Device API level is %d, which is below 30. Skipping thermal check.",
             android_get_device_api_level());
        return true;
    }

    // Dynamically load the thermal functions from libandroid.so
    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (!libandroid) {
        LOGI("Failed to open libandroid.so: %s", dlerror());
        return true;
    }

    auto acquireManager = (AThermal_acquireManager_t)dlsym(libandroid, "AThermal_acquireManager");
    auto getCurrentStatus = (AThermal_getCurrentThermalStatus_t)dlsym(libandroid, "AThermal_getCurrentThermalStatus");
    auto releaseManager = (AThermal_releaseManager_t)dlsym(libandroid, "AThermal_releaseManager");

    if (!acquireManager || !getCurrentStatus || !releaseManager) {
        LOGI("Failed to get thermal API function pointers");
        dlclose(libandroid);
        return true;
    }

    void* thermalManager = acquireManager();
    if (!thermalManager) {
        LOGI("Failed to acquire thermal manager");
        dlclose(libandroid);
        return true;
    }

    int status = getCurrentStatus(thermalManager);
    releaseManager(thermalManager);
    dlclose(libandroid);

    LOGI("Current thermal status: %d", status);

    if (status >= ATHERMAL_STATUS_LIGHT) {
        LOGI("Device thermal status is LIGHT or worse. Too hot to benchmark.");
        return false;
    }

    LOGI("Device thermal status is NONE. Safe to start.");
    return true;
}