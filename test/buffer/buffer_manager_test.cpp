//
// Created by Hieu Pham on 9/19/26.
//
#include "buffer/buffer_manager.h"
#include "../test_utils.h"

#include <utility>
#include <vector>
#include <gtest/gtest.h>

class BufferManagerTest : public testing::Test {
protected:
    void TearDown() override {
        for (auto const &filePath : filePaths) {
            std::filesystem::remove(filePath);
        }
    }

    std::vector<std::string> filePaths;
};

TEST_F(BufferManagerTest, PinPage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    std::ofstream create(randomFilePath);

    // setup data
    std::fstream filestream;
    filestream.open(randomFilePath, std::fstream::in | std::fstream::out);
    filestream.seekp(0, std::fstream::beg);

    Page newPage;
    Slot* newSlot = reinterpret_cast<Slot*>(newPage.pageData.get());
    newSlot->empty = false;
    newSlot->offset = 123;
    newSlot->size = 123456;
    filestream.write(newPage.pageData.get(), PAGE_SIZE);
    filestream.flush();

    // buffer manager functionality test
    bm.registerFileManager("fm1", randomFilePath);

    auto page = bm.pinPage(PageID{.fileManagerId="fm1", .fileManagerPageId=0}, SHARED);
    ASSERT_TRUE(page);

    Slot* slot = reinterpret_cast<Slot*>(page->pageData.get());

    ASSERT_EQ(slot->empty, false);
    ASSERT_EQ(slot->offset, 123);
    ASSERT_EQ(slot->size, 123456);
}

TEST_F(BufferManagerTest, PinPageWithnoEvictablePage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    std::ofstream create(randomFilePath);

    // setup data
    std::fstream filestream;
    filestream.open(randomFilePath, std::fstream::in | std::fstream::out);
    filestream.seekp(0, std::fstream::beg);

    Page newPage;
    Slot* newSlot = reinterpret_cast<Slot*>(newPage.pageData.get());
    newSlot->empty = false;
    newSlot->offset = 123;
    newSlot->size = 123456;
    filestream.write(newPage.pageData.get(), PAGE_SIZE);
    filestream.flush();

    // buffer manager functionality test
    bm.registerFileManager("fm", randomFilePath);

    // load many pages so the cache is full
    std::vector<PageGuard> guards;
    for (size_t i = 0; i < MAX_CACHED_PAGES; i++) {
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(i)};
        guards.push_back(bm.pinPage(pageId, SHARED));
    }

    ASSERT_THROW(bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=10000}, SHARED), std::logic_error);
}

TEST_F(BufferManagerTest, PinExistingPinnedPage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    std::ofstream create(randomFilePath);

    // setup data
    std::fstream filestream;
    filestream.open(randomFilePath, std::fstream::in | std::fstream::out);
    filestream.seekp(0, std::fstream::beg);

    Page newPage;
    Slot* newSlot = reinterpret_cast<Slot*>(newPage.pageData.get());
    newSlot->empty = false;
    newSlot->offset = 123;
    newSlot->size = 123456;
    filestream.write(newPage.pageData.get(), PAGE_SIZE);
    filestream.flush();

    // buffer manager functionality test
    bm.registerFileManager("fm", randomFilePath);

    auto firstPin = bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, SHARED);
    auto page = bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, SHARED);
    ASSERT_TRUE(page);

    Slot* slot = reinterpret_cast<Slot*>(page->pageData.get());

    ASSERT_EQ(slot->empty, false);
    ASSERT_EQ(slot->offset, 123);
    ASSERT_EQ(slot->size, 123456);
}

TEST_F(BufferManagerTest, ExclusivePinOnAlreadyPinnedSharedPage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    auto sharedGuard = bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, SHARED);

    // page is current pinned in shared mode, so the exclusive request will timeout
    ASSERT_ANY_THROW(bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, EXCLUSIVE));
}

TEST_F(BufferManagerTest, SharePinOnAlreadyPinnedExclusivePage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    auto exclusiveGuard = bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, EXCLUSIVE);

    // page is current pinned in exclusive mode, so the shared request will timeout
    ASSERT_ANY_THROW(bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, SHARED));
}

TEST_F(BufferManagerTest, ExclusivePinOnAlreadyPinnedExclusivePage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    PageID pageId{.fileManagerId="fm", .fileManagerPageId=0};

    auto exclusiveGuard = bm.pinPage(pageId, EXCLUSIVE);

    // page is current pinned in exclusive mode, so the other exclusive request will timeout
    ASSERT_ANY_THROW(bm.pinPage(pageId, EXCLUSIVE));
}

