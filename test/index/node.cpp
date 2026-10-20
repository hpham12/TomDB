//
// Created by Hieu Pham on 10/18/26.
//

#include <gtest/gtest.h>
#include <iostream>

#include "index/node.h"

TEST(LeafNodeTest, Initialization) {
    LeafNode<int> leafNode;

    ASSERT_EQ(leafNode.getCount(), 0);
    ASSERT_EQ(leafNode.getLevel(), 0);
    ASSERT_TRUE(leafNode.isLeaf());
}

TEST(LeafNodeTest, GetKeyIndex) {
    LeafNode<int> leafNode;
    array<int, LeafNode<int>::kCapacity> keys{1, 4, 5, 10, 12, 17, 23};

    leafNode.keys = keys;
    leafNode.count = 7;

    ASSERT_EQ(leafNode.getKeyIndex(4), 2);
    ASSERT_EQ(leafNode.getKeyIndex(0), 0);
    ASSERT_EQ(leafNode.getKeyIndex(6), 3);
    ASSERT_EQ(leafNode.getKeyIndex(8), 3);
    ASSERT_EQ(leafNode.getKeyIndex(25), 7);
}

TEST(LeafNodeTest, Add) {
    LeafNode<int> leafNode;

    for (uint16_t i = 0; i < LeafNode<int>::getKCapacity(); i++) {
        ASSERT_TRUE(leafNode.add(i, (Record{.pageId=PageID{.fileManagerId="test", .fileManagerPageId=1}, .slotIndex=static_cast<size_t>(i)})));
        ASSERT_EQ(leafNode.getCount(), i + 1);
    }

    // node is full, so another attempt to add will not succeed
    ASSERT_FALSE(leafNode.add(123, Record{.pageId=PageID{.fileManagerId="test", .fileManagerPageId=1}, .slotIndex=static_cast<size_t>(123)}));
}

TEST(LeafNodeTest, GetRecord) {
    LeafNode<int> leafNode;

    for (uint16_t i = 0; i < LeafNode<int>::getKCapacity(); i++) {
        ASSERT_TRUE(leafNode.add(i, (Record{.pageId=PageID{.fileManagerId="test", .fileManagerPageId=1}, .slotIndex=static_cast<size_t>(i)})));
    }

    for (uint16_t i = 0; i < LeafNode<int>::getKCapacity(); i++) {
        auto expectedRecord = Record{.pageId=PageID{.fileManagerId="test", .fileManagerPageId=1}, .slotIndex=static_cast<size_t>(i)};

        ASSERT_EQ(leafNode.getRecord(i), expectedRecord);
    }
}

TEST(InnerNodeTest, Initialization) {
    InnerNode<int> innerNode;

    ASSERT_EQ(innerNode.getCount(), 0);
    ASSERT_EQ(innerNode.getLevel(), 1);
    ASSERT_FALSE(innerNode.isLeaf());
}

TEST(InnerNodeTest, GetKeyIndex) {
    InnerNode<int> innerNode;
    array<int, InnerNode<int>::kCapacity> keys{1, 4, 5, 10, 12, 17, 23};

    innerNode.keys = keys;
    innerNode.count = 7;

    ASSERT_EQ(innerNode.getKeyIndex(4), 2);
    ASSERT_EQ(innerNode.getKeyIndex(0), 0);
    ASSERT_EQ(innerNode.getKeyIndex(6), 3);
    ASSERT_EQ(innerNode.getKeyIndex(8), 3);
    ASSERT_EQ(innerNode.getKeyIndex(25), 7);
}

TEST(InnerNodeTest, AddKeyReturnsInsertionIndexWithoutIncreasingCount) {
    InnerNode<int> node;
    EXPECT_EQ(node.addKey(10), 0);
    EXPECT_EQ(node.getCount(), 0);
    node.setCount(1);
    EXPECT_EQ(node.getKeyIndex(10), 1);
}

TEST(InnerNodeTest, AddKeyAppendsAndRejectsFullNode) {
    InnerNode<int> node;
    for (uint16_t i = 0; i < node.getKCapacity(); ++i) {
        ASSERT_EQ(node.addKey(static_cast<int>(i) * 2), i);
        ASSERT_EQ(node.getCount(), i);
        node.setCount(i + 1);
        EXPECT_EQ(node.getKeyIndex(static_cast<int>(i) * 2), i + 1);
    }
    EXPECT_EQ(node.addKey(123), INVALID_INDEX);
    EXPECT_EQ(node.getCount(), node.getKCapacity());
}

TEST(InnerNodeTest, AddChildDoesNotIncreaseCount) {
    InnerNode<int> node;
    const PageID child{.fileManagerId="test", .fileManagerPageId=1};
    EXPECT_TRUE(node.addChild(child, 0));
    EXPECT_EQ(node.getCount(), 0);
}

TEST(InnerNodeTest, AddChildRejectsFullNode) {
    InnerNode<int> node;
    node.setCount(node.getKCapacity());
    const PageID child{.fileManagerId="test", .fileManagerPageId=1};
    EXPECT_FALSE(node.addChild(child, 1));
    EXPECT_EQ(node.getCount(), node.getKCapacity());
}

TEST(LeafNodeTest, AddShiftsKeysAndRecordsTogether) {
    LeafNode<int> node;
    const Record low{.pageId=PageID{.fileManagerId="test", .fileManagerPageId=1}, .slotIndex=0};
    const Record middle{.pageId=PageID{.fileManagerId="test", .fileManagerPageId=2}, .slotIndex=1};
    const Record high{.pageId=PageID{.fileManagerId="test", .fileManagerPageId=3}, .slotIndex=2};
    ASSERT_TRUE(node.add(10, low));
    ASSERT_TRUE(node.add(30, high));
    ASSERT_TRUE(node.add(20, middle));
    EXPECT_EQ(node.getCount(), 3);
    EXPECT_EQ(node.getRecord(10), low);
    EXPECT_EQ(node.getRecord(20), middle);
    EXPECT_EQ(node.getRecord(30), high);
}

TEST(LeafNodeTest, GetRecordReturnsInvalidForMissingKey) {
    LeafNode<int> node;
    const Record record{.pageId=PageID{.fileManagerId="test", .fileManagerPageId=1}, .slotIndex=0};
    ASSERT_TRUE(node.add(10, record));
    EXPECT_EQ(node.getRecord(20), INVALID_RECORD);
}
