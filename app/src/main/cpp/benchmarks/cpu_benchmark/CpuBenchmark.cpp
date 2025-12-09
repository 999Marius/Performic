#include "CpuBenchmark.h"
#include "utils.h"
#include <vector>
#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>
#include <algorithm>
#include <android/log.h>
#include <numeric>

#define LOG_TAG "PerformicCPU"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// Constants for workloads
constexpr int MANDELBROT_SIZE = 500;
constexpr int MANDELBROT_ITER = 5000;


CpuBenchmark::Scores CpuBenchmark::runFullSuite() {
    for (int i = 0; i < WARMUP_ITERATIONS; ++i){
        float resF = performMatrixMultiplication(); DoNotOptimize(resF);
        long resI = performIntegerWorkload();       DoNotOptimize(resI);
        bool resL = performLUDecomposition();       DoNotOptimize(resL);
        double resC = performDataCompression();     DoNotOptimize(resC);
    }
    LOGD("--- STARTING REALTIME STABILITY SUITE ---");

    std::vector<double> singleHistory;
    std::vector<double> multiHistory;

    double refFloat = 600.0;
    double refInt = 647.0;
    double refLu =  955.0;
    double refCompress = 128.0;

    for (int i = 0; i < STABILITY_ITERATIONS; ++i) {
        // --- A. Float ---
        auto startF = std::chrono::high_resolution_clock::now();
        ClobberMemory();
        float resF = performMatrixMultiplication();
        DoNotOptimize(resF);
        auto endF = std::chrono::high_resolution_clock::now();
        double timeF = std::chrono::duration<double, std::milli>(endF - startF).count();

        // --- B. Integer ---
        auto startI = std::chrono::high_resolution_clock::now();
        ClobberMemory();
        long resI = performIntegerWorkload();
        DoNotOptimize(resI);
        auto endI = std::chrono::high_resolution_clock::now();
        double timeI = std::chrono::duration<double, std::milli>(endI - startI).count();

        // --- C. LU Decomp ---
        auto startL = std::chrono::high_resolution_clock::now();
        ClobberMemory();
        bool resL = performLUDecomposition();
        DoNotOptimize(resL);
        auto endL = std::chrono::high_resolution_clock::now();
        double timeL = std::chrono::duration<double, std::milli>(endL - startL).count();

        // --- D. Compression ---
        auto startC = std::chrono::high_resolution_clock::now();
        ClobberMemory();
        double resC = performDataCompression();
        DoNotOptimize(resC);
        auto endC = std::chrono::high_resolution_clock::now();
        double timeC = std::chrono::duration<double, std::milli>(endC - startC).count();

        // --- Calculate Score for this Iteration ---
        // Avoid division by zero
        double r1 = refFloat / std::max(timeF, 0.001);
        double r2 = refInt / std::max(timeI, 0.001);
        double r3 = refLu / std::max(timeL, 0.001);
        double r4 = refCompress / std::max(timeC, 0.001);

        // Geometric Mean of the 4 tests
        double iterGeoMean = std::pow(r1 * r2 * r3 * r4, 0.25);
        double iterScore = iterGeoMean * 1000.0; // Scale to nice number

        singleHistory.push_back(iterScore);

        // Optional: Small sleep to allow thermal regulation to update slightly?
        // usually not needed if workloads are heavy enough.
    }

    // Final Single Score = Average of the history
    double avgSingleScore = 0.0;
    if (!singleHistory.empty()) {
        double sum = std::accumulate(singleHistory.begin(), singleHistory.end(), 0.0);
        avgSingleScore = sum / singleHistory.size();
    }


    // ==========================================
    // 2. MULTI CORE STABILITY LOOP
    // ==========================================
    unsigned int numCores = std::thread::hardware_concurrency();
    if (numCores == 0) numCores = 4;

    double refMulti = 14395.0; // Reference time for 1 multi-core iteration

    for (int iter = 0; iter < STABILITY_ITERATIONS; ++iter) {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        threads.reserve(numCores);
        for (unsigned int i = 0; i < numCores; ++i) {
            threads.emplace_back(&CpuBenchmark::runThreadedWorkload, this);
        }
        for (auto& t : threads) { if (t.joinable()) t.join(); }

        auto end = std::chrono::high_resolution_clock::now();
        double timeMulti = std::chrono::duration<double, std::milli>(end - start).count();

        // Calculate Score
        double rMulti = refMulti / std::max(timeMulti, 0.001);
        double iterScore = rMulti * 1000.0;

        multiHistory.push_back(iterScore);
    }

    double avgMultiScore = 0.0;
    if (!multiHistory.empty()) {
        double sum = std::accumulate(multiHistory.begin(), multiHistory.end(), 0.0);
        avgMultiScore = sum / multiHistory.size();
    }

    // Return the struct with vectors filled
    return {avgSingleScore, avgMultiScore, singleHistory, multiHistory};
}

