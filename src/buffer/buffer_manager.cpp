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

std::unique_ptr<BufferFrame> &BufferManager::pinPage(const PageID &pageId, const LockMode lockMode) noexcept(false) {
    std::unique_lock metadataLock(metadataMutex);
    if (pageToFrameMapping.contains(pageId)) {
        policy->accessPage(pageId);
        auto frameId = pageToFrameMapping[pageId];
        auto &frame = bufferPool.at(frameId);
        pinnedPages.insert(pageId);
        ++pinCounters[frameId];
        metadataLock.unlock();

        try {
            if (lockMode == EXCLUSIVE) {
                frameLockTable->lockExclusive(frameId);
                frame->exclusive.store(true);
            } else {
                frameLockTable->lockShare(frameId);
                frame->exclusive.store(false);
            }
            return frame;
        } catch (std::exception &e) {
            // rollback
            std::unique_lock rollbackLock(metadataMutex);
            if (--pinCounters[frameId] == 0) {
                pinnedPages.erase(pageId);
            }
            throw;
        }
    }

    if (policy->isCacheFull()) {
        evictPage();
    }

    // find an available frame
    if (availableFrames.empty()) {
        throw std::logic_error("Error: No frame available. This is likely a implementation logic error!");
    }
    FrameID frameId = *availableFrames.begin();

    if (pageId.fileManagerPageId >= getNumPages(pageId.fileManagerId)) {
        storageManager->extend(pageId.fileManagerId, pageId.fileManagerPageId);
    }

    std::unique_ptr<Page> page = storageManager->getPage(pageId);

    policy->accessPage(pageId);

    if (lockMode == EXCLUSIVE) {
        frameLockTable->lockExclusive(frameId);
        bufferPool[frameId]->exclusive = true;
    } else {
        frameLockTable->lockShare(frameId);
        bufferPool[frameId]->exclusive = false;
    }

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

void BufferManager::unpinPage(const PageID &pageId) {
    std::unique_lock guard(metadataMutex);
    if (!pinnedPages.contains(pageId)) {
        throw std::logic_error("Error: page is not pinned");
    }

    const auto frameId = pageToFrameMapping[pageId];
    auto &frame = bufferPool.at(frameId);

    --pinCounters[frameId];

    if (pinCounters[frameId] == 0) {
        pinnedPages.erase(pageId);
    }

    guard.unlock();

    try {
        if (frame->exclusive) {
            frameLockTable->unlockExclusive(frameId);
        } else {
            frameLockTable->unlockShare(frameId);
        }
    } catch (std::exception&) {
        // rollback
        std::unique_lock rollbackGuard(metadataMutex);
        ++pinCounters[frameId];
        pinnedPages.insert(pageId);
    }
}

size_t BufferManager::getNumPages(const std::string &fileManagerId) const {
    return storageManager->getNumPages(fileManagerId);
}

void BufferManager::flushPage(const PageID &pageId) {
    auto frameId = pageToFrameMapping[pageId];
    auto &frame = bufferPool[frameId];
    storageManager->flushPage(pageId, *frame->page);
}

void BufferManager::evictPage() {
    auto pageToEvict = policy->selectPageToEvict(pinnedPages);
    if (pageToEvict == INVALID_PAGE_ID) {
        throw std::logic_error("Error: Cannot find page to evict");
    }

    if (!pageToFrameMapping.contains(pageToEvict)) {
        return;
    }

    const auto frameId = pageToFrameMapping[pageToEvict];
    const auto &frame = bufferPool.at(frameId);
    if (frame->isDirty) {
        flushPage(pageToEvict);
    }
    pageToFrameMapping.erase(pageToEvict);
    frame->reset();
    availableFrames.insert(frameId);
}

void BufferManager::registerFileManager(const std::string &fileManagerId, const std::string &filePath) const {
    storageManager->registerFileManager(fileManagerId, filePath);
}

bool BufferManager::isPinned(const PageID &pageId) const {
    std::shared_lock guard(metadataMutex);
    return pinnedPages.contains(pageId);
}
