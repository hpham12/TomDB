//
// Created by Hieu Pham on 10/6/26.
//

#include <gtest/gtest.h>
#include <memory>

#include "../test_utils.h"
#include "index/b_tree.h"

TEST(BTreeTest, InitializeEmptyIndexFile) {
    auto dataFilePath = generateRandomFilePath();
    auto indexFilePath = generateRandomFilePath();
    BTree<int> bTree(indexFilePath, "index", dataFilePath, "data");
    BufferManager bufferManager;
    bufferManager.registerFileManager("index", indexFilePath);
    bufferManager.registerFileManager("data", dataFilePath);

    ASSERT_EQ(bTree.getRootId(), (PageID{"index", 0}));

    auto rootPage = bufferManager.pinPage(PageID{.fileManagerId="index", .fileManagerPageId=0}, SHARED);
    std::span<const char> bytes(rootPage->pageData.get(), PAGE_SIZE);

    auto isLeaf = NodeSerializer<int>::isLeaf(bytes);
    ASSERT_TRUE(isLeaf);

    auto rootNode = NodeSerializer<int>::deserializeLeaf(bytes);
    ASSERT_EQ(rootNode.getLevel(), 0);
    ASSERT_EQ(rootNode.getCount(), 0);
    ASSERT_EQ(rootNode.getParentId(), INVALID_PAGE_ID);
    ASSERT_EQ(rootNode.getId(), (PageID{.fileManagerId="index", .fileManagerPageId=0}));
    ASSERT_EQ(rootNode.getNext(), INVALID_PAGE_ID);
}

TEST(BTreeTest, InitializeNonEmptyIndexFile) {
    auto dataFilePath = generateRandomFilePath();
    auto indexFilePath = generateRandomFilePath();
    BTree<int> bTree1(indexFilePath, "index", dataFilePath, "data");

    // btree2 reuse the index file of btree1
    BTree<int> bTree2(indexFilePath, "index", dataFilePath, "data");
    BufferManager bufferManager;
    bufferManager.registerFileManager("index", indexFilePath);
    bufferManager.registerFileManager("data", dataFilePath);

    ASSERT_EQ(bufferManager.getNumPages("index"), 1);
    ASSERT_EQ(bTree2.getRootId(), (PageID{"index", 0}));

    auto rootPage = bufferManager.pinPage(PageID{.fileManagerId="index", .fileManagerPageId=0}, SHARED);
    std::span<const char> bytes(rootPage->pageData.get(), PAGE_SIZE);

    auto isLeaf = NodeSerializer<int>::isLeaf(bytes);
    ASSERT_TRUE(isLeaf);

    auto rootNode = NodeSerializer<int>::deserializeLeaf(bytes);
    ASSERT_EQ(rootNode.getLevel(), 0);
    ASSERT_EQ(rootNode.getCount(), 0);
    ASSERT_EQ(rootNode.getParentId(), INVALID_PAGE_ID);
    ASSERT_EQ(rootNode.getId(), (PageID{.fileManagerId="index", .fileManagerPageId=0}));
    ASSERT_EQ(rootNode.getNext(), INVALID_PAGE_ID);
}

TEST(BTreeTest, Insert) {
    auto dataFilePath = generateRandomFilePath();
    auto indexFilePath = generateRandomFilePath();
    BTree<int> btree(indexFilePath, "index", dataFilePath, "data");

    // setup tuple

    std::unique_ptr<Tuple> tuple = std::make_unique<Tuple>();
    tuple->addField(std::make_unique<Field>(1));
    tuple->addField(std::make_unique<Field>((float) 123.123));
    tuple->addField(std::make_unique<Field>("Hello World"));

    btree.insert(1, std::move(tuple));

    auto resultTuple = btree.get(1);
    EXPECT_NE(resultTuple, nullptr);
    auto field1 = resultTuple->getField(0);
    EXPECT_EQ(field1->type, FieldType::INTEGER);
    EXPECT_EQ(field1->size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(field1->value.get()), 1);

    auto field2 = resultTuple->getField(1);
    EXPECT_EQ(field2->type, FieldType::FLOAT);
    EXPECT_EQ(field2->size, sizeof(float));
    EXPECT_FLOAT_EQ(*reinterpret_cast<float*>(field2->value.get()), 123.123);

    auto field3 = resultTuple->getField(2);
    EXPECT_EQ(field3->type, FieldType::STRING);
    EXPECT_EQ(field3->size, 11);
    EXPECT_STREQ(field3->value.get(), "Hello World");
}

