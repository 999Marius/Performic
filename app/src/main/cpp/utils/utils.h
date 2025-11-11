//
// Created by Marius on 11/11/2025.
//

#ifndef PERFORMIC_UTILS_H
#define PERFORMIC_UTILS_H

inline void ClobberMemory() {
    asm volatile("" ::: "memory");
}

template <class T>
inline void DoNotOptimize(T const& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

#endif //PERFORMIC_UTILS_H
