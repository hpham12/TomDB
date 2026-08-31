cmake_minimum_required(VERSION 3.20)

project(tomdb
        VERSION 0.1.0
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------
# GoogleTest
# --------------------

include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

# --------------------
# Core database library
# --------------------

add_library(tomdb_lib
        src/testLib.cpp
        src/file_manager.cpp
)

target_include_directories(tomdb_lib
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# --------------------
# Database executable
# --------------------

add_executable(tomdb
        src/main.cpp
        src/page.cpp
)

target_link_libraries(tomdb
        PRIVATE
        tomdb_lib
)

# --------------------
# Tests
# --------------------

add_executable(tomdb_tests
        test/hello_test.cpp
)

target_link_libraries(tomdb_tests
        PRIVATE
        tomdb_lib
        GTest::gtest_main
)