//
// Created by Hieu Pham on 9/19/26.
//

#ifndef TOMDB_BUFFER_MANAGER_H
#define TOMDB_BUFFER_MANAGER_H
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "buffer_frame.h"
#include "commons.h"
#include "policy.h"
#include "two_queue_policy.h"
#include "records/page.h"
#include "storage/storage_manager.h"

class BufferManager {
    std::vector<std::unique_ptr<BufferFrame>> bufferPool;
    std::unordered_set<PageID> pinnedPages;
    std::unordered_map<PageID, FrameID> pageToFrameMapping;
    std::unique_ptr<Policy> policy = std::make_unique<TwoQPolicy>();
    std::unordered_set<FrameID> availableFrames;
    std::unique_ptr<StorageManager> storageManager = std::make_unique<StorageManager>();
    // std::unique_ptr<LockTable> lockTable;

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
