cmake_minimum_required(VERSION 3.20)

project(tomdb
        VERSION 0.1.0
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------
# Options
# --------------------

option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

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
        src/records/field.cpp
        src/records/tuple.cpp
        src/records/page.cpp
        src/storage/file_manager.cpp
        src/storage/storage_manager.cpp
        src/buffer/two_queue_policy.cpp
        src/buffer/buffer_manager.cpp
        src/buffer/buffer_frame.cpp
)

target_include_directories(tomdb_lib
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# --------------------
# Coverage
# --------------------

if(ENABLE_COVERAGE)
    message(STATUS "Code coverage enabled")

    target_compile_options(tomdb_lib
            PRIVATE
            --coverage
            -fprofile-abs-path
    )

    target_link_options(tomdb_lib
            PRIVATE
            --coverage
    )
endif()

# --------------------
# Database executable
# --------------------

add_executable(tomdb
        src/records/page.cpp
        src/records/field.cpp
        src/records/tuple.cpp
        src/storage/storage_manager.cpp
        src/storage/file_manager.cpp
        src/main.cpp
        src/buffer/two_queue_policy.cpp
        src/buffer/buffer_manager.cpp
        include/buffer/buffer_frame.h
        src/buffer/buffer_frame.cpp
)

target_link_libraries(tomdb
        PRIVATE
        tomdb_lib
)

if(ENABLE_COVERAGE)
    target_link_options(tomdb
            PRIVATE
            --coverage
    )
endif()

# --------------------
# Tests
# --------------------

add_executable(tomdb_tests
        test/records/field_test.cpp
        test/records/tuple_test.cpp
        test/records/page_test.cpp
        test/storage/file_manager_test.cpp
        test/storage/storage_manager_test.cpp
        test/buffer/two_queue_policy_test.cpp
        test/buffer/buffer_manager_test.cpp
        test/test_utils.h
)

target_link_libraries(tomdb_tests
        PRIVATE
        tomdb_lib
        GTest::gtest_main
)

if(ENABLE_COVERAGE)
    target_link_options(tomdb_tests
            PRIVATE
            --coverage
    )
endif()

enable_testing()

include(GoogleTest)
gtest_discover_tests(tomdb_tests)

# --------------------
# Coverage report
# --------------------

if(ENABLE_COVERAGE)

    find_program(GCOVR gcovr REQUIRED)

    add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND}
            --output-on-failure

            COMMAND ${GCOVR}
            --root ${PROJECT_SOURCE_DIR}
            --object-directory ${CMAKE_BINARY_DIR}
            --sonarqube ${CMAKE_BINARY_DIR}/coverage.xml

            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}

            DEPENDS tomdb_tests

            COMMENT "Running tests and generating SonarQube coverage report"
    )

endif()

# --------------------
# Clang-Tidy
# --------------------

find_program(CLANG_TIDY clang-tidy REQUIRED)

add_custom_target(tomdb_clang_tidy
        COMMAND ${CLANG_TIDY}
        src/records/field.cpp
        src/records/tuple.cpp
        src/storage/file_manager.cpp
        src/records/page.cpp
        src/storage/storage_manager.cpp
        --
        -std=c++20
        -I${PROJECT_SOURCE_DIR}/include

        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Running clang-tidy"
)