// ---------------------------------------------------------
// WORKLOAD FUNCTIONS (UNCHANGED)
// ---------------------------------------------------------

void CpuBenchmark::runThreadedWorkload() {
    double res = performMandelbrot();
    DoNotOptimize(res);
}

float CpuBenchmark::performMatrixMultiplication() {
    int size = MATRIX_SIZE;
    std::vector<float> a(size * size);
    std::vector<float> b(size * size);
    std::vector<float> result(size * size, 0.0f);

    for (int i = 0; i < size * size; ++i) {
        a[i] = (float)((i % 100) + 1);
        b[i] = (float)((i % 50) + 1);
    }

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < size; ++k) {
                sum += a[i * size + k] * b[k * size + j];
            }
            result[i * size + j] = sum;
        }
    }
    return result[0];
}

inline uint32_t CpuBenchmark::mixBits(uint32_t a, uint32_t b, uint32_t c) {
    a -= c;  a ^= ((c << 4) | (c >> 28));  c += b;
    b -= a;  b ^= ((a << 6) | (a >> 26));  a += c;
    c -= b;  c ^= ((b << 8) | (b >> 24));  b += a;
    return a + b + c;
}

long CpuBenchmark::performIntegerWorkload() {
    uint32_t hash = 0xDEADBEEF;
    uint32_t seed = 0x12345678;

    for (int i = 0; i < INT_ARRAY_SIZE; ++i) {
        seed = mixBits(i, seed, hash);
        hash = seed ^ i;
    }
    return (long)hash;
}

bool CpuBenchmark::performLUDecomposition() {
    int n = LU_MATRIX_SIZE;
    std::vector<double> A(n * n);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double value = (double)((i * j + j) % 10);
            if (i == j) value += (double)n;
            A[i * n + j] = value;
        }
    }

    for (int i = 0; i < n; i++) {
        double maxEl = std::abs(A[i * n + i]);
        int maxRow = i;
        for (int k = i + 1; k < n; k++) {
            if (std::abs(A[k * n + i]) > maxEl) {
                maxEl = std::abs(A[k * n + i]);
                maxRow = k;
            }
        }

        for (int k = i; k < n; k++) {
            double tmp = A[maxRow * n + k];
            A[maxRow * n + k] = A[i * n + k];
            A[i * n + k] = tmp;
        }

        double diag = A[i * n + i];
        if (std::abs(diag) < 1e-9) return false;

        for (int k = i + 1; k < n; k++) {
            double c = -A[k * n + i] / diag;
            A[k * n + i] = 0;
            for (int j = i + 1; j < n; j++) {
                A[k * n + j] += c * A[i * n + j];
            }
        }
    }
    return true;
}

double CpuBenchmark::performDataCompression() {
    int n = COMPRESSION_SIZE;
    std::vector<uint8_t> input(n);
    std::vector<uint8_t> output(n * 2);

    for(int i=0; i<n; i++) {
        input[i] = (uint8_t)((i / 10) % 255);
    }

    int outIdx = 0;
    for (int i = 0; i < n; i++) {
        uint8_t count = 1;
        while (i < n - 1 && input[i] == input[i + 1] && count < 255) {
            count++;
            i++;
        }
        output[outIdx++] = count;
        output[outIdx++] = input[i];
    }
    return (double)outIdx;
}

double CpuBenchmark::performMandelbrot() {
    const int WIDTH = MANDELBROT_SIZE;
    const int HEIGHT = MANDELBROT_SIZE;
    const int MAX_ITER = MANDELBROT_ITER;

    double sum = 0.0;

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            double zx = 0.0, zy = 0.0;
            double cx = (x - WIDTH / 2.0) * 4.0 / WIDTH;
            double cy = (y - HEIGHT / 2.0) * 4.0 / HEIGHT;

            int iter = 0;
            while (zx * zx + zy * zy < 4.0 && iter < MAX_ITER) {
                double temp = zx * zx - zy * zy + cx;
                zy = 2.0 * zx * zy + cy;
                zx = temp;
                iter++;
            }
            sum += iter;
        }
    }
    return sum;
}