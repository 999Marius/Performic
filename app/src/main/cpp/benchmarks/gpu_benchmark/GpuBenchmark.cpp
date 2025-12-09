#include "GpuBenchmark.h"
#include <android/log.h>
#include <chrono>
#include <cmath>
#include <string>
#include <sstream>

#define LOG_TAG "PerformicGPU"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

const double BENCHMARK_DURATION_MS = 20000.0;

const char* vShaderSrc = R"(
    attribute vec4 vPosition;
    void main() {
        gl_Position = vPosition;
    }
)";

const char* fShaderSrc = R"(
    precision highp float;
    uniform float uTime;
    uniform vec2 uResolution;

    float gyroid(vec3 p) {
        return dot(sin(p), cos(p.yzx));
    }

    float map(vec3 p) {
        float d = gyroid(p * 5.0 + uTime * 0.5) * 0.1;
        d += gyroid(p * 2.0) * 0.3;
        return d;
    }

    void main() {
        vec2 uv = (gl_FragCoord.xy * 2.0 - uResolution) / uResolution.y;

        vec3 ro = vec3(0.0, 0.0, uTime);
        vec3 rd = normalize(vec3(uv, 1.0));

        float t = 0.0;
        vec3 col = vec3(0.0);
        float glow = 0.0;

        for(int i = 0; i < 80; i++) {
            vec3 p = ro + rd * t;
            float d = map(p);
            float local_glow = 1.0 / (1.0 + abs(d) * 20.0);
            glow += local_glow;
            t += max(d * 0.5, 0.02);
            if(t > 10.0) break;
        }

        col = vec3(glow * 0.02);
        col += vec3(0.8, 0.4, 0.1) * (glow * 0.01);
        col += vec3(0.5, 0.1, 0.1) * (t * 0.1);

        gl_FragColor = vec4(col, 1.0);
    }
)";

double GpuBenchmark::run(ANativeWindow* window, JNIEnv* env, jobject callbackObj) {
    if (!initEGL(window)) {
        LOGE("Failed to init EGL");
        return 0.0;
    }

    jclass cls = env->GetObjectClass(callbackObj);
    jmethodID methodId = env->GetMethodID(cls, "onFpsUpdate", "(I)V");

    GLuint vShader = loadShader(GL_VERTEX_SHADER, vShaderSrc);
    GLuint fShader = loadShader(GL_FRAGMENT_SHADER, fShaderSrc);

    if (vShader == 0 || fShader == 0) {
        LOGE("Shader compilation failed");
        return 0.0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLchar log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        LOGE("Program link failed: %s", log);
        return 0.0;
    }

    glUseProgram(program);
    LOGD("Shaders compiled and linked successfully");

    GLfloat vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
    GLint posLoc = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, vertices);

    GLint timeLoc = glGetUniformLocation(program, "uTime");
    GLint resLoc = glGetUniformLocation(program, "uResolution");

    LOGD("Uniform locations - uTime: %d, uResolution: %d", timeLoc, resLoc);

    int frameCount = 0;
    int fpsFrameCount = 0;
    auto start = std::chrono::high_resolution_clock::now();
    auto lastFpsTime = start;
    double elapsedMs = 0;

    while (elapsedMs < BENCHMARK_DURATION_MS) {
        auto now = std::chrono::high_resolution_clock::now();
        elapsedMs = std::chrono::duration<double, std::milli>(now - start).count();
        double loopTime = std::chrono::duration<double, std::milli>(now - lastFpsTime).count();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform1f(timeLoc, (float)elapsedMs / 1000.0f);
        glUniform2f(resLoc, (float)width, (float)height);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        eglSwapBuffers(display, surface);

        frameCount++;
        fpsFrameCount++;

        if (loopTime >= 1000.0) {
            int currentFps = (int)(fpsFrameCount / (loopTime / 1000.0));
            env->CallVoidMethod(callbackObj, methodId, currentFps);
            //LOGD("FPS: %d", currentFps);
            fpsFrameCount = 0;
            lastFpsTime = now;
        }
    }

    cleanupEGL();

    double avgFps = (double)frameCount / (elapsedMs / 1000.0);
    LOGD("Benchmark complete - Avg FPS: %.2f, Score: %.2f", avgFps, avgFps * 100.0);
    return avgFps * 100.0;
}

bool GpuBenchmark::initEGL(ANativeWindow* window) {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, 0, 0);
    const EGLint attribs[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_NONE };
    EGLConfig config; EGLint numConfigs;
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    surface = eglCreateWindowSurface(display, config, window, 0);
    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) return false;

    eglSwapInterval(display, 0);

    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);
    glViewport(0, 0, width, height);

    LOGD("EGL initialized - Resolution: %dx%d", width, height);
    return true;
}

void GpuBenchmark::cleanupEGL() {
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
}

GLuint GpuBenchmark::loadShader(GLenum type, const char* shaderSrc) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSrc, 0);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLchar log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        LOGE("Shader compilation failed: %s", log);
        return 0;
    }

    return shader;
}