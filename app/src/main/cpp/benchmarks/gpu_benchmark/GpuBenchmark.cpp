#include "GpuBenchmark.h" // Include the header file for this class.
#include <android/log.h> // Include the Android logging library to print messages to logcat.
#include <chrono> // Include the library for time-related functions, used for measuring performance.
#include <cmath> // Include the math library for functions like sin, cos, abs.
#include <string> // Include the string library for using strings.
#include <sstream> // Include the string stream library, useful for building strings.

#define LOG_TAG "PerformicGPU" // Define a tag for our log messages to easily filter them in logcat.
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__) // Define a macro for debug logs.
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__) // Define a macro for error logs.

const double BENCHMARK_DURATION_MS = 20000.0; // Define how long the benchmark should run, in milliseconds (20 seconds).

// This is the Vertex Shader code, written in GLSL (OpenGL Shading Language).
const char* vShaderSrc = R"(
    attribute vec4 vPosition; // Input: the position of a vertex (a corner of our shape).
    void main() { // The main function that runs for each vertex.
        gl_Position = vPosition; // Output: Set the final position of the vertex on the screen.
    }
)";

// This is the Fragment Shader code, which calculates the color of each pixel.
const char* fShaderSrc = R"(
    precision highp float; // Set the default precision for floating-point numbers to high.
    uniform float uTime; // Input from C++: the current elapsed time for animation.
    uniform vec2 uResolution; // Input from C++: the screen resolution (width, height).

    // A function to create a "gyroid" pattern, a complex 3D shape.
    float gyroid(vec3 p) {
        return dot(sin(p), cos(p.yzx)); // It's a mathematical formula using sine, cosine, and dot product.
    }

    // A function that defines the 3D scene by combining gyroid patterns.
    float map(vec3 p) {
        float d = gyroid(p * 5.0 + uTime * 0.5) * 0.1; // First layer of detail, animated with time.
        d += gyroid(p * 2.0) * 0.3; // Second layer of detail.
        return d; // Returns the "distance" from the surface, used for raymarching.
    }

    // The main function that runs for every single pixel on the screen.
    void main() {
        // Convert pixel coordinates (gl_FragCoord) to a normalized -1 to 1 range.
        vec2 uv = (gl_FragCoord.xy * 2.0 - uResolution) / uResolution.y;

        // Define the camera position (ro = ray origin) and direction (rd = ray direction).
        vec3 ro = vec3(0.0, 0.0, uTime); // The camera moves forward over time.
        vec3 rd = normalize(vec3(uv, 1.0)); // The ray shoots from the camera through the pixel.

        float t = 0.0; // The distance traveled along the ray.
        vec3 col = vec3(0.0); // The final color of the pixel, starts as black.
        float glow = 0.0; // A variable to accumulate a glow effect.

        // This loop "marches" a ray through the 3D scene to find what it hits. This is called raymarching.
        for(int i = 0; i < 80; i++) {
            vec3 p = ro + rd * t; // Calculate the current point along the ray.
            float d = map(p); // Get the distance to the nearest surface from that point.
            float local_glow = 1.0 / (1.0 + abs(d) * 20.0); // Calculate glow based on proximity to a surface.
            glow += local_glow; // Add to the total glow.
            t += max(d * 0.5, 0.02); // Move the ray forward. Step size is based on distance `d`.
            if(t > 10.0) break; // If we've traveled too far without hitting anything, stop.
        }

        // Calculate the final color based on the accumulated glow and distance traveled.
        col = vec3(glow * 0.02); // Base color from the glow.
        col += vec3(0.8, 0.4, 0.1) * (glow * 0.01); // Add some orange/yellow color.
        col += vec3(0.5, 0.1, 0.1) * (t * 0.1); // Add some dark red based on distance.

        // Set the final color of the pixel. The '1.0' is for alpha (fully opaque).
        gl_FragColor = vec4(col, 1.0);
    }
)";