TEST_F(BufferManagerTest, ExclusivePinCachedPageRollbackWhenFailedToGetFrameLock) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    PageID pageId{.fileManagerId="fm", .fileManagerPageId=0};

    bm.pageToFrameMapping[pageId] = 0;
    bm.frameLockTable->lockExclusive(0);

    ASSERT_ANY_THROW(bm.pinPage(pageId, EXCLUSIVE));

    ASSERT_TRUE(bm.pinnedPages.empty());
}

TEST_F(BufferManagerTest, SharedPinCachedPageRollbackWhenFailedToGetFrameLock) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    PageID pageId{.fileManagerId="fm", .fileManagerPageId=0};

    bm.pageToFrameMapping[pageId] = 0;
    bm.frameLockTable->lockExclusive(0);

    ASSERT_ANY_THROW(bm.pinPage(pageId, SHARED));

    ASSERT_TRUE(bm.pinnedPages.empty());
}

TEST_F(BufferManagerTest, ExclusivePinUncachedPageRollbackWhenFailedToGetFrameLock) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    PageID pageId{.fileManagerId="fm", .fileManagerPageId=0};

    // force available frame to contain only 0
    bm.availableFrames.clear();
    bm.availableFrames.insert(0);

    bm.frameLockTable->lockExclusive(0);

    ASSERT_ANY_THROW(bm.pinPage(pageId, EXCLUSIVE));

    ASSERT_TRUE(bm.pinnedPages.empty());
}

TEST_F(BufferManagerTest, SharedPinUncachedPageRollbackWhenFailedToGetFrameLock) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    PageID pageId{.fileManagerId="fm", .fileManagerPageId=0};

    // force available frame to contain only 0
    bm.availableFrames.clear();
    bm.availableFrames.insert(0);

    bm.frameLockTable->lockExclusive(0);

    ASSERT_ANY_THROW(bm.pinPage(pageId, SHARED));

    ASSERT_TRUE(bm.pinnedPages.empty());
}

TEST_F(BufferManagerTest, SharedUnpinPageRollbackWhenFailedToUnlockFrame) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    PageID pageId{.fileManagerId="fm", .fileManagerPageId=0};

    auto guard = bm.pinPage(pageId, SHARED);

    auto frameId = bm.pageToFrameMapping[pageId];
    auto &frame = bm.bufferPool.at(frameId);
    // flip the frame exclusivity
    frame->exclusive.store(true);

    EXPECT_ANY_THROW(bm.unpinPage(pageId));
    frame->exclusive.store(false);

    ASSERT_TRUE(bm.pinnedPages.contains(pageId));

    ASSERT_EQ(bm.pinCounters[frameId], 1);
}

TEST_F(BufferManagerTest, ExclusiveUnpinPageRollbackWhenFailedToUnlockFrame) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    PageID pageId{.fileManagerId="fm", .fileManagerPageId=0};

    auto guard = bm.pinPage(pageId, EXCLUSIVE);

    auto frameId = bm.pageToFrameMapping[pageId];
    auto &frame = bm.bufferPool.at(frameId);
    // flip the frame exclusivity
    frame->exclusive.store(false);

    EXPECT_ANY_THROW(bm.unpinPage(pageId));
    frame->exclusive.store(true);

    ASSERT_TRUE(bm.pinnedPages.contains(pageId));

    ASSERT_EQ(bm.pinCounters[frameId], 1);
}

TEST_F(BufferManagerTest, SharePinOnAlreadyPinnedSharedPage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    auto first = bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, SHARED);

    // Both shared guards can hold the same page concurrently.
    auto second = bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, SHARED);
    ASSERT_EQ(first.getFrame(), second.getFrame());
}

TEST_F(BufferManagerTest, MultiplePinsAndUnpins) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);
    auto pageId = PageID{.fileManagerId="fm", .fileManagerPageId=0};
    {
        auto first = bm.pinPage(pageId, SHARED);
        {
            auto second = bm.pinPage(pageId, SHARED);
            ASSERT_TRUE(bm.isPinned(pageId));
        }
        ASSERT_TRUE(bm.isPinned(pageId));
    }
    ASSERT_FALSE(bm.isPinned(pageId));
}

