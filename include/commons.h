//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_COMMONS_H
#define TOMDB_COMMONS_H
#include <string>

#include <cstdint>
#include <limits>

using namespace std;

inline constexpr string DATABASE_FILE_NAME = "tomdb.data";
inline constexpr string INDEX_FILE_NAME = "tomdb.index";

inline constexpr size_t PAGE_SIZE = 4096; // bytes

inline constexpr size_t INVALID_VALUE = numeric_limits<uint64_t>::max();

inline constexpr size_t INVALID_KEY_INDEX = numeric_limits<size_t>::max();

inline constexpr size_t INVALID_FRAME_ID = numeric_limits<size_t>::max();

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

inline constexpr PageID INVALID_PAGE_ID = PageID{.fileManagerId="", .fileManagerPageId=numeric_limits<size_t>::max()};

enum LockMode {
    SHARED,
    EXCLUSIVE
};

inline constexpr size_t MAX_CACHED_PAGES = 15;

struct Record {
    PageID pageId = INVALID_PAGE_ID;
    size_t slotIndex = 0;

    bool operator==(const Record& other) const {
        return other.pageId == pageId && other.slotIndex == slotIndex;
    }
};

inline const Record INVALID_RECORD = Record{.pageId=INVALID_PAGE_ID, .slotIndex=0};

#endif //TOMDB_COMMONS_H
