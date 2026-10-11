//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_COMMONS_H
#define TOMDB_COMMONS_H
#include <string>

#include <cstdint>
#include <limits>

inline constexpr std::string DATABASE_FILE_NAME = "tomdb.data";
inline constexpr std::string INDEX_FILE_NAME = "tomdb.index";

inline constexpr size_t PAGE_SIZE = 4096; // bytes

inline constexpr size_t INVALID_VALUE = std::numeric_limits<uint64_t>::max();

inline constexpr size_t INVALID_FRAME_ID = std::numeric_limits<size_t>::max();

inline constexpr size_t MAX_SLOTS = 50;

using FrameID = size_t;

struct PageID {
    std::string fileManagerId;
    size_t fileManagerPageId;

    // Provide an equality operator (required for hash collisions)
    bool operator==(const PageID& other) const {
        return fileManagerPageId == other.fileManagerPageId && fileManagerId == other.fileManagerId;
    }
};

// Inject custom specialization into the std namespace
namespace std {
    template <>
    struct hash<PageID> {
        std::size_t operator()(const PageID& u) const noexcept {
            std::size_t h1 = std::hash<std::string>{}(u.fileManagerId);
            std::size_t h2 = std::hash<size_t>{}(u.fileManagerPageId);

            return h1 ^ (h2 << 1);

        }
    };
}

inline constexpr PageID INVALID_PAGE_ID = PageID{.fileManagerId="", .fileManagerPageId=std::numeric_limits<size_t>::max()};

enum LockMode {
    SHARED,
    EXCLUSIVE
};

inline constexpr size_t MAX_CACHED_PAGES = 15;

#endif //TOMDB_COMMONS_H
