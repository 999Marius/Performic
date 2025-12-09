#ifndef PERFORMIC_GPUBENCHMARK_H
#define PERFORMIC_GPUBENCHMARK_H

#include <android/native_window.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <jni.h>
class GpuBenchmark {
public:
    // Runs the 3D scene for 5 seconds and returns a Score (based on Frames Rendered)
    double run(ANativeWindow* window, JNIEnv* env, jobject callbackObj);
private:
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    int width;
    int height;

    bool initEGL(ANativeWindow* window);
    void cleanupEGL();
    GLuint loadShader(GLenum type, const char* shaderSrc);
};

#endif //PERFORMIC_GPUBENCHMARK_H