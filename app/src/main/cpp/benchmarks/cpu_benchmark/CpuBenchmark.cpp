#include "CpuBenchmark.h"
#include "utils.h"      // CORRECTED: Simplified include path
#include <vector>
#include <chrono>

// --- Benchmark Configuration ---
constexpr int NUM_ITERATIONS = 10;
constexpr int MATRIX_SIZE = 200;

double CpuBenchmark::run() {
    std::vector<double> iteration_times;

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto start_time = std::chrono::high_resolution_clock::now();

        ClobberMemory();
        float result_value = performMatrixMultiplication();
        ClobberMemory();

        auto end_time = std::chrono::high_resolution_clock::now();

        DoNotOptimize(result_value);

        double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        iteration_times.push_back(duration_ms);
    }

    double total_time = 0;
    for (double time : iteration_times) {
        total_time += time;
    }
    return total_time / NUM_ITERATIONS;
}

float CpuBenchmark::performMatrixMultiplication() {
    std::vector<std::vector<float>> a(MATRIX_SIZE, std::vector<float>(MATRIX_SIZE));
    std::vector<std::vector<float>> b(MATRIX_SIZE, std::vector<float>(MATRIX_SIZE));
    std::vector<std::vector<float>> result(MATRIX_SIZE, std::vector<float>(MATRIX_SIZE, 0.0f));

    for (int i = 0; i < MATRIX_SIZE; ++i) {
        for (int j = 0; j < MATRIX_SIZE; ++j) {
            a[i][j] = (float)(i + 1);
            b[i][j] = (float)(j + 1);
        }
    }

    for (int i = 0; i < MATRIX_SIZE; ++i) {
        for (int j = 0; j < MATRIX_SIZE; ++j) {
            for (int k = 0; k < MATRIX_SIZE; ++k) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }

    return result[0][0];
}