// The main function that runs the GPU benchmark.
double GpuBenchmark::run(ANativeWindow* window, JNIEnv* env, jobject callbackObj) {
    // Initialize EGL (the interface between OpenGL and the Android window system).
    if (!initEGL(window)) {
        LOGE("Failed to init EGL"); // Log an error if initialization fails.
        return 0.0; // Return a score of 0.0 on failure.
    }

    // Get the Java/Kotlin class and method to call for updating the FPS on the UI.
    jclass cls = env->GetObjectClass(callbackObj); // Get the class of the callback object.
    jmethodID methodId = env->GetMethodID(cls, "onFpsUpdate", "(I)V"); // Find the 'onFpsUpdate' method that takes an integer.

    // Load and compile the vertex and fragment shaders from the source code strings.
    GLuint vShader = loadShader(GL_VERTEX_SHADER, vShaderSrc); // Compile the vertex shader.
    GLuint fShader = loadShader(GL_FRAGMENT_SHADER, fShaderSrc); // Compile the fragment shader.

    // Check if the shaders compiled successfully.
    if (vShader == 0 || fShader == 0) {
        LOGE("Shader compilation failed"); // Log an error if compilation failed.
        return 0.0; // Return a score of 0.0.
    }

    // Create an OpenGL program and attach the shaders to it.
    GLuint program = glCreateProgram(); // Create a new shader program.
    glAttachShader(program, vShader); // Attach the compiled vertex shader.
    glAttachShader(program, fShader); // Attach the compiled fragment shader.
    glLinkProgram(program); // Link the shaders together into a single program.

    // Check if the linking process was successful.
    GLint linkStatus; // Variable to hold the link status.
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus); // Get the link status from the program.
    if (linkStatus != GL_TRUE) { // If linking failed...
        GLchar log[512]; // Create a buffer to hold the error message.
        glGetProgramInfoLog(program, 512, NULL, log); // Get the error log.
        LOGE("Program link failed: %s", log); // Print the error log.
        return 0.0; // Return a score of 0.0.
    }

    glUseProgram(program); // Tell OpenGL to use our compiled and linked shader program for all drawing.
    LOGD("Shaders compiled and linked successfully"); // Log success message.

    // Define the vertices for a shape that covers the entire screen (two triangles making a square).
    GLfloat vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
    // Get the location of the 'vPosition' attribute in our vertex shader.
    GLint posLoc = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(posLoc); // Enable this vertex attribute.
    // Tell OpenGL how to read the vertex data from our 'vertices' array.
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, vertices);

    // Get the location of the 'uTime' and 'uResolution' uniforms in our fragment shader.
    GLint timeLoc = glGetUniformLocation(program, "uTime"); // Location for the time variable.
    GLint resLoc = glGetUniformLocation(program, "uResolution"); // Location for the resolution variable.

    LOGD("Uniform locations - uTime: %d, uResolution: %d", timeLoc, resLoc); // Log their locations for debugging.

    // =========================================================
    // 1. WARM-UP PHASE
    // =========================================================
    // We run the render loop for a few seconds but DO NOT count the frames.
    // This forces the GPU to wake up from low-power states to its maximum performance clock speed.

    LOGD("Starting GPU Warm-up (5000ms)..."); // Log the start of the warm-up phase.
    auto warmStart = std::chrono::high_resolution_clock::now(); // Get the starting time for the warm-up.
    double warmElapsedMs = 0; // Initialize elapsed time for warm-up.

    while (warmElapsedMs < 5000.0) { // Loop for 5 seconds.
        auto now = std::chrono::high_resolution_clock::now(); // Get the current time.
        // Calculate the elapsed time in milliseconds since the warm-up started.
        warmElapsedMs = std::chrono::duration<double, std::milli>(now - warmStart).count();

        // Perform the exact same rendering work as the real benchmark.
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set the screen clear color to black.
        glClear(GL_COLOR_BUFFER_BIT); // Clear the screen.

        // Send the current time and resolution to the fragment shader.
        glUniform1f(timeLoc, (float)warmElapsedMs / 1000.0f); // Update the 'uTime' uniform.
        glUniform2f(resLoc, (float)width, (float)height); // Update the 'uResolution' uniform.

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Draw the square, which triggers the fragment shader for every pixel.
        eglSwapBuffers(display, surface); // Swap the back buffer to the front to show the rendered image.

        // Note: We deliberately do NOT call the Java callback here to keep the UI clean during warm-up.
    }
    LOGD("GPU Warm-up complete. Starting Measurement."); // Log the end of the warm-up.


    // =========================================================
    // 2. REAL BENCHMARK PHASE
    // =========================================================
    // Reset all counters. We act as if the benchmark is just starting *now*.

    int frameCount = 0; // Total number of frames rendered during the benchmark.
    int fpsFrameCount = 0; // Number of frames rendered in the last second, for UI updates.

    // Reset the clock!
    auto start = std::chrono::high_resolution_clock::now(); // Get the official start time of the benchmark.
    auto lastFpsTime = start; // The time when we last calculated the FPS.
    double elapsedMs = 0; // Total elapsed time since the benchmark started.

    // The main benchmark loop. It continues until the desired duration has passed.
    while (elapsedMs < BENCHMARK_DURATION_MS) {
        auto now = std::chrono::high_resolution_clock::now(); // Get the current time.
        elapsedMs = std::chrono::duration<double, std::milli>(now - start).count(); // Update total elapsed time.
        double loopTime = std::chrono::duration<double, std::milli>(now - lastFpsTime).count(); // Time since last FPS update.

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set clear color to black.
        glClear(GL_COLOR_BUFFER_BIT); // Clear the screen.

        // Send the current time and resolution to the fragment shader.
        glUniform1f(timeLoc, (float)elapsedMs / 1000.0f); // Update 'uTime'.
        glUniform2f(resLoc, (float)width, (float)height); // Update 'uResolution'.

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Draw the shape, running the shader on the GPU.

        eglSwapBuffers(display, surface); // Show the newly rendered frame on the screen.

        frameCount++; // Increment the total frame counter.
        fpsFrameCount++; // Increment the counter for the current second.

        // Check if one second has passed since the last FPS update.
        if (loopTime >= 1000.0) {
            // Calculate the current FPS.
            int currentFps = (int)(fpsFrameCount / (loopTime / 1000.0));
            // Send the current FPS to the Kotlin/Java UI thread.
            env->CallVoidMethod(callbackObj, methodId, currentFps);
            fpsFrameCount = 0; // Reset the frame counter for the next second.
            lastFpsTime = now; // Reset the timer for the next FPS calculation.
        }
    }

    cleanupEGL(); // Clean up all EGL and OpenGL resources.

    // Calculate the final average FPS over the entire benchmark duration.
    double avgFps = (double)frameCount / (elapsedMs / 1000.0);
    // Log the final results. The score is just the average FPS multiplied by 100.
    LOGD("Benchmark complete - Avg FPS: %.2f, Score: %.2f", avgFps, avgFps * 100.0);
    return avgFps * 100.0; // Return the final score.
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