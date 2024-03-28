#include <iostream>
#include <cstdint>

#include "mem_pool.hpp"

int main() {
    struct Example {
        int64_t a;
        char b;
    };

    common::MemPool<Example> example_pool(4);
    common::MemPool<double> double_pool(2);

    double* da = double_pool.allocate(1.0);
    double* db =double_pool.allocate(2.0);
    double_pool.deallocate(da);

    Example* ea = example_pool.allocate(100, 'a');
    Example* eb = example_pool.allocate(100, 'a');
    Example* ec = example_pool.allocate(100, 'a');
    Example* ed = example_pool.allocate(100, 'a');

    example_pool.deallocate(eb);
    Example* ee = example_pool.allocate(100, 'a');
    Example* ef = example_pool.allocate(100, 'a'); // this should cause sysexit
    return 0;
}