cmake_minimum_required(VERSION 3.10)
project(two_level_bp CXX)

# We use C++17 for std::optional, structured bindings, etc.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Expose headers in include/
include_directories(include)

# Main simulator executable
add_executable(bp_sim
    src/main.cpp
    src/hrt.cpp
    src/pattern_table.cpp
    src/two_level_at.cpp
)
