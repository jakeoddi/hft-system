#include "thread_utils.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <iostream>

void simple_counter(int& counter) {
    // Simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    counter++;
}

TEST(ThreadedFunctionTest, IncrementsCounter) {
    int counter = 0;
    int core_id = 1;
    std::thread* t = common::create_and_start_thread(
        core_id, 
        "test_simple_counter", 
        simple_counter, 
        counter
    );
    t->join(); 
    ASSERT_EQ(counter, 1);
}

TEST(ThreadedFunctionTest, FailsForInvalidCore) {
    int counter = 0;
    int core_id = 1e5;
    std::thread* t = common::create_and_start_thread(
        core_id, 
        "test_invalid_core", 
        simple_counter, 
        counter
    );
    t->join(); 
    ASSERT_EQ(counter, 0);
}

void concurrent_counter(std::atomic<int>& counter) {
    // Simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    counter.fetch_add(1, std::memory_order_relaxed);
}

TEST(ThreadedFunctionTest, ConcurrentIncrements) {
    std::atomic<int> counter(0);
    constexpr int numThreads = 10;
    std::vector<std::thread*> threads;

    // Create and start multiple threads
    for (int i = 0; i < numThreads; ++i) {
        std::thread* t = common::create_and_start_thread(
            -1, // Using -1 for core_id to let OS decide the affinity
            "test_concurrent_counter_" + std::to_string(i),
            concurrent_counter, 
            counter
        );
        threads.push_back(t);
    }

    // Join all threads
    for (auto& t : threads) {
        t->join(); 
        delete t; // Clean up dynamic allocation
    }

    // Verify the counter's final value
    ASSERT_EQ(counter.load(std::memory_order_relaxed), numThreads);
}