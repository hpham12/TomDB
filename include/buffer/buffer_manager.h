//
// Created by Hieu Pham on 9/19/26.
//

#ifndef TOMDB_BUFFER_MANAGER_H
#define TOMDB_BUFFER_MANAGER_H
#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <atomic>

#include "buffer_frame.h"
#include "commons.h"
#include "policy.h"
#include "two_queue_policy.h"
#include "records/page.h"
#include "storage/storage_manager.h"

class BufferManager {
    // a vector containing buffer frame cached in memory
    std::vector<std::unique_ptr<BufferFrame>> bufferPool;

    // mutex to guard the buffer pool
    std::mutex bufferPoolMutex;

    // pages that are pinned (locked). Those pages will not be evicted by policy
    std::unordered_set<PageID> pinnedPages;

    // mutex to guard the pinnedPages
    std::mutex pinMutex;

    // map pageId to frameId, where frameId is the index within the bufferPool.
    // As long as the mapping exist it is guaranteed that the page is cached
    std::unordered_map<PageID, FrameID> pageToFrameMapping;

    // mutex to guard the pinnedPages
    std::mutex pageToFrameMappingMutex;

    // cache eviction policy
    std::unique_ptr<Policy> policy = std::make_unique<TwoQPolicy>();

    // frames available to be taken
    std::unordered_set<FrameID> availableFrames;
    std::unique_ptr<StorageManager> storageManager = std::make_unique<StorageManager>();
    // std::unique_ptr<LockTable> lockTable;

    // An array of atomic counters to track who is using a frame
    std::array<std::atomic<uint16_t>, MAX_CACHED_PAGES> pinCounters{};

    void evictPage();
public:
    BufferManager();
    std::unique_ptr<Page> &pinPage(const PageID& pageId, LockMode lockMode);
    void unpinPage(PageID pageId);
    void flushPage(PageID pageId);
    size_t getNumPages(const std::string& fileManagerId) const;
    void registerFileManager(const std::string& fileManagerId, const std::string& filePath) const;
    bool isPinned(PageID pageId) const;
};

#endif //TOMDB_BUFFER_MANAGER_H
