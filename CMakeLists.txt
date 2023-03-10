cmake_minimum_required(VERSION 3.24)
project(job_system)

set(CMAKE_CXX_STANDARD 23)

add_executable(job_system src/main.cpp include/job_system.hpp src/job_system.cpp include/thread_safe_ring_buffer.hpp)
target_include_directories(job_system PRIVATE include/)