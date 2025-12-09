#ifndef PERFORMIC_MEMORYBENCHMARK_H
#define PERFORMIC_MEMORYBENCHMARK_H

class MemoryBenchmark {
public:
    struct MemoryScores {
        double l1Throughput;
        double l2Throughput;
        double ramThroughput;
        double memoryScore;
    };

    MemoryScores runMemorySuite();

private:
    double measureBandwidth(int bufferSize);
};

#endif //PERFORMIC_MEMORYBENCHMARK_H