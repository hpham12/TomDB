//
// Created by Hieu Pham on 9/19/26.
//

#ifndef TOMDB_BUFFER_MANAGER_H
#define TOMDB_BUFFER_MANAGER_H
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "commons.h"
#include "policy.h"
#include "two_queue_policy.h"
#include "records/page.h"
#include "storage/storage_manager.h"

class BufferFrame;

class BufferManager {
    std::vector<std::unique_ptr<BufferFrame>> bufferPool;
    std::unordered_set<PageID> pinnedPages;
    std::unordered_map<PageID, FrameID> pageToFrameMapping;
    std::unique_ptr<Policy> policy = std::make_unique<TwoQPolicy>();
    std::unordered_set<FrameID> availableFrames;
    std::unique_ptr<StorageManager> storageManager = std::make_unique<StorageManager>();
    // std::unique_ptr<LockTable> lockTable;

    void flushPage(PageID pageId);
    void evictPage(PageID pageId);
    void unpinPage(PageID pageId);
public:
    BufferManager();
    std::unique_ptr<Page> &pinPage(const PageID& pageId, LockMode lockMode);
    size_t getNumPages(const std::string& fileManagerId) const;
    void registerFileManager(const std::string& fileManagerId, const std::string& filePath) const;
};

class BufferFrame {
    std::unique_ptr<Page> page = nullptr;
    PageID pageId = INVALID_PAGE_ID;
    FrameID frameId = INVALID_VALUE;
    bool isDirty = false;

    friend class BufferManager;

public:
    BufferFrame() = default;
    BufferFrame(PageID pageId, FrameID, std::unique_ptr<Page> page);
    void markDirty();

};

#endif //TOMDB_BUFFER_MANAGER_H
