cmake_minimum_required(VERSION 3.10)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(hft VERSION 1.0)

add_compile_options(-Wall -Wextra -Wpedantic)
# regular executables
add_executable(market_data src/market_data.cpp)
# TEST
add_executable(test_thread_utils src/test_thread_utils.cpp)
add_executable(test_mem_pool src/test_mem_pool.cpp)
add_executable(test_lock_free_queue src/test_lock_free_queue.cpp)
# BENCHMARK
add_executable(benchmark_lock_free_queue src/benchmark_lock_free_queue.cpp)
# PROFILING
# ...

# GoogleTest setup - this allows us to add tests here later
enable_testing()

find_package(Boost 1.75 REQUIRED COMPONENTS json)
find_package(OpenSSL REQUIRED)
find_package(benchmark REQUIRED)
find_package(GTest REQUIRED)

# Configure common test, benchmark, and profiling interfaces
# these are composed of external libraries
add_library(common_test_interface INTERFACE)
add_library(common_benchmark_interface INTERFACE)
add_library(common_profiling_interface INTERFACE)

target_link_libraries(common_test_interface INTERFACE GTest::GTest GTest::Main)
target_link_libraries(common_benchmark_interface INTERFACE benchmark::benchmark)
target_link_libraries(common_profiling_interface INTERFACE profiler)

# Link test, benchmarking, and profiling executables with their required interfaces
# TEST
target_link_libraries(test_thread_utils PRIVATE common_test_interface)
target_link_libraries(test_lock_free_queue PRIVATE common_test_interface)
# BENCHMARK
#...

# PROFILING
#...

# ThreadSanitizer
target_compile_options(test_lock_free_queue PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
target_compile_options(test_thread_utils PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
target_link_options(test_lock_free_queue PRIVATE -fsanitize=thread)
target_link_options(test_thread_utils PRIVATE -fsanitize=thread)


# Link Boost libraries
if(Boost_FOUND)
    target_include_directories(market_data PUBLIC ${Boost_INCLUDE_DIRS})
    target_link_libraries(market_data ${Boost_LIBRARIES})
endif()

# Link OpenSSL libraries
if(OPENSSL_FOUND)
    target_include_directories(market_data PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(market_data ${OPENSSL_LIBRARIES})
endif()


target_include_directories(market_data PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)