TEST_F(BufferManagerTest, PinPageFailsWhenAvailableFramesEmpty) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);
    auto pageId = PageID{.fileManagerId="fm", .fileManagerPageId=0};

    bm.availableFrames.clear();
    ASSERT_THROW(bm.pinPage(pageId, SHARED), std::logic_error);
}

TEST_F(BufferManagerTest, UnpinPage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);
    auto pageId = PageID{.fileManagerId="fm", .fileManagerPageId=0};
    {
        auto guard = bm.pinPage(pageId, SHARED);
        ASSERT_TRUE(bm.isPinned(pageId));
    }
    ASSERT_FALSE(bm.isPinned(pageId));
}

TEST_F(BufferManagerTest, MoveConstructionTransfersPin) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);
    auto pageId = PageID{.fileManagerId="fm", .fileManagerPageId=0};
    {
        auto guard = bm.pinPage(pageId, SHARED);
        {
            auto moved = std::move(guard);
            ASSERT_FALSE(guard);
            ASSERT_TRUE(moved);
            ASSERT_TRUE(bm.isPinned(pageId));
        }
        ASSERT_FALSE(bm.isPinned(pageId));
    }
    ASSERT_FALSE(bm.isPinned(pageId));
}

TEST_F(BufferManagerTest, UnpinPageThatIsNotPinned) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);
    auto pageId = PageID{.fileManagerId="fm", .fileManagerPageId=0};

    ASSERT_THROW(bm.unpinPage(pageId), std::logic_error);
}

TEST_F(BufferManagerTest, GetNumPages) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);
    ASSERT_EQ(bm.getNumPages("fm"), 1);

    auto pageId = PageID{.fileManagerId="fm", .fileManagerPageId=5};
    bm.pinPage(pageId, SHARED);

    ASSERT_EQ(bm.getNumPages("fm"), 6);
}

TEST_F(BufferManagerTest, GetNumPagesWithNonExisitentFm) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);
    ASSERT_THROW(bm.getNumPages("fm1"), FileManagerNotRegisteredException);
}

TEST_F(BufferManagerTest, EvictPage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    // load multiple pages
    std::vector<PageGuard> guards;
    for (size_t i = 0; i < MAX_CACHED_PAGES; i++) {
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(i)};
        guards.push_back(bm.pinPage(pageId, SHARED));
    }

    PageID firstPageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(0)};

    guards.erase(guards.begin());

    ASSERT_TRUE(bm.pageToFrameMapping.contains(firstPageId));
    ASSERT_TRUE(bm.availableFrames.empty());

    bm.evictPage();

    // the first page will be evicted
    ASSERT_FALSE(bm.pageToFrameMapping.contains(firstPageId));
    ASSERT_FALSE(bm.availableFrames.empty());

    auto availableFrameId = *bm.availableFrames.begin();
    auto &evictedFrame = bm.bufferPool[availableFrameId];
    ASSERT_EQ(evictedFrame->pageId, INVALID_PAGE_ID);
    ASSERT_EQ(evictedFrame->frameId, INVALID_FRAME_ID);
    ASSERT_EQ(evictedFrame->page, nullptr);
    ASSERT_FALSE(evictedFrame->isDirty);
}

TEST_F(BufferManagerTest, EvictPageFailsWithNoEvictablePage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    // load multiple pages
    std::vector<PageGuard> guards;
    for (size_t i = 0; i < MAX_CACHED_PAGES; i++) {
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(i)};
        guards.push_back(bm.pinPage(pageId, SHARED));
    }

    ASSERT_THROW(bm.evictPage(), std::logic_error);
}

TEST_F(BufferManagerTest, EvictPageNoopWhenMappingDoesNotContainPage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(0)};
    {
        auto guard = bm.pinPage(pageId, SHARED);
    }
    bm.pageToFrameMapping.erase(pageId);
    ASSERT_NO_THROW(bm.evictPage());
}

