cmake_minimum_required(VERSION 3.22.1)
project("performic")

find_library(VULKAN_LIBRARY vulkan)

# Include directories for headers
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks
        ${CMAKE_CURRENT_SOURCE_DIR}/includes
        ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

# ✅ Corrected file paths
add_library(${CMAKE_PROJECT_NAME} SHARED
        benchmarks/BenchmarkCore.cpp
        benchmarks/cpu_benchmark/CpuBenchmark.cpp
        native-lib.cpp
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        ${VULKAN_LIBRARY}
        android
        log
        dl)
