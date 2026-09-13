//
// Created by Hieu Pham on 9/12/26.
//

#include "storage_manager.h"

#include <random>

#include "gtest/gtest.h"

// TODO: add file clean up after each test

std::string generateRandomFilePath() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution distr(1, 10000);
    int randomNum = distr(gen);

    return std::filesystem::temp_directory_path() / ("tomdb_test" + std::to_string(randomNum) + ".data");
}

TEST(StorageManagerTest, RegisterFileManager) {
    StorageManager storageManager;

    ASSERT_TRUE(storageManager.registerFileManager("fm1", generateRandomFilePath()));
    ASSERT_TRUE(storageManager.registerFileManager("fm2", generateRandomFilePath()));
    ASSERT_TRUE(storageManager.registerFileManager("fm3", generateRandomFilePath()));

    ASSERT_EQ(storageManager.getNumPages("fm1"), 1);
    ASSERT_EQ(storageManager.getNumPages("fm2"), 1);
    ASSERT_EQ(storageManager.getNumPages("fm3"), 1);

    ASSERT_FALSE(storageManager.registerFileManager("fm1", generateRandomFilePath()));
}

TEST(StorageManagerTest, GetPage) {
    StorageManager storageManager;
    ASSERT_TRUE(storageManager.registerFileManager("fm1", generateRandomFilePath()));

    ASSERT_NE(storageManager.getPage(PageID{.fileManagerId="fm1", .fileManagerPageId=0}), nullptr);
}

TEST(StorageManagerTest, GetPageWithFileManagerIdNotFound) {
    StorageManager storageManager;
    ASSERT_TRUE(storageManager.registerFileManager("fm2", generateRandomFilePath()));

    ASSERT_THROW(storageManager.getPage(PageID{.fileManagerId="fm1", .fileManagerPageId=0}), std::logic_error);
}

TEST(StorageManagerTest, FlushPage) {
    StorageManager storageManager;
    auto filePath = generateRandomFilePath();
    ASSERT_TRUE(storageManager.registerFileManager("fm1", filePath));

    std::fstream filestream(filePath, std::ios::out | std::ios::in);

    Page page;
    Slot *slots = reinterpret_cast<Slot*>(page.pageData.get());
    slots[0].empty = false;
    slots[0].offset = 123;
    slots[0].size = 123456;

    filestream.seekp(0, std::ios::end);
    filestream.write(page.pageData.get(), PAGE_SIZE);

    ASSERT_TRUE(storageManager.flushPage(PageID{.fileManagerId="fm1", .fileManagerPageId=0}, page));

    auto &fileManager = storageManager.getFileManager("fm1");
    auto updatedPage = fileManager->load(0);
    auto *updatedSlots = reinterpret_cast<Slot*>(page.pageData.get());
    updatedSlots[0].empty = false;
    updatedSlots[0].offset = 123;
    updatedSlots[0].size = 123456;
}

TEST(StorageManagerTest, FlushPageWhenFileManagerIdNotFound) {
    StorageManager storageManager;
    ASSERT_TRUE(storageManager.registerFileManager("fm2", generateRandomFilePath()));

    Page page;
    ASSERT_THROW(storageManager.flushPage(PageID{.fileManagerId="fm1", .fileManagerPageId=0}, page), std::logic_error);
}

TEST(StorageManagerTest, ExtendPage) {
    StorageManager storageManager;
    ASSERT_TRUE(storageManager.registerFileManager("fm1", generateRandomFilePath()));

    storageManager.extend("fm1");
    ASSERT_EQ(storageManager.getFileManager("fm1")->getNumPages(), 2);
}

TEST(StorageManagerTest, ExtendPageWhenFileManagerIdNotFound) {
    StorageManager storageManager;
    ASSERT_TRUE(storageManager.registerFileManager("fm2", generateRandomFilePath()));

    ASSERT_THROW(storageManager.extend("fm1"), std::logic_error);
    ASSERT_EQ(storageManager.getFileManager("fm2")->getNumPages(), 1);
}

TEST(StorageManagerTest, ExtendPageTilMax) {
    StorageManager storageManager;
    ASSERT_TRUE(storageManager.registerFileManager("fm1", generateRandomFilePath()));

    storageManager.extend("fm1", 10);
    ASSERT_EQ(storageManager.getFileManager("fm1")->getNumPages(), 11);
}

TEST(StorageManagerTest, ExtendPageTilMaxWhenFileManagerIdNotFound) {
    StorageManager storageManager;
    ASSERT_TRUE(storageManager.registerFileManager("fm1", generateRandomFilePath()));

    ASSERT_THROW(storageManager.extend("fm2", 10), std::logic_error);
    ASSERT_EQ(storageManager.getFileManager("fm1")->getNumPages(), 1);
}

TEST(StorageManagerTest, GetNumPages) {
    StorageManager storageManager;
    ASSERT_TRUE(storageManager.registerFileManager("fm1", generateRandomFilePath()));

    ASSERT_EQ(storageManager.getNumPages("fm1"), 1);
}

TEST(StorageManagerTest, GetNumPagesWhenFileManagerIdNotFound) {
    StorageManager storageManager;
    ASSERT_TRUE(storageManager.registerFileManager("fm1", generateRandomFilePath()));

    ASSERT_THROW(storageManager.getNumPages("fm2"), std::logic_error);
}