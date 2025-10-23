cmake_minimum_required(VERSION 3.25)
project(cppkit VERSION 0.1.0 LANGUAGES CXX)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define the shared library target
add_library(cppkit SHARED
    src/cppkit.cpp
)

# Specify include directories for the target
target_include_directories(cppkit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Set properties for the shared library
set_target_properties(cppkit PROPERTIES
    OUTPUT_NAME "cppkit"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_subdirectory(tests)