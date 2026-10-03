//
// Created by Hieu Pham on 10/3/26.
//

#ifndef TOMDB_PAGE_GUARD_H
#define TOMDB_PAGE_GUARD_H
#include "buffer_frame.h"
#include "gtest/gtest_prod.h"

class BufferManager;

/**
 * PageGuard is a RAII wrapper around BufferFrame
 */
class PageGuard {
    BufferManager *bufferManager = nullptr;
    BufferFrame *bufferFrame = nullptr;
    bool isDirty = false;

    FRIEND_TEST(PageGuardTest, MoveConstructor);
    FRIEND_TEST(PageGuardTest, FieldConstructor);
    FRIEND_TEST(PageGuardTest, EqualOperator);

public:
    PageGuard(BufferManager *bufferManager, BufferFrame *bufferFrame);
    ~PageGuard();

    PageGuard& operator=(const PageGuard&) = delete;
    PageGuard(const PageGuard&) = delete;
    PageGuard(PageGuard&&)  noexcept ;
    PageGuard& operator=(PageGuard&&) noexcept;

    Page* operator->() {
        return bufferFrame != nullptr ? bufferFrame->page.get() : nullptr;
    }

    Page& operator*() {
        return *bufferFrame->page;
    }
    const Page* operator->() const {
        return bufferFrame != nullptr ? bufferFrame->page.get() : nullptr;
    }

    const Page& operator*() const {
        return *bufferFrame->page;
    }

    void markDirty() {
        isDirty = true;
    }

    [[nodiscard]] PageID getPageId() const {
        return bufferFrame != nullptr ? bufferFrame->pageId : INVALID_PAGE_ID;
    }

    [[nodiscard]] FrameID getFrameId() const {
        return bufferFrame != nullptr ? bufferFrame->frameId : INVALID_FRAME_ID;
    }

    [[nodiscard]] BufferFrame* getFrame() const { return bufferFrame; }

    explicit operator bool() const { return bufferFrame != nullptr; }
};

#endif //TOMDB_PAGE_GUARD_H
