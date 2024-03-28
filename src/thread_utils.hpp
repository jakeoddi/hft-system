#pragma once

#include <thread>
#include <atomic>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

namespace common {
    inline bool set_thread_core(int core_id) {
        cpu_set_t cpuset;
        
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);

        return (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0);

    }

    template <typename T, typename... A>
    inline auto create_and_start_thread(int core_id, const std::string& name, T&& func, A&&... args) {
        std::atomic<bool> running(false), failed(false);

        auto thread_body = [&]() {
            if (core_id >= 0 && !set_thread_core(core_id)) {
                std::cerr << "Failed to set core affinity for " << name << " "
                << pthread_self() << " to " << core_id << std::endl;
                failed = true;
                return;
            }
            std::cout << " Set core affinity for " << name << " " << pthread_self() 
            << " to " << core_id << std::endl;

            running = true;
            std::forward<T>(func) ((std::forward<A>(args))...);
        };

        auto t = new std::thread(thread_body);
        
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s); // allow time for setup

        return t;
    }
}
