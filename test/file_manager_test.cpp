//
// Created by Hieu Pham on 9/12/26.
//

#include "file_manager.h"

#include <gtest/gtest.h>
#include <filesystem>
#include <random>

class FileManagerTest : public ::testing::Test {
protected:

    void SetUp() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution distr(1, 10000);
        int randomNum = distr(gen);

        filePath = std::filesystem::temp_directory_path() / ("tomdb_test" + std::to_string(randomNum) + ".data");
        std::ofstream create(filePath);
    }

    void TearDown() override {
        std::filesystem::remove(filePath);
    }

    std::filesystem::path filePath;
};

TEST_F(FileManagerTest, InitializationWhenDataFileNotExists) {
    std::filesystem::path filePath = "test-123456.data";
    FileManager fileManager(filePath);

    ASSERT_EQ(fileManager.getNumPages(), 1);
}

TEST_F(FileManagerTest, InitializationWhenFileExistsWithNoData) {
    FileManager fileManager(filePath);

    ASSERT_EQ(fileManager.getNumPages(), 1);
}

TEST_F(FileManagerTest, InitializationWithExistingData) {
    std::fstream filestream;
    filestream.open(filePath, std::fstream::in | std::fstream::out);
    filestream.seekp(0, std::fstream::beg);

    for (size_t i = 0; i < 10; i++) {
        Page page;
        filestream.write(page.pageData.get(), PAGE_SIZE);
    }

    filestream.flush();
    FileManager fileManager(filePath);

    ASSERT_EQ(fileManager.getNumPages(), 10);
}

TEST_F(FileManagerTest, LoadExistingPage) {
    std::fstream filestream;
    filestream.open(filePath, std::fstream::in | std::fstream::out);
    filestream.seekp(0, std::fstream::beg);

    Page newPage;
    Slot* newSlot = reinterpret_cast<Slot*>(newPage.pageData.get());
    newSlot->empty = false;
    newSlot->offset = 123;
    newSlot->size = 123456;
    filestream.write(newPage.pageData.get(), PAGE_SIZE);
    filestream.flush();

    FileManager fileManager(filePath);
    auto page = fileManager.load(0);

    Slot* slot = reinterpret_cast<Slot*>(page->pageData.get());
    ASSERT_EQ(slot->empty, false);
    ASSERT_EQ(slot->offset, 123);
    ASSERT_EQ(slot->size, 123456);
}

TEST_F(FileManagerTest, LoadPageOutOfBound) {
    FileManager fileManager(filePath);
    ASSERT_THROW(fileManager.load(2), std::out_of_range);
}

TEST_F(FileManagerTest, FlushPage) {
    std::fstream filestream;
    filestream.open(filePath, std::fstream::in | std::fstream::out);
    filestream.seekp(0, std::fstream::beg);
    Page newPage;

    Slot* newSlots = reinterpret_cast<Slot*>(newPage.pageData.get());
    newSlots[0].empty = false;
    newSlots[0].offset = 123;
    newSlots[0].size = 123456;

    filestream.write(newPage.pageData.get(), PAGE_SIZE);
    filestream.flush();
    FileManager fileManager(filePath);

    auto page = fileManager.load(0);
    Slot* slots = reinterpret_cast<Slot*>(page->pageData.get());
    ASSERT_EQ(slots[0].empty, false);

    slots->empty = true;
    ASSERT_EQ(fileManager.flush(0, *page), true);

    page = fileManager.load(0);
    slots = reinterpret_cast<Slot*>(page->pageData.get());
    ASSERT_EQ(slots[0].empty, true);
}

TEST_F(FileManagerTest, FlushPageOutOfBound) {
    Page page;
    FileManager fileManager(filePath);
    ASSERT_EQ(fileManager.flush(2, page), false);
}

TEST_F(FileManagerTest, ExtendPage) {
    FileManager fileManager(filePath);
    ASSERT_THROW(fileManager.load(1), std::out_of_range);
    fileManager.extend();

    auto page = fileManager.load(1);
    Slot *slots = reinterpret_cast<Slot*>(page->pageData.get());
    ASSERT_EQ(slots[0].empty, true);
    ASSERT_EQ(slots[0].offset, INVALID_VALUE);
    ASSERT_EQ(slots[0].size, INVALID_VALUE);
}

TEST_F(FileManagerTest, ExtendTilPage) {
    FileManager fileManager(filePath);
    ASSERT_THROW(fileManager.load(10), std::out_of_range);
    fileManager.extend(10);

    auto page = fileManager.load(10);
    Slot *slots = reinterpret_cast<Slot*>(page->pageData.get());
    ASSERT_EQ(slots[0].empty, true);
    ASSERT_EQ(slots[0].offset, INVALID_VALUE);
    ASSERT_EQ(slots[0].size, INVALID_VALUE);
}

TEST_F(FileManagerTest, ExtendTilPageSmallerThanNumPages) {
    FileManager fileManager(filePath);
    fileManager.extend(10);

    ASSERT_EQ(fileManager.getNumPages(), 11);
    fileManager.extend(9);
    ASSERT_EQ(fileManager.getNumPages(), 11);
}
