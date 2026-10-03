//
// Created by Hieu Pham on 10/3/26.
//

#include "buffer/page_guard.h"

#include <iostream>

#include "buffer/buffer_manager.h"

PageGuard::PageGuard(PageGuard &&pageGuard) noexcept {
    this->bufferFrame = pageGuard.bufferFrame;
    this->bufferManager = pageGuard.bufferManager;
    this->isDirty = pageGuard.isDirty;
    pageGuard.bufferFrame = nullptr;
    pageGuard.bufferManager = nullptr;
    pageGuard.isDirty = false;
}

PageGuard::PageGuard(BufferManager *bufferManager, BufferFrame *bufferFrame) {
    this->bufferManager = bufferManager;
    this->bufferFrame = bufferFrame;
}

PageGuard::~PageGuard() {
    if (bufferManager != nullptr && bufferFrame != nullptr) {
        // lazily mark dirty
        if (isDirty) {
            bufferFrame->markDirty();
        }
        try {
            this->bufferManager->unpinPage(bufferFrame->pageId);
        } catch (std::logic_error &e) {
            // TODO: replace with proper logging in next phase
            std::cerr << e.what() << '\n';
        }
    }
}

PageGuard &PageGuard::operator=(PageGuard&& pageGuard) noexcept {
    if (this != &pageGuard) {
        if (this->bufferManager != nullptr && this->bufferFrame != nullptr) {
            if (this->isDirty) {
                this->bufferFrame->markDirty();
            }
            try {
                this->bufferManager->unpinPage(this->getPageId());
            } catch (std::logic_error &e) {
                // TODO: replace with proper logging in next phase
                std::cerr << e.what() << '\n';
            }
        }

        this->bufferFrame = pageGuard.bufferFrame;
        this->bufferManager = pageGuard.bufferManager;
        this->isDirty = pageGuard.isDirty;

        pageGuard.bufferFrame = nullptr;
        pageGuard.bufferManager = nullptr;
        pageGuard.isDirty = false;
    }
    return *this;
}
