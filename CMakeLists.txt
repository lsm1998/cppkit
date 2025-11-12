cmake_minimum_required(VERSION 3.25)
project(cppkit VERSION 0.1.0 LANGUAGES CXX)

# Set default build type to Release if not specified
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif ()

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable generation of compile_commands.json for tooling support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define the shared library target
add_library(cppkit SHARED
        src/http/http_client.cpp
        src/http/http_request.cpp
        src/http/http_server.cpp
        src/http/http_router.cpp
        src/io/file.cpp
        src/strings.cpp
        src/arg_parser.cpp
        src/event/ae.cpp
        src/event/server.cpp
        src/http/http_respone.cpp
        src/log/log.cpp
        src/net/socket.cpp
        src/net/udp_datagram.cpp
        src/net/net.cpp
        src/crypto/base.cpp
        src/crypto/md5.cpp
        src/crypto/sha1.cpp
        src/crypto/sha256.cpp
        src/crypto/sha512.cpp
        src/crypto/aes.cpp
        src/concurrency/wait_group.cpp
        src/concurrency/thread_pool.cpp
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