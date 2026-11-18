#ifndef TOMDB_NODE_FORMAT_H
#define TOMDB_NODE_FORMAT_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "commons.h"

// Version 1 wire sizes, independent of the C++ node and PageID object layouts.
namespace NodeFormat {
inline constexpr size_t MAX_FILE_MANAGER_NAME_BYTES = 8;
inline constexpr size_t NAME_LENGTH_BYTES = 1;
inline constexpr size_t PAGE_NUMBER_BYTES = 8;
inline constexpr size_t SLOT_INDEX_BYTES = 8;
inline constexpr size_t MAX_PAGE_ID_BYTES =
    NAME_LENGTH_BYTES + MAX_FILE_MANAGER_NAME_BYTES + PAGE_NUMBER_BYTES;
// Magic (4), version/kind/key width/signedness (4), level/count (2 each).
inline constexpr size_t PREFIX_BYTES = 12;
// Self and parent IDs, plus next-leaf ID or the inner node's extra child ID.
inline constexpr size_t MAX_FIXED_BYTES = PREFIX_BYTES + 3 * MAX_PAGE_ID_BYTES;

template<class K>
struct Layout {
    static_assert(std::is_integral_v<K> && !std::is_same_v<K, bool> && sizeof(K) <= 8,
                  "Node format requires an integral key of at most 64 bits (excluding bool)");
    static_assert(PAGE_SIZE >= MAX_FIXED_BYTES, "Page cannot hold node metadata");
    static constexpr size_t LEAF_ENTRY_BYTES = sizeof(K) + MAX_PAGE_ID_BYTES + SLOT_INDEX_BYTES;
    static constexpr size_t INNER_ENTRY_BYTES = sizeof(K) + MAX_PAGE_ID_BYTES;
    static constexpr size_t LEAF_CAPACITY = (PAGE_SIZE - MAX_FIXED_BYTES) / LEAF_ENTRY_BYTES;
    static constexpr size_t INNER_CAPACITY = (PAGE_SIZE - MAX_FIXED_BYTES) / INNER_ENTRY_BYTES;
    static_assert(LEAF_CAPACITY > 0 && INNER_CAPACITY > 0, "Page cannot hold node entries");
    static_assert(LEAF_CAPACITY <= std::numeric_limits<uint16_t>::max() &&
                  INNER_CAPACITY <= std::numeric_limits<uint16_t>::max(),
                  "Node capacity exceeds the encoded count range");
};
}

#endif // TOMDB_NODE_FORMAT_H
