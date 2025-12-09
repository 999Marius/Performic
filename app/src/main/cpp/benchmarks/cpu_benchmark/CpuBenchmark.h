//
// Created by Marius on 11/11/2025.
//

#ifndef PERFORMIC_CPUBENCHMARK_H
#define PERFORMIC_CPUBENCHMARK_H

#include <stdint.h>

class CpuBenchmark{
public:
    struct Scores {
        double singleCoreScore;
        double multiCoreScore;
        double t1, t2, t3, t4, t5;
    };

    Scores runFullSuite();

private:
    static constexpr int NUM_ITERATIONS = 1;
    static constexpr int COMPRESSION_SIZE = 1000000;
    static constexpr int MATRIX_SIZE = 300;
    static constexpr int INT_ARRAY_SIZE = 25000000;
    static constexpr int LU_MATRIX_SIZE = 500;

    float performMatrixMultiplication();
    long performIntegerWorkload();
    bool performLUDecomposition();
    double performMandelbrot();
    double performDataCompression();

    inline uint32_t mixBits(uint32_t a, uint32_t b, uint32_t c);

    void runThreadedWorkload();

};

#endif //PERFORMIC_CPUBENCHMARK_H
