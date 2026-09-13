//
// Created by Hieu Pham on 9/12/26.
//

#include "storage_manager.h"

#include <random>

#include "gtest/gtest.h"

class StorageManagerTest : public testing::Test {
protected:
    void TearDown() override {
        for (auto const &filePath : filePaths) {
            std::filesystem::remove(filePath);
        }
    }

    std::vector<std::string> filePaths;
};

std::string generateRandomFilePath() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution distr(1, 10000);
    int randomNum = distr(gen);

    return std::filesystem::temp_directory_path() / ("tomdb_test" + std::to_string(randomNum) + ".data");
}

TEST_F(StorageManagerTest, RegisterFileManager) {
    StorageManager storageManager;

    auto randomFilePath1 = generateRandomFilePath();
    auto randomFilePath2 = generateRandomFilePath();
    auto randomFilePath3 = generateRandomFilePath();

    filePaths.push_back(randomFilePath1);
    filePaths.push_back(randomFilePath1);
    filePaths.push_back(randomFilePath1);

    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath1));
    ASSERT_TRUE(storageManager.registerFileManager("fm2", randomFilePath2));
    ASSERT_TRUE(storageManager.registerFileManager("fm3", randomFilePath3));

    ASSERT_EQ(storageManager.getNumPages("fm1"), 1);
    ASSERT_EQ(storageManager.getNumPages("fm2"), 1);
    ASSERT_EQ(storageManager.getNumPages("fm3"), 1);

    ASSERT_FALSE(storageManager.registerFileManager("fm1", generateRandomFilePath()));
}

TEST_F(StorageManagerTest, GetPage) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath));

    ASSERT_NE(storageManager.getPage(PageID{.fileManagerId="fm1", .fileManagerPageId=0}), nullptr);
}

TEST_F(StorageManagerTest, GetPageWithFileManagerIdNotFound) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm2", randomFilePath));

    ASSERT_THROW(storageManager.getPage(PageID{.fileManagerId="fm1", .fileManagerPageId=0}), FileManagerNotRegisteredException);
}

TEST_F(StorageManagerTest, FlushPage) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);
    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath));

    std::fstream filestream(randomFilePath, std::ios::out | std::ios::in);

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
    auto *updatedSlots = reinterpret_cast<Slot*>(updatedPage->pageData.get());
    updatedSlots[0].empty = false;
    updatedSlots[0].offset = 123;
    updatedSlots[0].size = 123456;
}

TEST_F(StorageManagerTest, FlushPageWhenFileManagerIdNotFound) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);
    ASSERT_TRUE(storageManager.registerFileManager("fm2", randomFilePath));

    Page page;
    ASSERT_THROW(storageManager.flushPage(PageID{.fileManagerId="fm1", .fileManagerPageId=0}, page), FileManagerNotRegisteredException);
}

TEST_F(StorageManagerTest, ExtendPage) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath));

    storageManager.extend("fm1");
    ASSERT_EQ(storageManager.getFileManager("fm1")->getNumPages(), 2);
}

TEST_F(StorageManagerTest, ExtendPageWhenFileManagerIdNotFound) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm2", randomFilePath));

    ASSERT_THROW(storageManager.extend("fm1"), FileManagerNotRegisteredException);
    ASSERT_EQ(storageManager.getFileManager("fm2")->getNumPages(), 1);
}

TEST_F(StorageManagerTest, ExtendPageTilMax) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath));

    storageManager.extend("fm1", 10);
    ASSERT_EQ(storageManager.getFileManager("fm1")->getNumPages(), 11);
}

TEST_F(StorageManagerTest, ExtendPageTilMaxWhenFileManagerIdNotFound) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath));

    ASSERT_THROW(storageManager.extend("fm2", 10), FileManagerNotRegisteredException);
    ASSERT_EQ(storageManager.getFileManager("fm1")->getNumPages(), 1);
}

TEST_F(StorageManagerTest, GetNumPages) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath));

    ASSERT_EQ(storageManager.getNumPages("fm1"), 1);
}

TEST_F(StorageManagerTest, GetNumPagesWhenFileManagerIdNotFound) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath));

    ASSERT_THROW(storageManager.getNumPages("fm2"), FileManagerNotRegisteredException);
}