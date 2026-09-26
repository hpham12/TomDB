//
// Created by Hieu Pham on 9/19/26.
//
#include "buffer/buffer_manager.h"
#include "../test_utils.h"

#include <random>
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

    auto &page = bm.pinPage(PageID{.fileManagerId="fm1", .fileManagerPageId=0}, SHARED)->page;
    ASSERT_NE(page, nullptr);

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
    for (size_t i = 0; i < MAX_CACHED_PAGES; i++) {
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(i)};
        bm.pinPage(pageId, SHARED);
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

    bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, SHARED);
    auto &page = bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, SHARED)->page;
    ASSERT_NE(page, nullptr);

    Slot* slot = reinterpret_cast<Slot*>(page->pageData.get());

    ASSERT_EQ(slot->empty, false);
    ASSERT_EQ(slot->offset, 123);
    ASSERT_EQ(slot->size, 123456);
}

TEST_F(BufferManagerTest, MultiplePinsAndUnpins) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);
    auto pageId = PageID{.fileManagerId="fm", .fileManagerPageId=0};
    bm.pinPage(pageId, SHARED);
    bm.pinPage(pageId, SHARED);

    ASSERT_TRUE(bm.isPinned(pageId));

    bm.unpinPage(pageId);
    ASSERT_TRUE(bm.isPinned(pageId));

    bm.unpinPage(pageId);
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
    bm.pinPage(pageId, SHARED);

    ASSERT_TRUE(bm.isPinned(pageId));

    bm.unpinPage(pageId);
    ASSERT_FALSE(bm.isPinned(pageId));
}

TEST_F(BufferManagerTest, Unpin) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);
    auto pageId = PageID{.fileManagerId="fm", .fileManagerPageId=0};
    bm.pinPage(pageId, SHARED);

    ASSERT_TRUE(bm.isPinned(pageId));

    bm.unpinPage(pageId);
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
    for (size_t i = 0; i < MAX_CACHED_PAGES; i++) {
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(i)};
        bm.pinPage(pageId, SHARED);
    }

    PageID firstPageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(0)};

    bm.unpinPage(firstPageId);

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
    for (size_t i = 0; i < MAX_CACHED_PAGES; i++) {
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(i)};
        bm.pinPage(pageId, SHARED);
    }

    ASSERT_THROW(bm.evictPage(), std::logic_error);
}

TEST_F(BufferManagerTest, EvictPageNoopWhenMappingDoesNotContainPage) {
    BufferManager bm;

    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);

    PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(0)};
    bm.pinPage(pageId, SHARED);

    bm.unpinPage(pageId);
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
    unique_ptr<BufferFrame> &frame = bm.pinPage(pageId, SHARED);
    auto &page = frame->page;

    Slot* slots = reinterpret_cast<Slot*>(page->pageData.get());

    ASSERT_EQ(slots[2].empty, true);
    ASSERT_EQ(slots[2].offset, INVALID_VALUE);
    ASSERT_EQ(slots[2].size, INVALID_VALUE);

    slots[2].empty = false;
    slots[2].offset = 123;
    slots[2].size = 123456;

    frame->markDirty();
    bm.unpinPage(pageId);
    bm.evictPage();

    unique_ptr<BufferFrame> &updatedFrame = bm.pinPage(pageId, SHARED);
    auto &updatedPage = updatedFrame->page;

    slots = reinterpret_cast<Slot*>(updatedPage->pageData.get());

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

    auto &page = bm.pinPage(pageId, SHARED)->page;

    Slot* slots = reinterpret_cast<Slot*>(page->pageData.get());

    ASSERT_EQ(slots[2].empty, true);
    ASSERT_EQ(slots[2].offset, INVALID_VALUE);
    ASSERT_EQ(slots[2].size, INVALID_VALUE);

    slots[2].empty = false;
    slots[2].offset = 123;
    slots[2].size = 123456;

    bm.flushPage(pageId);

    bm.unpinPage(pageId);
    FileManager reader(randomFilePath);
    auto updatedPage = reader.load(0);
    slots = reinterpret_cast<Slot*>(updatedPage->pageData.get());

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

    auto &frame = bm.pinPage(pageId, EXCLUSIVE);

    auto tuple = std::make_unique<Tuple>();
    tuple->addField(std::make_unique<Field>(std::string("unflushed")));
    ASSERT_EQ(frame->page->addTuple(std::move(tuple), nullptr), 0);

    frame->markDirty();

    const std::string expected(frame->page->pageData.get(), PAGE_SIZE);

    bm.unpinPage(pageId);
    auto &updatedFrame = bm.pinPage(pageId, SHARED);

    ASSERT_TRUE(bm.isPinned(pageId));
    ASSERT_TRUE(updatedFrame->isPageDirty());
    ASSERT_EQ(std::string(updatedFrame->page->pageData.get(), PAGE_SIZE), expected);

    bm.unpinPage(pageId);
    ASSERT_FALSE(bm.isPinned(pageId));
}

TEST_F(BufferManagerTest, AutomaticEvictionPersistsDirtyVictimAndReusesFrame) {
    BufferManager bm;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    bm.registerFileManager("fm", randomFilePath);
    PageID firstPageId{.fileManagerId="fm", .fileManagerPageId=0};

    auto &frame = bm.pinPage(firstPageId, EXCLUSIVE);

    auto tuple = std::make_unique<Tuple>();
    tuple->addField(std::make_unique<Field>(123));
    ASSERT_EQ(frame->page->addTuple(std::move(tuple), nullptr), 0);

    frame->markDirty();

    const std::string expected(frame->page->pageData.get(), PAGE_SIZE);

    // load remaining pages so the cache is full
    for (size_t i = 1; i < MAX_CACHED_PAGES; i++) {
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(i)};
        bm.pinPage(pageId, SHARED);
    }

    bm.unpinPage(firstPageId);

    PageID nextPageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(MAX_CACHED_PAGES)};
    auto &replacement = bm.pinPage(nextPageId, SHARED);
    ASSERT_FALSE(replacement->isPageDirty());

    FileManager fileManager(randomFilePath);
    auto persistedPage = fileManager.load(0);
    ASSERT_EQ(std::string(persistedPage->pageData.get(), PAGE_SIZE), expected);

    bm.unpinPage(nextPageId);
    auto &updatedFrame = bm.pinPage(firstPageId, SHARED);
    ASSERT_EQ(std::string(updatedFrame->page->pageData.get(), PAGE_SIZE), expected);
}