TEST(BTreeTest, InsertSplitsLeafRoot) {
    auto dataFilePath = generateRandomFilePath();
    auto indexFilePath = generateRandomFilePath();
    BTree<int> tree(indexFilePath, "index", dataFilePath, "data");
    const auto originalRootId = tree.getRootId();
    const int capacity = LeafNode<int>::getKCapacity();

    // Fill the root leaf, then insert one more key to trigger its split.
    for (int key = 0; key <= capacity; ++key) {
        SCOPED_TRACE(key);
        std::unique_ptr<Tuple> tuple = std::make_unique<Tuple>();
        tuple->addField(std::make_unique<Field>(1));
        tuple->addField(std::make_unique<Field>((float) 123.123));
        tuple->addField(std::make_unique<Field>("Hello World"));
        tuple->addField(std::make_unique<Field>(key));
        ASSERT_TRUE(tree.insert(key, std::move(tuple)));
    }

    ASSERT_NE(tree.getRootId(), originalRootId);
    FileManager file(indexFilePath);
    auto rootPage = file.load(tree.getRootId().fileManagerPageId);
    std::span<const char> rootBytes(rootPage->pageData.get(), PAGE_SIZE);
    ASSERT_FALSE(NodeSerializer<int>::isLeaf(rootBytes));
    auto root = NodeSerializer<int>::deserializeInner(rootBytes);
    EXPECT_EQ(root.getId(), tree.getRootId());
    EXPECT_EQ(root.getParentId(), INVALID_PAGE_ID);
    EXPECT_EQ(root.getLevel(), 1);
    EXPECT_EQ(root.getCount(), 1);

    // The original leaf becomes the left child; its next link leads to the sibling.
    auto leftPage = file.load(originalRootId.fileManagerPageId);
    auto left = NodeSerializer<int>::deserializeLeaf({leftPage->pageData.get(), PAGE_SIZE});
    ASSERT_NE(left.getNext(), INVALID_PAGE_ID);
    ASSERT_NE(left.getNext(), originalRootId);
    auto rightPage = file.load(left.getNext().fileManagerPageId);
    auto right = NodeSerializer<int>::deserializeLeaf({rightPage->pageData.get(), PAGE_SIZE});
    EXPECT_EQ(left.getParentId(), tree.getRootId());
    EXPECT_EQ(right.getParentId(), tree.getRootId());
    EXPECT_EQ(left.getLevel(), 0);
    EXPECT_EQ(right.getLevel(), 0);
    EXPECT_EQ(left.getCount(), capacity / 2);
    EXPECT_EQ(right.getCount(), capacity - capacity / 2 + 1);
    EXPECT_EQ(right.getNext(), INVALID_PAGE_ID);
    for (int key = 0; key <= capacity; ++key) {
        SCOPED_TRACE(key);
        const auto record = key < capacity / 2 ? left.getRecord(key) : right.getRecord(key);
        EXPECT_NE(record, INVALID_RECORD);
    }
}

