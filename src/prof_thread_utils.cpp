#include <gperftools/profiler.h>

int main(int argc, char** argv) {
    ProfilerStart("my_app.prof"); // Start profiling
    // Your application code here

    ProfilerStop(); // Stop profiling
    return 0;
}
