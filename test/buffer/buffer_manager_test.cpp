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

    auto &page = bm.pinPage(PageID{.fileManagerId="fm1", .fileManagerPageId=0}, SHARED);
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
    auto &page = bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, SHARED);
    ASSERT_NE(page, nullptr);

    Slot* slot = reinterpret_cast<Slot*>(page->pageData.get());

    ASSERT_EQ(slot->empty, false);
    ASSERT_EQ(slot->offset, 123);
    ASSERT_EQ(slot->size, 123456);
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

TEST_F(BufferManagerTest, FlushPage) {
    BufferManager bm;
    auto randomFilePath = generateRandomFilePath();

    bm.registerFileManager("fm", randomFilePath);
    PageID pageId = PageID{.fileManagerId="fm", .fileManagerPageId=0};

    auto &page = bm.pinPage(pageId, SHARED);

    Slot* slots = reinterpret_cast<Slot*>(page->pageData.get());

    ASSERT_EQ(slots[2].empty, true);
    ASSERT_EQ(slots[2].offset, INVALID_VALUE);
    ASSERT_EQ(slots[2].size, INVALID_VALUE);

    slots[2].empty = false;
    slots[2].offset = 123;
    slots[2].size = 123456;

    bm.flushPage(pageId);

    ASSERT_EQ(slots[2].empty, false);
    ASSERT_EQ(slots[2].offset, 123);
    ASSERT_EQ(slots[2].size, 123456);
}
