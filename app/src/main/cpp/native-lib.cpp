#include <jni.h>
#include <string>
#include "BenchmarkCore.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_performic_BenchmarkManager_runNativeBenchmark(
        JNIEnv* env,
        jobject /* this */) {

    BenchmarkCore core;

    std::string json_result = core.runFullBenchmark();

    return env->NewStringUTF(json_result.c_str());
}