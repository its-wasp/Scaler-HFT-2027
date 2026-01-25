cmake_minimum_required(VERSION 3.16)
project(MarketDataSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost REQUIRED COMPONENTS system)

# Fetch fmt library
include(FetchContent)
message(STATUS "fmt not found, fetching from GitHub...")
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.1.0
)
FetchContent_MakeAvailable(fmt)

# Publisher executable
add_executable(publisher src/publisher.cpp)
target_link_libraries(publisher PRIVATE Boost::system fmt::fmt pthread rt)
target_include_directories(publisher PRIVATE ${CMAKE_SOURCE_DIR}/include)

# SHM Consumer executable
add_executable(shm_consumer src/shm_consumer.cpp)
target_link_libraries(shm_consumer PRIVATE fmt::fmt pthread rt)
target_include_directories(shm_consumer PRIVATE ${CMAKE_SOURCE_DIR}/include)

# TCP Consumer executable
add_executable(tcp_consumer src/tcp_consumer.cpp)
target_link_libraries(tcp_consumer PRIVATE Boost::system fmt::fmt pthread)
target_include_directories(tcp_consumer PRIVATE ${CMAKE_SOURCE_DIR}/include)
