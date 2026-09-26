//
// Created by Hieu Pham on 9/19/26.
//

#include "buffer/buffer_manager.h"

#include <functional>

#include "buffer/buffer_frame.h"

BufferManager::BufferManager() {
    for (size_t i = 0; i < MAX_CACHED_PAGES; i++) {
        bufferPool.emplace_back(std::make_unique<BufferFrame>());
        availableFrames.insert(static_cast<FrameID>(i));
    }
}

std::unique_ptr<BufferFrame> &BufferManager::pinPage(const PageID &pageId, LockMode lockMode) noexcept(false) {
    {
        std::lock_guard pinGuard(pinMutex);
        std::lock_guard bufferGuard(bufferPoolMutex);
        // Cached pages retain their contents and dirty state between pins.
        if (pageToFrameMapping.contains(pageId)) {
            policy->accessPage(pageId);
            auto frameId = pageToFrameMapping[pageId];
            auto &frame = bufferPool.at(frameId);
            pinnedPages.insert(pageId);
            ++pinCounters[frameId];
            return frame;
        }
    }

    FrameID frameId;

    if (policy->isCacheFull()) {
        evictPage();
    }

    {
        std::shared_lock guard(availableFramesMutex);
        // find an available frame
        if (availableFrames.empty()) {
            throw std::logic_error("Error: No frame available. This is likely a implementation logic error!");
        }
        frameId = *availableFrames.begin();
    }

    if (pageId.fileManagerPageId >= getNumPages(pageId.fileManagerId)) {
        storageManager->extend(pageId.fileManagerId, pageId.fileManagerPageId);
    }

    std::unique_ptr<Page> page = storageManager->getPage(pageId);

    policy->accessPage(pageId);

    std::lock_guard bufferGuard(bufferPoolMutex);
    bufferPool[frameId]->isDirty = false;
    bufferPool[frameId]->frameId = frameId;
    bufferPool[frameId]->pageId = pageId;
    bufferPool[frameId]->page = std::move(page);
    pageToFrameMapping[pageId] = frameId;
    availableFrames.erase(frameId);

    if (pinCounters[frameId] == 0) {
        pinnedPages.insert(pageId);
    }

    ++pinCounters[frameId];

    return bufferPool[frameId];
}

void BufferManager::unpinPage(PageID pageId) {
    {
        std::shared_lock guard(pinMutex);
        if (!pinnedPages.contains(pageId)) {
            throw std::logic_error("Error: page is not pinned");
        }
    }

    auto frameId = pageToFrameMapping[pageId];
    --pinCounters[frameId];

    if (pinCounters[frameId] == 0) {
        std::lock_guard guard(pinMutex);
        pinnedPages.erase(pageId);
    }
}

size_t BufferManager::getNumPages(const std::string &fileManagerId) const {
    return storageManager->getNumPages(fileManagerId);
}

void BufferManager::flushPage(PageID pageId) {
    auto frameId = pageToFrameMapping[pageId];
    auto &frame = bufferPool[frameId];
    storageManager->flushPage(pageId, *frame->page);
}

void BufferManager::evictPage() {
    auto pageToEvict = policy->selectPageToEvict(pinnedPages);
    if (pageToEvict == INVALID_PAGE_ID) {
        throw std::logic_error("Error: Cannot find page to evict");
    }

    std::unique_lock pageToFrameGuard(pageToFrameMappingMutex);

    if (!pageToFrameMapping.contains(pageToEvict)) {
        return;
    }

    auto frameId = pageToFrameMapping[pageToEvict];
    std::shared_lock bufferGuard(bufferPoolMutex);
    auto &frame = bufferPool.at(frameId);
    if (frame->isDirty) {
        flushPage(pageToEvict);
    }
    pageToFrameMapping.erase(pageToEvict);
    frame->reset();
    std::unique_lock availableFramesGuard(availableFramesMutex);
    availableFrames.insert(frameId);
}

void BufferManager::registerFileManager(const std::string &fileManagerId, const std::string &filePath) const {
    storageManager->registerFileManager(fileManagerId, filePath);
}

bool BufferManager::isPinned(PageID pageId) const {
    std::shared_lock pinnedPagesGuard(pinMutex);
    return pinnedPages.contains(pageId);
}
