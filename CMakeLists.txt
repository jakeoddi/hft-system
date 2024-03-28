cmake_minimum_required(VERSION 3.10)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(hft VERSION 1.0)

add_executable(market_data src/market_data.cpp)
add_executable(test_thread_utils src/test_thread_utils.cpp)
add_executable(test_mem_pool src/test_mem_pool.cpp)

find_package(Boost 1.75 REQUIRED COMPONENTS json)
find_package(OpenSSL REQUIRED)

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