cmake_minimum_required(VERSION 3.20)
project(orderbook CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Which implementation to build (folder under implementations/)
set(IMPL "reference" CACHE STRING "Implementation to build")

# Workload profile for the benchmark (BALANCED | ADD_HEAVY | MODIFY_HEAVY | QUERY_HEAVY)
set(WORKLOAD "BALANCED" CACHE STRING "Benchmark workload profile")

add_compile_options(-O3 -march=native -Wall -Wextra -fno-omit-frame-pointer)

# Static library for the chosen implementation
add_library(engine_impl STATIC
    implementations/${IMPL}/engine.cpp
)
target_include_directories(engine_impl PUBLIC
    implementations/${IMPL}
    common
)

# Test executable — always enable asserts regardless of build type
add_executable(run_tests tests/tests.cpp)
target_link_libraries(run_tests PRIVATE engine_impl)
target_compile_options(run_tests PRIVATE -UNDEBUG)

# Benchmark executable
add_executable(bench bench/main.cc)
target_link_libraries(bench PRIVATE engine_impl)

# Forward the chosen workload profile as a preprocessor definition.
# BALANCED is the default (no define needed; handled by #else in bench/main.cc).
if(NOT WORKLOAD STREQUAL "BALANCED")
    target_compile_definitions(bench PRIVATE WORKLOAD_${WORKLOAD})
endif()

# Optional Sniper integration
if(DEFINED SNIPER_ROOT)
    target_compile_definitions(bench PRIVATE USE_SNIPER_ROI)
    target_include_directories(bench PRIVATE ${SNIPER_ROOT}/include)
    target_link_directories(bench PRIVATE ${SNIPER_ROOT}/lib)
    target_link_libraries(bench PRIVATE sift)
endif()
