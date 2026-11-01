//
// Created by Hieu Pham on 9/12/26.
//

#include "storage/storage_manager.h"
#include "../test_utils.h"

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

TEST_F(StorageManagerTest, RegisterFileManager) {
    StorageManager storageManager;

    auto randomFilePath1 = generateRandomFilePath();
    auto randomFilePath2 = generateRandomFilePath();
    auto randomFilePath3 = generateRandomFilePath();

    filePaths.push_back(randomFilePath1);
    filePaths.push_back(randomFilePath2);
    filePaths.push_back(randomFilePath3);

    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath1));
    ASSERT_TRUE(storageManager.registerFileManager("fm2", randomFilePath2));
    ASSERT_TRUE(storageManager.registerFileManager("fm3", randomFilePath3));

    ASSERT_EQ(storageManager.getNumPages("fm1"), 0);
    ASSERT_EQ(storageManager.getNumPages("fm2"), 0);
    ASSERT_EQ(storageManager.getNumPages("fm3"), 0);

    ASSERT_FALSE(storageManager.registerFileManager("fm1", generateRandomFilePath()));
}

TEST_F(StorageManagerTest, GetPage) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath));

    ASSERT_THROW(storageManager.getPage((PageID{"fm1", 0})), std::out_of_range);
    storageManager.extend("fm1");
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

    storageManager.extend("fm1");

    Page page;
    Slot *slots = reinterpret_cast<Slot*>(page.pageData.get());
    slots[0].empty = false;
    slots[0].offset = 123;
    slots[0].size = 123456;

    ASSERT_TRUE(storageManager.flushPage(PageID{.fileManagerId="fm1", .fileManagerPageId=0}, page));

    auto &fileManager = storageManager.getFileManager("fm1");
    auto updatedPage = fileManager->load(0);
    auto *updatedSlots = reinterpret_cast<Slot*>(updatedPage->pageData.get());
    ASSERT_FALSE(updatedSlots[0].empty);
    ASSERT_EQ(updatedSlots[0].offset, 123);
    ASSERT_EQ(updatedSlots[0].size, 123456);
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
    ASSERT_EQ(storageManager.getFileManager("fm1")->getNumPages(), 1);
}

TEST_F(StorageManagerTest, ExtendPageWhenFileManagerIdNotFound) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm2", randomFilePath));

    ASSERT_THROW(storageManager.extend("fm1"), FileManagerNotRegisteredException);
    ASSERT_EQ(storageManager.getFileManager("fm2")->getNumPages(), 0);
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
    ASSERT_EQ(storageManager.getFileManager("fm1")->getNumPages(), 0);
}

TEST_F(StorageManagerTest, GetNumPages) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath));

    ASSERT_EQ(storageManager.getNumPages("fm1"), 0);
}

TEST_F(StorageManagerTest, GetNumPagesWhenFileManagerIdNotFound) {
    StorageManager storageManager;
    auto randomFilePath = generateRandomFilePath();
    filePaths.push_back(randomFilePath);

    ASSERT_TRUE(storageManager.registerFileManager("fm1", randomFilePath));

    ASSERT_THROW(storageManager.getNumPages("fm2"), FileManagerNotRegisteredException);
}

TEST_F(StorageManagerTest, GetAvailablePageId) {
    StorageManager manager;
    const auto filePath = generateRandomFilePath();
    filePaths.push_back(filePath);
    ASSERT_TRUE(manager.registerFileManager("fm", filePath));
    for (size_t expectedId = 0; expectedId < 2; ++expectedId) {
        const auto pageId = manager.getAvailablePageId("fm");
        EXPECT_EQ(pageId.fileManagerId, "fm");
        ASSERT_EQ(pageId.fileManagerPageId, expectedId);
        ASSERT_EQ(manager.getNumPages("fm"), expectedId + 1);
        ASSERT_EQ(std::filesystem::file_size(filePath), (expectedId + 1) * PAGE_SIZE);
    }
}

TEST_F(StorageManagerTest, GetAvailablePageIdWhenFileManagerNotRegistered) {
    StorageManager manager;
    const auto filePath = generateRandomFilePath();
    filePaths.push_back(filePath);
    ASSERT_TRUE(manager.registerFileManager("fm", filePath));
    ASSERT_THROW(manager.getAvailablePageId("missing"), FileManagerNotRegisteredException);
    EXPECT_EQ(manager.getNumPages("fm"), 0);
}

TEST_F(StorageManagerTest, GetActivePageIdReturnsLastPageWithoutAllocating) {
    StorageManager manager;
    const auto filePath = generateRandomFilePath();
    filePaths.push_back(filePath);
    ASSERT_TRUE(manager.registerFileManager("fm", filePath));
    EXPECT_THROW(manager.getActivePageId("fm"), std::out_of_range);
    EXPECT_EQ(manager.getNumPages("fm"), 0);
    EXPECT_THROW(manager.getActivePageId("missing"), FileManagerNotRegisteredException);
    for (size_t i = 0; i < 2; ++i) {
        const auto allocated = manager.getAvailablePageId("fm");
        EXPECT_EQ(manager.getActivePageId("fm"), allocated);
        EXPECT_EQ(manager.getActivePageId("fm"), allocated);
        EXPECT_EQ(manager.getNumPages("fm"), i + 1);
        EXPECT_EQ(std::filesystem::file_size(filePath), (i + 1) * PAGE_SIZE);
    }
}