TEST_F(BufferManagerTest, EvictPageFlushesDirtyPage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    std::fstream filestream;
    filestream.open(randomFilePath, std::fstream::in | std::fstream::out);
    filestream.seekp(0, std::fstream::beg);

    Page newPage;
    Slot* newSlot = reinterpret_cast<Slot*>(newPage.pageData.get());
    newSlot->empty = false;
    newSlot->offset = 123;
    newSlot->size = 123456;
    filestream.write(newPage.pageData.get(), PAGE_SIZE);
    filestream.flush();

    bm.registerFileManager("fm", randomFilePath);
    PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(0)};
    {
        auto page = bm.pinPage(pageId, EXCLUSIVE);

        Slot* slots = reinterpret_cast<Slot*>(page->pageData.get());

        ASSERT_EQ(slots[2].empty, true);
        ASSERT_EQ(slots[2].offset, INVALID_VALUE);
        ASSERT_EQ(slots[2].size, INVALID_VALUE);

        slots[2].empty = false;
        slots[2].offset = 123;
        slots[2].size = 123456;

        page.markDirty();
    }
    bm.evictPage();

    auto updatedPage = bm.pinPage(pageId, SHARED);

    auto* slots = reinterpret_cast<Slot*>(updatedPage->pageData.get());

    ASSERT_EQ(slots[2].empty, false);
    ASSERT_EQ(slots[2].offset, 123);
    ASSERT_EQ(slots[2].size, 123456);
}

TEST_F(BufferManagerTest, FlushPage) {
    BufferManager bm;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);
    PageID pageId = PageID{.fileManagerId="fm", .fileManagerPageId=0};

    {
        auto page = bm.pinPage(pageId, EXCLUSIVE);

        Slot* slots = reinterpret_cast<Slot*>(page->pageData.get());

        ASSERT_EQ(slots[2].empty, true);
        ASSERT_EQ(slots[2].offset, INVALID_VALUE);
        ASSERT_EQ(slots[2].size, INVALID_VALUE);

        slots[2].empty = false;
        slots[2].offset = 123;
        slots[2].size = 123456;

        bm.flushPage(pageId);

    }
    FileManager reader(randomFilePath);
    auto updatedPage = reader.load(0);
    auto* slots = reinterpret_cast<Slot*>(updatedPage->pageData.get());

    ASSERT_EQ(slots[2].empty, false);
    ASSERT_EQ(slots[2].offset, 123);
    ASSERT_EQ(slots[2].size, 123456);
}

TEST_F(BufferManagerTest, RepinningDirtyCachedPagePreservesChanges) {
    BufferManager bm;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);
    bm.registerFileManager("fm", randomFilePath);
    PageID pageId{.fileManagerId="fm", .fileManagerPageId=0};

    std::string expected;
    {
        auto page = bm.pinPage(pageId, EXCLUSIVE);
        auto tuple = std::make_unique<Tuple>();
        tuple->addField(std::make_unique<Field>(std::string("unflushed")));
        ASSERT_EQ(page->addTuple(std::move(tuple), nullptr), 0);
        page.markDirty();
        expected.assign(page->pageData.get(), PAGE_SIZE);
    }
    {
        auto page = bm.pinPage(pageId, SHARED);
        ASSERT_TRUE(bm.isPinned(pageId));
        ASSERT_TRUE(page.getFrame()->isPageDirty());
        ASSERT_EQ(std::string(page->pageData.get(), PAGE_SIZE), expected);
    }
    ASSERT_FALSE(bm.isPinned(pageId));
}

TEST_F(BufferManagerTest, AutomaticEvictionPersistsDirtyVictimAndReusesFrame) {
    BufferManager bm;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);
    bm.registerFileManager("fm", randomFilePath);
    PageID firstPageId{.fileManagerId="fm", .fileManagerPageId=0};

    std::string expected;
    FrameID originalFrameId;
    {
        auto page = bm.pinPage(firstPageId, EXCLUSIVE);
        originalFrameId = page.getFrameId();
        auto tuple = std::make_unique<Tuple>();
        tuple->addField(std::make_unique<Field>(123));
        ASSERT_EQ(page->addTuple(std::move(tuple), nullptr), 0);
        page.markDirty();
        expected.assign(page->pageData.get(), PAGE_SIZE);
    }

    std::vector<PageGuard> guards;
    for (size_t i = 1; i < MAX_CACHED_PAGES; i++) {
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(i)};
        guards.push_back(bm.pinPage(pageId, SHARED));
    }

    PageID nextPageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(MAX_CACHED_PAGES)};
    {
        auto replacement = bm.pinPage(nextPageId, SHARED);
        ASSERT_EQ(replacement.getFrameId(), originalFrameId);
        ASSERT_FALSE(replacement.getFrame()->isPageDirty());
    }

    FileManager fileManager(randomFilePath);
    auto persistedPage = fileManager.load(0);
    ASSERT_EQ(std::string(persistedPage->pageData.get(), PAGE_SIZE), expected);

    auto updatedPage = bm.pinPage(firstPageId, SHARED);
    ASSERT_EQ(std::string(updatedPage->pageData.get(), PAGE_SIZE), expected);
}
