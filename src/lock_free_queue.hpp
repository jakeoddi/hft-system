#pragma once

#include <vector>
#include <atomic>
#include <optional>
#include <new>

namespace common {
    template <typename T>
    class LFQueue {
    private:

        // compile-time check
        static_assert(
            std::atomic<size_t>::is_always_lock_free, 
            "atomic size_t is not lock-free."
        );

        // define cache line size
        #ifdef __cpp_lib_hardware_interference_size
            using std::hardware_constructive_interference_size;
            using std::hardware_destructive_interference_size;
        #else
            // 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
            static constexpr std::size_t hardware_constructive_interference_size = 64;
            static constexpr std::size_t hardware_destructive_interference_size = 64;
        #endif

        size_t capacity_;
        std::vector<T> store_;
        // avoid false sharing, which leads to waiting, by aligning different 
        // thread work on different cache lines when using atomics
        // on NUMA machines may even need to align to different pages

        // x86 cache lines are aligned to 64-byte boundaries in memory,
        // meaning cache lines start at addresses that are multiples of 64.
        // this makes it easy to index in the cache.
        // aligning the idx to the size of the cache line prevents false sharing.
        alignas(hardware_destructive_interference_size) std::atomic<size_t> cur_write_idx_{};
        alignas(hardware_destructive_interference_size) std::atomic<size_t> cur_read_idx_{};
        alignas(hardware_destructive_interference_size) std::atomic<size_t> size_{};
        // TODO: put read_idx and write_idx on separate cache lines, then evaluate perf
        // should capacity be aligned too?

        // not sure if this is also necessary for false sharing but saw in cppcon23
        // test if this improves performance
        // char padding_[hardware_destructive_interference_size - sizeof(size_t)];


        size_t increment(size_t idx) {
            return (idx + 1) % capacity_;
        }

    public:
        // init to the provided capacity+1 because of the way push and 
        // pop check if the buffer is empty/full
        LFQueue(int capacity) : capacity_(capacity+1), store_(capacity+1) {}

        LFQueue() = delete;
        LFQueue(const LFQueue&) = delete;
        LFQueue(const LFQueue&&) = delete;
        LFQueue& operator=(const LFQueue&) = delete;
        LFQueue& operator=(const LFQueue&&) = delete;
        
        //std::memory_order_acquire: operations before the barrier can 
        // be moved to after it, but operations that are after it cannot
        // be moved to before it

        // release barrier is the reverse

        // even if only one variable x is atomic, if you set up an acquire-release
        // barrier on x, you can essentially publish data from the release-store thread
        // because all its operations, atomic and otherwise, only become visible
        // to the other, acquire-load thread after the barrier. So even though
        // some data may not be atomic, it is protected by this memory barrier.

        // acq_rel barrier is bidirectional, no operation can move accross it
        // as long as both threads use the same atomic variable. on x86 this is
        // the same thing as sequential consistency - the most restrictive barrier
        // and default barrier
        template <typename U>
        bool push(U&& obj) {
            // TODO: push() and pop() both need to be benchmarked to decide if 
            // move semantics or perfect forwarding are even necessary.
            // For trivially copyable types, they can introduce unnecessary
            // overhead

            // this enables us to accept rvalues or lvalues without having to 
            // deepcopy
            static_assert(
                std::is_same<T, std::decay_t<U>>::value, 
                "Type U must be the same as LFQueue element type T"
            );
            size_t write_idx = cur_write_idx_.load(std::memory_order_relaxed);
            write_idx = increment(write_idx);
            // fail if the buffer is full
            if (write_idx == cur_read_idx_.load(std::memory_order_acquire)) {
                return false;
            }
            store_[write_idx] = std::forward<U>(obj);
            cur_write_idx_.store(write_idx, std::memory_order_release);
            // size_.fetch_add(1, std::memory_order_relaxed);
            size_.fetch_add(1);
            return true;
        }
        // TODO: test a split version of this that returns pointer,
        // where other function updates the idx. leaves more for the user to do but could be faster
        std::optional<T> pop() { 
            size_t read_idx = cur_read_idx_.load(std::memory_order_relaxed);
            // fail if the buffer is empty
            if (read_idx == cur_write_idx_.load(std::memory_order_acquire)) {
                return std::nullopt;
            }
            read_idx = increment(read_idx);
            T out = std::move(store_[read_idx]);
            cur_read_idx_.store(read_idx, std::memory_order_release);
            // size_.fetch_sub(1, std::memory_order_relaxed);
            size_.fetch_sub(1);
            return out;
        }

        size_t size() {
            return size_.load(); // default sequential consistency
        }

    };
}