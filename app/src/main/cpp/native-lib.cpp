#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include "BenchmarkCore.h"
#include "gpu_benchmark/GpuBenchmark.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_performic_BenchmarkManager_runNativeBenchmark(
        JNIEnv* env,
        jobject /* this */) {

    BenchmarkCore core;

    std::string json_result = core.runFullBenchmark();

    return env->NewStringUTF(json_result.c_str());
}

extern "C" JNIEXPORT jdouble JNICALL
Java_com_example_performic_BenchmarkManager_runGpuBenchmark(
        JNIEnv* env,
        jobject thiz /* this */,
        jobject surface) { // Receives SurfaceHolder.surface

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);

    GpuBenchmark gpu;
    double score = gpu.run(window, env, thiz);

    ANativeWindow_release(window);
    return score;
}