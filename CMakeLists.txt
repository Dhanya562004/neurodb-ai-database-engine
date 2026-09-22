cmake_minimum_required(VERSION 3.10)
project(NeuroDB CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Executable for NeuroDB Database Engine
add_executable(neurodb src/main.cpp)

# Executable for Seeding Test Data
add_executable(setup_test_data src/setup_test_data.cpp)
