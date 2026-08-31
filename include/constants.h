//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_CONSTANTS_H
#define TOMDB_CONSTANTS_H

#include <cstdint>
#include <limits>

#define DATABASE_FILE_NAME "tomdb.data"
#define INDEX_FILE_NAME "tomdb.index"

using namespace std;

inline constexpr size_t PAGE_SIZE = 4096;

inline constexpr size_t INVALID_VALUE = numeric_limits<uint64_t>::max();

#endif //TOMDB_CONSTANTS_H
