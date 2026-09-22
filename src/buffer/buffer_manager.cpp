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

std::unique_ptr<Page> &BufferManager::pinPage(const PageID &pageId, LockMode lockMode) noexcept(false) {
    pinMutex.lock();
    bufferPoolMutex.lock();
    if (pinnedPages.contains(pageId)) {
        policy->accessPage(pageId);
        auto frameId = pageToFrameMapping[pageId];
        auto &frame = bufferPool.at(frameId);
        pinMutex.unlock();
        ++pinCounters[frameId];
        return frame->page;
    }
    pinMutex.unlock();
    bufferPoolMutex.unlock();

    if (pageId.fileManagerPageId >= getNumPages(pageId.fileManagerId)) {
        storageManager->extend(pageId.fileManagerId, pageId.fileManagerPageId);
    }

    FrameID frameId;

    if (pageToFrameMapping.contains(pageId)) {
        frameId = pageToFrameMapping[pageId];
    } else {
        if (policy->isCacheFull()) {
            evictPage();
        }
        // find an available frame
        if (availableFrames.empty()) {
            throw std::logic_error("Error: No frame available. This is likely a implementation logic error!");
        }
        frameId = *availableFrames.begin();
    }

    std::unique_ptr<Page> page = storageManager->getPage(pageId);

    policy->accessPage(pageId);

    std::lock_guard buffeGuard(bufferPoolMutex);
    bufferPool[frameId]->isDirty = false;
    bufferPool[frameId]->frameId = frameId;
    bufferPool[frameId]->pageId = pageId;
    bufferPool[frameId]->page = std::move(page);
    pageToFrameMapping[pageId] = frameId;
    availableFrames.erase(frameId);

    if (pinCounters[frameId] == 0) {
        std::lock_guard guard(pinMutex);
        pinnedPages.insert(pageId);
    }

    ++pinCounters[frameId];

    return bufferPool[frameId]->page;
}

void BufferManager::unpinPage(PageID pageId) {
    pinMutex.lock();
    if (!pinnedPages.contains(pageId)) {
        throw std::logic_error("Error: page is not pinned");
    }
    pinMutex.unlock();

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
    auto frameId = pageToFrameMapping[pageToEvict];
    auto &frame = bufferPool.at(frameId);
    if (frame->isDirty) {
        flushPage(pageToEvict);
    }
    frame->reset();
    availableFrames.insert(frameId);
    pageToFrameMapping.erase(pageToEvict);
}

void BufferManager::registerFileManager(const std::string &fileManagerId, const std::string &filePath) const {
    storageManager->registerFileManager(fileManagerId, filePath);
}

bool BufferManager::isPinned(PageID pageId) const {
    return pinnedPages.contains(pageId);
}
