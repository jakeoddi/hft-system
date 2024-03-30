#include "thread_utils.hpp"
#include "lock_free_queue.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <iostream>


template<typename T>
class LFQueueTest : public ::testing::Test {
protected:
    common::LFQueue<T> queue{10}; // capacity 10
    common::LFQueue<T> queue2{100}; // capacity 100
};

// Instantiate tests for int type
using IntLFQueueTest = LFQueueTest<int>;

TEST_F(IntLFQueueTest, PushPopOneThread) {
    ASSERT_TRUE(queue.push(1));
    auto result = queue.pop();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), 1);
}

TEST_F(IntLFQueueTest, PopFromEmptyQueueOneThread) {
    auto result = queue.pop();
    ASSERT_FALSE(result.has_value());
}

TEST_F(IntLFQueueTest, PushToFullQueueOneThread) {
    ASSERT_EQ(queue.size(), 0);
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(queue.push(i)); // push accepts lvalues
    }
    ASSERT_FALSE(queue.push(11)); // This should fail since the queue is full
}

TEST_F(IntLFQueueTest, PushPopTwoThreads) {
    constexpr int numIterations = 1e6;
    std::vector<int> pushedValues(numIterations);
    std::vector<int> poppedValues(numIterations);

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < numIterations; ++i) {
            while (!queue.push(i)); // Keep trying until push succeeds
            pushedValues[i] = i;
        }
    });

    // Consumer thread
    std::thread consumer([&]() {
        for (int i = 0; i < numIterations; ++i) {
            std::optional<int> val;
            do {
                val = queue.pop(); // Keep trying until pop succeeds
            } while (!val.has_value());
            poppedValues[i] = val.value();
        }
    });

    producer.join();
    consumer.join();

    // Verify that all pushed values were popped
    std::sort(poppedValues.begin(), poppedValues.end()); // Sort because the order might not be preserved
    ASSERT_EQ(pushedValues, poppedValues);
}

// INTEGRATION TEST
// Function to be run by the producer thread
void producerWork(common::LFQueue<int>& q, int numIterations) {
    for (int i = 0; i < numIterations; ++i) {
        while (!q.push(i)); // Keep trying until push succeeds
    }
}

// Function to be run by the consumer thread
void consumerWork(common::LFQueue<int>& q, std::vector<int>& outVec, int numIterations) {
    for (int i = 0; i < numIterations; ++i) {
        std::optional<int> val;
        do {
            val = q.pop(); // Keep trying until pop succeeds
        } while (!val.has_value());
        outVec[i] = val.value();
    }
}

TEST_F(IntLFQueueTest, PushPopTwoCustomThreads) {
    constexpr int numIterations = 1e6;
    std::vector<int> poppedValues(numIterations);

    // Start producer thread
    auto producerThread = common::create_and_start_thread(-1, "producer", producerWork, queue2, numIterations);

    // Start consumer thread
    auto consumerThread = common::create_and_start_thread(-1, "consumer",  consumerWork, queue2, poppedValues, numIterations);

    // Wait for threads to complete their work
    producerThread->join();
    consumerThread->join();

    // Clean up
    delete producerThread;
    delete consumerThread;

    // Verify that all values pushed to the queue were popped
    std::sort(poppedValues.begin(), poppedValues.end()); // Sort because the order might not be preserved
    for (int i = 0; i < numIterations; ++i) {
        ASSERT_EQ(poppedValues[i], i);
    }
}