#include "CpuBenchmark.h"
#include "utils.h"
#include <vector>
#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>
#include <algorithm>
#include <android/log.h>
#define LOG_TAG "PerformicCPU"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

constexpr int MANDELBROT_SIZE = 400;
constexpr int MANDELBROT_ITER = 4000;

CpuBenchmark::Scores CpuBenchmark::runFullSuite() {
    LOGD("--- STARTING FINAL CALIBRATED SUITE ---");

    // 1. Float
    double floatTotalTime = 0;
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        ClobberMemory();
        float res = performMatrixMultiplication();
        ClobberMemory();
        auto end = std::chrono::high_resolution_clock::now();
        DoNotOptimize(res);
        floatTotalTime += std::chrono::duration<double, std::milli>(end - start).count();
    }
    double floatAvg = floatTotalTime / NUM_ITERATIONS;

    // 2. Integer
    double intTotalTime = 0;
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        ClobberMemory();
        long res = performIntegerWorkload();
        ClobberMemory();
        auto end = std::chrono::high_resolution_clock::now();
        DoNotOptimize(res);
        intTotalTime += std::chrono::duration<double, std::milli>(end - start).count();
    }
    double intAvg = intTotalTime / NUM_ITERATIONS;

    // 3. LU Decomp
    double luTotalTime = 0;
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        ClobberMemory();
        bool res = performLUDecomposition();
        ClobberMemory();
        auto end = std::chrono::high_resolution_clock::now();
        DoNotOptimize(res);
        luTotalTime += std::chrono::duration<double, std::milli>(end - start).count();
    }
    double luAvg = luTotalTime / NUM_ITERATIONS;

    // 4. Compression
    double compressTotalTime = 0;
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        ClobberMemory();
        double res = performDataCompression();
        ClobberMemory();
        auto end = std::chrono::high_resolution_clock::now();
        DoNotOptimize(res);
        compressTotalTime += std::chrono::duration<double, std::milli>(end - start).count();
    }
    double compressAvg = compressTotalTime / NUM_ITERATIONS;




    double refFloat = 600.0;
    double refInt = 647.0;
    double refLu = 955.0;
    double refCompress = 128.0;

    double r1 = refFloat / std::max(floatAvg, 0.001);
    double r2 = refInt / std::max(intAvg, 0.001);
    double r3 = refLu / std::max(luAvg, 0.001);
    double r4 = refCompress / std::max(compressAvg, 0.001);

    double geoMean = std::pow(r1 * r2 * r3 * r4, 0.25);
    double singleCoreScore = geoMean * 1000.0;


//multi core bench
    unsigned int numCores = std::thread::hardware_concurrency();
    if (numCores == 0) numCores = 4;

    double multiCoreTotalTime = 0;
    int multiIterations = 50;

    for (int iter = 0; iter < multiIterations; ++iter) {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        threads.reserve(numCores);
        for (unsigned int i = 0; i < numCores; ++i) {
            threads.emplace_back(&CpuBenchmark::runThreadedWorkload, this);
        }
        for (auto& t : threads) { if (t.joinable()) t.join(); }
        auto end = std::chrono::high_resolution_clock::now();
        multiCoreTotalTime += std::chrono::duration<double, std::milli>(end - start).count();
    }

    double multiAvg = multiCoreTotalTime / multiIterations;

    double refMulti = 11516.0;

    double rMulti = refMulti / std::max(multiAvg, 0.001);
    double multiCoreScore = rMulti * 1000.0;

    return {singleCoreScore, multiCoreScore};
}

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