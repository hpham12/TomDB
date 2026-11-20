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

FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
)
FetchContent_MakeAvailable(nlohmann_json)

# --------------------
# Core database library
# --------------------

add_library(tomdb_lib
        src/metadata.cpp
        src/query/insert_json.cpp
        src/query/operators.cpp
        src/query/query_processor.cpp
        src/records/field.cpp
        src/records/tuple.cpp
        src/records/page.cpp
        src/storage/file_manager.cpp
        src/storage/storage_manager.cpp
        src/buffer/two_queue_policy.cpp
        src/buffer/buffer_manager.cpp
        src/buffer/buffer_frame.cpp
        src/buffer/frame_lock_table.cpp
        src/buffer/page_guard.cpp
)

target_include_directories(tomdb_lib
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# Public headers use gtest/gtest_prod.h for FRIEND_TEST declarations.
# Propagate GoogleTest's include requirements to consumers of those headers.
target_link_libraries(tomdb_lib
        PUBLIC
        GTest::gtest
        PRIVATE
        nlohmann_json::nlohmann_json
)

# --------------------
# Coverage
# --------------------

if(ENABLE_COVERAGE)
    message(STATUS "Code coverage enabled")

    # Consumers instantiate header-only and template code, so instrument them too.
    target_compile_options(tomdb_lib
            PUBLIC
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
        src/buffer/buffer_frame.cpp
        src/buffer/page_guard.cpp
        include/query/query_processor.h
        include/query/operators.h
        src/query/query_processor.cpp
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
        test/commons_test.cpp
        test/records/field_test.cpp
        test/records/tuple_test.cpp
        test/records/page_test.cpp
        test/storage/file_manager_test.cpp
        test/storage/storage_manager_test.cpp
        test/buffer/two_queue_policy_test.cpp
        test/buffer/buffer_manager_test.cpp
        test/test_utils.h
        test/buffer/buffer_frame_test.cpp
        test/buffer/frame_lock_table_test.cpp
        test/buffer/page_guard_test.cpp
        test/index/b_tree_test.cpp
        test/index/node_test.cpp
        test/index/node_serializer_test.cpp
        test/query/insert_json_test.cpp
        test/query/query_processor_test.cpp
        include/query/query_processor.h
        test/metadata_test.cpp
        test/query/operators_test.cpp
)

target_link_libraries(tomdb_tests
        PRIVATE
        tomdb_lib
        GTest::gmock
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
            --filter ${PROJECT_SOURCE_DIR}/src/
            --filter ${PROJECT_SOURCE_DIR}/include/
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
