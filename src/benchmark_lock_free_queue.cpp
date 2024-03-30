// #include <benchmark/benchmark.h>
// #include <thread>

// static void BM_ThreadedFunction(benchmark::State& state) {
//     for (auto _ : state) {
//         int counter = 0;
//         std::thread t([&] { counter++; });
//         t.join();
//         benchmark::DoNotOptimize(counter);
//     }
// }
// // Register the function as a benchmark
// BENCHMARK(BM_ThreadedFunction);

// BENCHMARK_MAIN();

int main() {
    return 0;
}