//
// Created by Hieu Pham on 10/18/26.
//

#include <gtest/gtest.h>

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
        leafNode.add(i, Record{.pageId=PageID{.fileManagerId="test", .fileManagerPageId=1}, .slotIndex=static_cast<size_t>(i)});
        ASSERT_EQ(leafNode.getCount(), i + 1);
    }

    // node is full, so another attempt to add will not succeed
    ASSERT_FALSE(leafNode.add(123, Record{.pageId=PageID{.fileManagerId="test", .fileManagerPageId=1}, .slotIndex=static_cast<size_t>(123)}));
}

TEST(LeafNodeTest, GetRecord) {
    LeafNode<int> leafNode;

    for (uint16_t i = 0; i < LeafNode<int>::getKCapacity(); i++) {
        leafNode.add(i, Record{.pageId=PageID{.fileManagerId="test", .fileManagerPageId=1}, .slotIndex=static_cast<size_t>(i)});
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

TEST(InnerNodeTest, Add) {
    InnerNode<int> innerNode;

    for (uint16_t i = 0; i < InnerNode<int>::getKCapacity(); i++) {
        innerNode.add(i, PageID{.fileManagerId="test", .fileManagerPageId=1});
        std::cout << i << '\n';
        ASSERT_EQ(innerNode.getCount(), i + 1);
    }

    // node is full, so another attempt to add will not succeed
    ASSERT_FALSE(innerNode.add(123, PageID{.fileManagerId="test", .fileManagerPageId=1}));
}
