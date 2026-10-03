//
// Created by Hieu Pham on 10/3/26.
//

#include "buffer/page_guard.h"

#include <gtest/gtest.h>
#include <utility>

#include "buffer/buffer_manager.h"
#include "../test_utils.h"

TEST(PageGuardTest, FieldConstructor) {
    BufferManager bufferManager;
    BufferFrame bufferFrame;

    PageGuard page{&bufferManager, &bufferFrame};

    EXPECT_EQ(page.bufferManager, &bufferManager);
    EXPECT_EQ(page.bufferFrame, &bufferFrame);
    EXPECT_FALSE(page.isDirty);

    // prevent destructor from attempting to unpin page
    page.bufferManager = nullptr;
}

TEST(PageGuardTest, MoveConstructor) {
    BufferManager bufferManager;
    BufferFrame bufferFrame;

    PageGuard page{&bufferManager, &bufferFrame};
    page.markDirty();

    PageGuard pageWithMoveConstructor{std::move(page)};

    EXPECT_EQ(pageWithMoveConstructor.bufferManager, &bufferManager);
    EXPECT_EQ(pageWithMoveConstructor.bufferFrame, &bufferFrame);
    EXPECT_TRUE(pageWithMoveConstructor.isDirty);

    EXPECT_EQ(page.bufferManager, nullptr);
    EXPECT_FALSE(page);
    EXPECT_FALSE(page.isDirty);

    // prevent destructor from attempting to unpin page
    pageWithMoveConstructor.bufferManager = nullptr;
}

TEST(PageGuardTest, EqualOperator) {
    BufferManager bufferManager;
    BufferFrame bufferFrame;

    PageGuard page{&bufferManager, &bufferFrame};
    page.markDirty();

    PageGuard newPage{nullptr, nullptr};
    PageGuard& result = (newPage = std::move(page));
    EXPECT_EQ(&result, &newPage);

    EXPECT_EQ(newPage.bufferManager, &bufferManager);
    EXPECT_EQ(newPage.bufferFrame, &bufferFrame);
    EXPECT_TRUE(newPage.isDirty);

    EXPECT_EQ(page.bufferManager, nullptr);
    EXPECT_FALSE(page);
    EXPECT_FALSE(page.isDirty);

    // prevent destructor from attempting to unpin page
    newPage.bufferManager = nullptr;
}

TEST(PageGuardTest, ArrowOperator) {
    BufferFrame bufferFrame;
    bufferFrame.page = std::make_unique<Page>();
    PageGuard page{nullptr, &bufferFrame};

    ASSERT_EQ(page.operator->(), bufferFrame.page.get());
    ASSERT_EQ(page->pageData.get(), bufferFrame.page->pageData.get());
}

TEST(PageGuardTest, ConstArrowOperator) {
    BufferFrame bufferFrame;
    bufferFrame.page = std::make_unique<Page>();
    const PageGuard page{nullptr, &bufferFrame};

    ASSERT_EQ(page.operator->(), bufferFrame.page.get());
    ASSERT_EQ(page->pageData.get(), bufferFrame.page->pageData.get());
}

TEST(PageGuardTest, DereferenceOperator) {
    BufferFrame bufferFrame;
    bufferFrame.page = std::make_unique<Page>();
    PageGuard page{nullptr, &bufferFrame};

    ASSERT_EQ(&*page, bufferFrame.page.get());
}

TEST(PageGuardTest, ConstDereferenceOperator) {
    BufferFrame bufferFrame;
    bufferFrame.page = std::make_unique<Page>();
    const PageGuard page{nullptr, &bufferFrame};

    ASSERT_EQ(&*page, bufferFrame.page.get());
}

TEST(PageGuardTest, GetFrame) {
    BufferFrame bufferFrame;
    const PageGuard page{nullptr, &bufferFrame};

    ASSERT_EQ(page.getFrame(), &bufferFrame);
}

TEST(PageGuardTest, BoolOperator) {
    BufferFrame bufferFrame;
    const PageGuard page{nullptr, &bufferFrame};
    const PageGuard emptyPage{nullptr, nullptr};

    ASSERT_TRUE(page);
    ASSERT_FALSE(emptyPage);
}

TEST(PageGuardTest, EmptyGuard) {
    const PageGuard page{nullptr, nullptr};

    ASSERT_EQ(page.operator->(), nullptr);
    ASSERT_EQ(page.getFrame(), nullptr);
    ASSERT_EQ(page.getPageId(), INVALID_PAGE_ID);
    ASSERT_EQ(page.getFrameId(), INVALID_FRAME_ID);
}

TEST(PageGuardTest, MovedFromGuard) {
    BufferFrame bufferFrame;
    PageGuard page{nullptr, &bufferFrame};
    PageGuard movedPage{std::move(page)};

    ASSERT_FALSE(page);
    ASSERT_EQ(page.operator->(), nullptr);
    ASSERT_EQ(page.getFrame(), nullptr);
    ASSERT_EQ(page.getPageId(), INVALID_PAGE_ID);
    ASSERT_EQ(page.getFrameId(), INVALID_FRAME_ID);
    ASSERT_EQ(movedPage.getFrame(), &bufferFrame);
}

TEST(PageGuardTest, SelfMoveAssignment) {
    BufferFrame bufferFrame;
    PageGuard page{nullptr, &bufferFrame};
    auto& alias = page;

    PageGuard& result = (page = std::move(alias));

    ASSERT_EQ(&result, &page);
    ASSERT_EQ(page.getFrame(), &bufferFrame);
}

TEST(PageGuardTest, MoveEmptyGuard) {
    PageGuard page{nullptr, nullptr};
    PageGuard movedPage{std::move(page)};
    PageGuard assignedPage{nullptr, nullptr};
    assignedPage = std::move(movedPage);

    ASSERT_FALSE(page);
    ASSERT_FALSE(movedPage);
    ASSERT_FALSE(assignedPage);
}

TEST(PageGuardTest, DestructorUnpinsAndMarksFrameDirty) {
    auto filePath = generateRandomFilePath();
    {
        BufferManager bufferManager;
        bufferManager.registerFileManager("fm", filePath);
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=0};
        FrameID frameId;
        {
            auto page = bufferManager.pinPage(pageId, EXCLUSIVE);
            EXPECT_EQ(page.getPageId(), pageId);
            frameId = page.getFrameId();
            EXPECT_NE(frameId, INVALID_FRAME_ID);
            EXPECT_TRUE(bufferManager.isPinned(pageId));
            EXPECT_FALSE(page.getFrame()->isPageDirty());

            page.markDirty();
            // Dirty state is propagated when the guard releases its pin.
            EXPECT_FALSE(page.getFrame()->isPageDirty());
        }
        EXPECT_FALSE(bufferManager.isPinned(pageId));
        {
            auto page = bufferManager.pinPage(pageId, SHARED);
            EXPECT_EQ(page.getPageId(), pageId);
            EXPECT_EQ(page.getFrameId(), frameId);
            EXPECT_TRUE(page.getFrame()->isPageDirty());
        }
        EXPECT_FALSE(bufferManager.isPinned(pageId));
    }
    std::filesystem::remove(filePath);
}
