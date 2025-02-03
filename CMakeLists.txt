cmake_minimum_required(VERSION 3.10)    # Specify the minimum CMake version
project(logger_test VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)              # Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD_REQUIRED True)   # Require the specified C++ standard
set(CMAKE_CXX_EXTENSIONS OFF)           # Disable compiler-specific extensions

# Set optimization flags for Release build
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Find the Qt5Widgets package
find_package(Qt5Widgets REQUIRED)

# Specify the source files
set(SOURCES
    src/main.cpp
    src/logger.cpp
    src/util.cpp
)

# Create the executable
add_executable(main ${SOURCES})

# Link the Qt5Widgets library
target_link_libraries(main Qt5::Widgets)

# Set compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(main PRIVATE -Wall -Wextra)
endif()
