//
// Created by Hieu Pham on 9/19/26.
//

#ifndef TOMDB_BUFFER_MANAGER_H
#define TOMDB_BUFFER_MANAGER_H
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "constants.h"
#include "policy.h"

class BufferFrame;

class BufferManager {
    std::vector<std::unique_ptr<BufferFrame>> bufferPool;
    std::unordered_set<PageID> pinnedPages;
    std::unordered_map<PageID, FrameID> pageToFrameMapping;
    std::unique_ptr<Policy> policy;
    // std::unique_ptr<LockTable> lockTable;

public:
    void pinPage(PageID pageId, LockMode lockMode);
    void unpinPage(PageID pageId);
    size_t getNumPages(std::string &fileManagerId);
    void flushPage(PageID pageId);
    void evictPage(PageID pageId);
};

#endif //TOMDB_BUFFER_MANAGER_H
