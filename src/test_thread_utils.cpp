#include "thread_utils.hpp"

#include <iostream>


int cum_sum(int n) {
    int sum = (n*(n+1))/2;
    std::cout << "sum of 1 to " << n << " is " << sum << std::endl;
    return sum;
}


int main() {
    std::cout << "running test_thread_utils.cpp" << std::endl;
    int core_id = 3;
    const std::string name = "cum sum thread";
    auto t = common::create_and_start_thread(core_id, name, cum_sum, 100);
    return 0;
}