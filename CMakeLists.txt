cmake_minimum_required(VERSION 3.20)

project(tomdb
        VERSION 0.1.0
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core database library
add_library(tomdb_lib
        src/testLib.cpp
        include/field.h
        include/tuple.h
        include/file_manager.h
        include/page.h
        src/file_manager.cpp
        include/storage_manager.h
)

target_include_directories(tomdb_lib
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# Database executable
add_executable(tomdb
        src/main.cpp
        src/testLib.cpp
        include/field.h
        include/tuple.h
        include/file_manager.h
        include/page.h
        include/constants.h
        src/file_manager.cpp
        src/file_manager.cpp
        include/storage_manager.h
)

target_link_libraries(tomdb
        PRIVATE
        tomdb_lib
)

# Tests
add_executable(tomdb_tests
        test/main.cpp
        include/field.h
        include/tuple.h
        include/file_manager.h
        include/page.h
        src/file_manager.cpp
        include/storage_manager.h
)

target_link_libraries(tomdb_tests
        PRIVATE
        tomdb_lib
)

target_include_directories(tomdb_tests
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)