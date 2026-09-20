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

TEST_F(BufferManagerTest, PinPageWithCacheEviction) {
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
    for (size_t i = 0; i < MAX_CACHED_PAGES + 10; i++) {
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(i)};
        bm.pinPage(pageId, SHARED);
    }

    auto &page = bm.pinPage(PageID{.fileManagerId="fm", .fileManagerPageId=0}, SHARED);
    ASSERT_NE(page, nullptr);

    Slot* slot = reinterpret_cast<Slot*>(page->pageData.get());

    ASSERT_EQ(slot->empty, false);
    ASSERT_EQ(slot->offset, 123);
    ASSERT_EQ(slot->size, 123456);
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