TEST(BTreeTest, InsertSplitsLeafWithParent) {
    auto dataFilePath = generateRandomFilePath();
    auto indexFilePath = generateRandomFilePath();
    BTree<int> tree(indexFilePath, "index", dataFilePath, "data");
    const auto originalRootId = tree.getRootId();
    const int capacity = LeafNode<int>::getKCapacity();
    const int midpoint = capacity / 2;

    // Split the root leaf first, then keep filling its right child.
    for (int key = 0; key < capacity + midpoint; ++key) {
        SCOPED_TRACE(key);
        auto tuple = std::make_unique<Tuple>();
        tuple->addField(std::make_unique<Field>(1));
        tuple->addField(std::make_unique<Field>((float) 123.123));
        tuple->addField(std::make_unique<Field>("Hello World"));
        tuple->addField(std::make_unique<Field>(key));
        ASSERT_TRUE(tree.insert(key, std::move(tuple)));
    }

    ASSERT_NE(tree.getRootId(), originalRootId);
    const auto parentId = tree.getRootId();
    FileManager file(indexFilePath);
    auto parentPage = file.load(parentId.fileManagerPageId);
    ASSERT_FALSE(NodeSerializer<int>::isLeaf({parentPage->pageData.get(), PAGE_SIZE}));
    auto parent = NodeSerializer<int>::deserializeInner({parentPage->pageData.get(), PAGE_SIZE});
    ASSERT_EQ(parent.getCount(), 1);

    auto leftPage = file.load(originalRootId.fileManagerPageId);
    auto left = NodeSerializer<int>::deserializeLeaf({leftPage->pageData.get(), PAGE_SIZE});
    ASSERT_NE(left.getNext(), INVALID_PAGE_ID);
    const auto middleId = left.getNext();
    auto middlePage = file.load(middleId.fileManagerPageId);
    auto middle = NodeSerializer<int>::deserializeLeaf({middlePage->pageData.get(), PAGE_SIZE});
    ASSERT_EQ(middle.getCount(), capacity);
    ASSERT_EQ(middle.getParentId(), parentId);
    ASSERT_EQ(middle.getNext(), INVALID_PAGE_ID);

    // One more insertion splits a leaf that already has a parent.
    const int lastKey = capacity + midpoint;
    auto tuple = std::make_unique<Tuple>();
    tuple->addField(std::make_unique<Field>(1));
    tuple->addField(std::make_unique<Field>((float) 123.123));
    tuple->addField(std::make_unique<Field>("Hello World"));
    tuple->addField(std::make_unique<Field>(lastKey));
    ASSERT_TRUE(tree.insert(lastKey, std::move(tuple)));

    file.refresh();
    ASSERT_EQ(tree.getRootId(), parentId);
    parentPage = file.load(parentId.fileManagerPageId);
    parent = NodeSerializer<int>::deserializeInner({parentPage->pageData.get(), PAGE_SIZE});
    EXPECT_EQ(parent.getId(), parentId);
    EXPECT_EQ(parent.getParentId(), INVALID_PAGE_ID);
    EXPECT_EQ(parent.getLevel(), 1);
    ASSERT_EQ(parent.getCount(), 2);

    leftPage = file.load(originalRootId.fileManagerPageId);
    left = NodeSerializer<int>::deserializeLeaf({leftPage->pageData.get(), PAGE_SIZE});
    EXPECT_EQ(left.getNext(), middleId);
    middlePage = file.load(middleId.fileManagerPageId);
    middle = NodeSerializer<int>::deserializeLeaf({middlePage->pageData.get(), PAGE_SIZE});
    ASSERT_NE(middle.getNext(), INVALID_PAGE_ID);
    ASSERT_NE(middle.getNext(), originalRootId);
    ASSERT_NE(middle.getNext(), middleId);
    auto rightPage = file.load(middle.getNext().fileManagerPageId);
    ASSERT_TRUE(NodeSerializer<int>::isLeaf({rightPage->pageData.get(), PAGE_SIZE}));
    auto right = NodeSerializer<int>::deserializeLeaf({rightPage->pageData.get(), PAGE_SIZE});

    EXPECT_EQ(left.getParentId(), parentId);
    EXPECT_EQ(middle.getParentId(), parentId);
    EXPECT_EQ(right.getParentId(), parentId);
    EXPECT_EQ(left.getCount(), midpoint);
    EXPECT_EQ(middle.getCount(), midpoint);
    EXPECT_EQ(right.getCount(), capacity - midpoint + 1);
    EXPECT_EQ(right.getLevel(), 0);
    EXPECT_EQ(right.getNext(), INVALID_PAGE_ID);
    FileManager dataFile(dataFilePath);
    for (int key = 0; key <= lastKey; ++key) {
        SCOPED_TRACE(key);
        const int childIndex = key < midpoint ? 0 : key < 2 * midpoint ? 1 : 2;
        EXPECT_EQ(parent.getKeyIndex(key), childIndex);
        const auto record = childIndex == 0 ? left.getRecord(key)
                          : childIndex == 1 ? middle.getRecord(key) : right.getRecord(key);
        ASSERT_NE(record, INVALID_RECORD);

        // Follow the persisted leaf record into the data file, bypassing the buffer cache.
        ASSERT_EQ(record.pageId.fileManagerId, "data");
        auto dataPage = dataFile.load(record.pageId.fileManagerPageId);
        ASSERT_NE(dataPage, nullptr);
        auto persistedTuple = dataPage->getTuple(record.slotIndex);
        ASSERT_NE(persistedTuple, nullptr);

        Tuple expected;
        expected.addField(std::make_unique<Field>(1));
        expected.addField(std::make_unique<Field>((float) 123.123));
        expected.addField(std::make_unique<Field>("Hello World"));
        expected.addField(std::make_unique<Field>(key));
        EXPECT_EQ(persistedTuple->serialize(), expected.serialize());
    }
}
