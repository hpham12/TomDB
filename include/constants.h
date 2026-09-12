//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_CONSTANTS_H
#define TOMDB_CONSTANTS_H
#include <string>

#include <cstdint>
#include <limits>

using namespace std;

inline constexpr string DATABASE_FILE_NAME = "tomdb.data";
inline constexpr string INDEX_FILE_NAME = "tomdb.index";

inline constexpr size_t PAGE_SIZE = 4096; // bytes

inline constexpr size_t INVALID_VALUE = numeric_limits<uint64_t>::max();

inline constexpr size_t MAX_SLOTS = 50;

#endif //TOMDB_CONSTANTS_H
