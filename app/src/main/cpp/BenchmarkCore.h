//
// Created by Marius on 09/11/2025.
//

#ifndef PERFORMIC_BENCHMARKCORE_H
#define PERFORMIC_BENCHMARKCORE_H

#include <string>

class BenchmarkCore {
public:
    // The main function to run all benchmarks.
    // It will return a string formatted as JSON.
    std::string runFullBenchmark();

private:
    // Our new thermal check gatekeeper.
    bool isDeviceCoolEnough();
};

#endif //PERFORMIC_BENCHMARKCORE_H