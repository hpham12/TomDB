//
// Created by Hieu Pham on 10/6/26.
//

#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <numeric>
#include <random>
#include <set>

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

TEST(BTreeTest, GetAfterRootSplit) {
    BTree<int> tree(generateRandomFilePath(), "index", generateRandomFilePath(), "data");
    const int capacity = LeafNode<int>::getKCapacity();

    // Fill the root leaf, then verify the public lookup API before it splits.
    for (int key = 0; key < capacity; ++key) {
        auto tuple = std::make_unique<Tuple>();
        tuple->addField(std::make_unique<Field>(key));
        ASSERT_TRUE(tree.insert(key, std::move(tuple)));
    }
    ASSERT_NE(tree.get(1), nullptr);
    EXPECT_EQ(tree.get(-1), nullptr);

    // One more insertion promotes the root to an inner node.
    auto tuple = std::make_unique<Tuple>();
    tuple->addField(std::make_unique<Field>(capacity));
    ASSERT_TRUE(tree.insert(capacity, std::move(tuple)));

    // Look up every key through get(), covering both leaves and the separator.
    for (int key = 0; key <= capacity; ++key) {
        SCOPED_TRACE(key);
        std::unique_ptr<Tuple> result;
        ASSERT_NO_THROW(result = tree.get(key));
        ASSERT_NE(result, nullptr);
        Tuple expected;
        expected.addField(std::make_unique<Field>(key));
        EXPECT_EQ(result->serialize(), expected.serialize());
    }
    // Missing keys on either side must return nullptr after traversal.
    EXPECT_EQ(tree.get(-1), nullptr);
    EXPECT_EQ(tree.get(capacity + 1), nullptr);
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

TEST(BTreeTest, InsertSplitsInnerRoot) {
    auto dataFilePath = generateRandomFilePath();
    auto indexFilePath = generateRandomFilePath();
    BTree<int> tree(indexFilePath, "index", dataFilePath, "data");
    const auto originalLeafId = tree.getRootId();
    const int leafCapacity = LeafNode<int>::getKCapacity();
    const int innerCapacity = InnerNode<int>::getKCapacity();
    const int leafMidpoint = leafCapacity / 2;
    const int innerMidpoint = innerCapacity / 2;
    // Ascending keys always fill the rightmost leaf. Inserting key leafCapacity
    // creates the inner root with one separator. Each subsequent leafMidpoint
    // insertions splits another leaf and adds one more separator to that root.
    // Insert keys [0, lastKey) to fill the inner root exactly; reserve lastKey
    // for the next insertion, which triggers the preemptive inner-root split.
    const int lastKey = leafCapacity + (innerCapacity - 1) * leafMidpoint + 1;
    auto makeTuple = [](int key) {
        auto tuple = std::make_unique<Tuple>();
        tuple->addField(std::make_unique<Field>(1));
        tuple->addField(std::make_unique<Field>((float) 123.123));
        tuple->addField(std::make_unique<Field>("Hello World"));
        tuple->addField(std::make_unique<Field>(key));
        return tuple;
    };
    for (int key = 0; key < lastKey; ++key) {
        SCOPED_TRACE(key);
        ASSERT_TRUE(tree.insert(key, makeTuple(key)));
    }

    // Read the full root from disk and save its keys and child ordering so we
    // can verify that the split preserves every subtree in the correct position.
    const auto oldRootId = tree.getRootId();
    ASSERT_NE(oldRootId, originalLeafId);
    FileManager file(indexFilePath);
    auto oldRootPage = file.load(oldRootId.fileManagerPageId);
    ASSERT_FALSE(NodeSerializer<int>::isLeaf({oldRootPage->pageData.get(), PAGE_SIZE}));
    auto oldRoot = NodeSerializer<int>::deserializeInner({oldRootPage->pageData.get(), PAGE_SIZE});
    ASSERT_EQ(oldRoot.getLevel(), 1);
    ASSERT_EQ(oldRoot.getCount(), innerCapacity);
    ASSERT_EQ(oldRoot.getParentId(), INVALID_PAGE_ID);
    const auto oldChildren = oldRoot.children;
    const int promotedKey = oldRoot.keys[innerMidpoint];

    // The next insertion preemptively splits the full inner root.
    ASSERT_TRUE(tree.insert(lastKey, makeTuple(lastKey)));
    ASSERT_NE(tree.getRootId(), oldRootId);
    // Refresh the file reader after insertion so assertions see the newly
    // persisted pages rather than bytes buffered before the split.
    file.refresh();
    auto rootPage = file.load(tree.getRootId().fileManagerPageId);
    ASSERT_FALSE(NodeSerializer<int>::isLeaf({rootPage->pageData.get(), PAGE_SIZE}));
    auto root = NodeSerializer<int>::deserializeInner({rootPage->pageData.get(), PAGE_SIZE});
    // The tree gains one level: a new root with the promoted median separator,
    // the original inner node on the left, and a new inner sibling on the right.
    EXPECT_EQ(root.getId(), tree.getRootId());
    EXPECT_EQ(root.getParentId(), INVALID_PAGE_ID);
    EXPECT_EQ(root.getLevel(), 2);
    ASSERT_EQ(root.getCount(), 1);
    EXPECT_EQ(root.keys[0], promotedKey);
    ASSERT_EQ(root.children[0], oldRootId);
    ASSERT_NE(root.children[1], INVALID_PAGE_ID);
    ASSERT_NE(root.children[1], oldRootId);

    // Walk both inner nodes and their leaves in key order. Cross-check the
    // parent-child traversal against the leaf chain, detecting skipped or
    // duplicated leaves as well as incorrect parent pointers.
    std::set<size_t> visitedLeaves;
    int nextKey = 0;
    PageID nextLeafId = originalLeafId;
    FileManager dataFile(dataFilePath);
    for (int side = 0; side < 2; ++side) {
        SCOPED_TRACE(side);
        auto innerPage = file.load(root.children[side].fileManagerPageId);
        ASSERT_FALSE(NodeSerializer<int>::isLeaf({innerPage->pageData.get(), PAGE_SIZE}));
        auto inner = NodeSerializer<int>::deserializeInner({innerPage->pageData.get(), PAGE_SIZE});
        EXPECT_EQ(inner.getId(), root.children[side]);
        EXPECT_EQ(inner.getParentId(), tree.getRootId());
        EXPECT_EQ(inner.getLevel(), 1);
        // The median moves up into the new root, so neither inner child retains it.
        ASSERT_EQ(inner.getCount(), side == 0 ? innerMidpoint : innerCapacity - innerMidpoint - 1);
        for (int i = 0; i < inner.getCount(); ++i) {
            EXPECT_EQ(inner.keys[i], oldRoot.keys[side == 0 ? i : innerMidpoint + 1 + i]);
        }
        // An inner node has one more child than keys. The right sibling's first
        // child must be the old child at innerMidpoint + 1, immediately to the
        // right of the promoted separator.
        for (int child = 0; child <= inner.getCount(); ++child) {
            SCOPED_TRACE(child);
            const auto leafId = inner.children[child];
            ASSERT_EQ(leafId, oldChildren[side == 0 ? child : innerMidpoint + 1 + child]);
            ASSERT_EQ(leafId, nextLeafId);
            ASSERT_TRUE(visitedLeaves.insert(leafId.fileManagerPageId).second);
            auto leafPage = file.load(leafId.fileManagerPageId);
            ASSERT_TRUE(NodeSerializer<int>::isLeaf({leafPage->pageData.get(), PAGE_SIZE}));
            auto leaf = NodeSerializer<int>::deserializeLeaf({leafPage->pageData.get(), PAGE_SIZE});
            EXPECT_EQ(leaf.getId(), leafId);
            EXPECT_EQ(leaf.getParentId(), inner.getId());
            ASSERT_GT(leaf.getCount(), 0);
            ASSERT_LE(leaf.getCount(), leafCapacity);
            // Keys were inserted consecutively, so each leaf must contain the
            // next range of keys. Verify both levels route each key to this leaf.
            for (int slot = 0; slot < leaf.getCount(); ++slot, ++nextKey) {
                SCOPED_TRACE(nextKey);
                ASSERT_LE(nextKey, lastKey);
                EXPECT_EQ(root.getKeyIndex(nextKey), side);
                EXPECT_EQ(inner.getKeyIndex(nextKey), child);
                const auto record = leaf.getRecord(nextKey);
                ASSERT_NE(record, INVALID_RECORD);
                ASSERT_EQ(record.pageId.fileManagerId, "data");
                // Follow the saved record directly into the data file and compare
                // the entire tuple, verifying persistence independently of the cache.
                auto dataPage = dataFile.load(record.pageId.fileManagerPageId);
                auto persistedTuple = dataPage->getTuple(record.slotIndex);
                ASSERT_NE(persistedTuple, nullptr);
                EXPECT_EQ(persistedTuple->serialize(), makeTuple(nextKey)->serialize());
            }
            nextLeafId = leaf.getNext();
        }
    }
    // Every inserted tuple must be reachable exactly once. Splitting an inner
    // root redistributes its leaves without adding or removing any leaf pages.
    EXPECT_EQ(nextKey, lastKey + 1);
    EXPECT_EQ(nextLeafId, INVALID_PAGE_ID);
    EXPECT_EQ(visitedLeaves.size(), innerCapacity + 1);
}

TEST(BTreeTest, InsertSplitsInnerWithParent) {
    auto dataFilePath = generateRandomFilePath();
    auto indexFilePath = generateRandomFilePath();
    BTree<int> tree(indexFilePath, "index", dataFilePath, "data");
    const auto originalLeafId = tree.getRootId();
    const int leafCapacity = LeafNode<int>::getKCapacity();
    const int innerCapacity = InnerNode<int>::getKCapacity();
    const int leafMidpoint = leafCapacity / 2;
    const int innerMidpoint = innerCapacity / 2;
    // After the inner root splits, its right child has innerCapacity -
    // innerMidpoint - 1 keys. Another innerMidpoint + 1 leaf splits fills it.
    // Ascending insertions cause a leaf split every leafMidpoint keys.
    const int lastKey = leafCapacity + (innerCapacity + innerMidpoint) * leafMidpoint + 1;
    auto makeTuple = [](int key) {
        auto tuple = std::make_unique<Tuple>();
        tuple->addField(std::make_unique<Field>(1));
        tuple->addField(std::make_unique<Field>((float) 123.123));
        tuple->addField(std::make_unique<Field>("Hello World"));
        tuple->addField(std::make_unique<Field>(key));
        return tuple;
    };
    for (int key = 0; key < lastKey; ++key) {
        SCOPED_TRACE(key);
        ASSERT_TRUE(tree.insert(key, makeTuple(key)));
    }

    // Confirm the target is a full inner child under a root that has room.
    const auto rootId = tree.getRootId();
    FileManager file(indexFilePath);
    auto rootPage = file.load(rootId.fileManagerPageId);
    ASSERT_FALSE(NodeSerializer<int>::isLeaf({rootPage->pageData.get(), PAGE_SIZE}));
    auto beforeRoot = NodeSerializer<int>::deserializeInner({rootPage->pageData.get(), PAGE_SIZE});
    ASSERT_EQ(beforeRoot.getLevel(), 2);
    ASSERT_EQ(beforeRoot.getCount(), 1);
    const auto targetId = beforeRoot.children[1];
    auto targetPage = file.load(targetId.fileManagerPageId);
    ASSERT_FALSE(NodeSerializer<int>::isLeaf({targetPage->pageData.get(), PAGE_SIZE}));
    auto beforeTarget = NodeSerializer<int>::deserializeInner({targetPage->pageData.get(), PAGE_SIZE});
    ASSERT_EQ(beforeTarget.getLevel(), 1);
    ASSERT_EQ(beforeTarget.getParentId(), rootId);
    ASSERT_EQ(beforeTarget.getCount(), innerCapacity);

    // The next insertion must split that child into the existing root,
    // promoting its median without increasing the height of the tree.
    ASSERT_TRUE(tree.insert(lastKey, makeTuple(lastKey)));
    ASSERT_EQ(tree.getRootId(), rootId);
    file.refresh();
    rootPage = file.load(rootId.fileManagerPageId);
    auto root = NodeSerializer<int>::deserializeInner({rootPage->pageData.get(), PAGE_SIZE});
    EXPECT_EQ(root.getId(), rootId);
    EXPECT_EQ(root.getParentId(), INVALID_PAGE_ID);
    EXPECT_EQ(root.getLevel(), 2);
    ASSERT_EQ(root.getCount(), 2);
    EXPECT_EQ(root.keys[0], beforeRoot.keys[0]);
    EXPECT_EQ(root.keys[1], beforeTarget.keys[innerMidpoint]);
    EXPECT_EQ(root.children[0], beforeRoot.children[0]);
    EXPECT_EQ(root.children[1], targetId);
    ASSERT_NE(root.children[2], INVALID_PAGE_ID);
    ASSERT_NE(root.children[2], root.children[0]);
    ASSERT_NE(root.children[2], targetId);

    // Walk all three persisted inner nodes, checking that the split preserves
    // the target's keys and child ordering and reparents the moved leaves.
    FileManager dataFile(dataFilePath);
    std::set<size_t> visitedLeaves;
    PageID nextLeafId = originalLeafId;
    int nextKey = 0;
    for (int side = 0; side < 3; ++side) {
        SCOPED_TRACE(side);
        auto innerPage = file.load(root.children[side].fileManagerPageId);
        ASSERT_FALSE(NodeSerializer<int>::isLeaf({innerPage->pageData.get(), PAGE_SIZE}));
        auto inner = NodeSerializer<int>::deserializeInner({innerPage->pageData.get(), PAGE_SIZE});
        EXPECT_EQ(inner.getId(), root.children[side]);
        EXPECT_EQ(inner.getParentId(), rootId);
        EXPECT_EQ(inner.getLevel(), 1);
        ASSERT_EQ(inner.getCount(), side < 2 ? innerMidpoint : innerCapacity - innerMidpoint - 1);
        if (side > 0) {
            for (int i = 0; i < inner.getCount(); ++i) {
                EXPECT_EQ(inner.keys[i], beforeTarget.keys[side == 1 ? i : innerMidpoint + 1 + i]);
            }
        }
        for (int child = 0; child <= inner.getCount(); ++child) {
            SCOPED_TRACE(child);
            const auto leafId = inner.children[child];
            if (side > 0) {
                EXPECT_EQ(leafId, beforeTarget.children[side == 1 ? child : innerMidpoint + 1 + child]);
            }
            ASSERT_EQ(leafId, nextLeafId);
            ASSERT_TRUE(visitedLeaves.insert(leafId.fileManagerPageId).second);
            auto leafPage = file.load(leafId.fileManagerPageId);
            ASSERT_TRUE(NodeSerializer<int>::isLeaf({leafPage->pageData.get(), PAGE_SIZE}));
            auto leaf = NodeSerializer<int>::deserializeLeaf({leafPage->pageData.get(), PAGE_SIZE});
            EXPECT_EQ(leaf.getId(), leafId);
            EXPECT_EQ(leaf.getParentId(), inner.getId());
            ASSERT_GT(leaf.getCount(), 0);
            ASSERT_LE(leaf.getCount(), leafCapacity);
            // Verify every key routes through both inner levels to the correct
            // leaf, then read its complete tuple directly from the data file.
            for (int slot = 0; slot < leaf.getCount(); ++slot, ++nextKey) {
                SCOPED_TRACE(nextKey);
                ASSERT_LE(nextKey, lastKey);
                EXPECT_EQ(root.getKeyIndex(nextKey), side);
                EXPECT_EQ(inner.getKeyIndex(nextKey), child);
                const auto record = leaf.getRecord(nextKey);
                ASSERT_NE(record, INVALID_RECORD);
                ASSERT_EQ(record.pageId.fileManagerId, "data");
                auto dataPage = dataFile.load(record.pageId.fileManagerPageId);
                auto persistedTuple = dataPage->getTuple(record.slotIndex);
                ASSERT_NE(persistedTuple, nullptr);
                EXPECT_EQ(persistedTuple->serialize(), makeTuple(nextKey)->serialize());
            }
            nextLeafId = leaf.getNext();
        }
    }
    // No tuple or leaf may be lost, duplicated, or disconnected by the split.
    EXPECT_EQ(nextKey, lastKey + 1);
    EXPECT_EQ(nextLeafId, INVALID_PAGE_ID);
    EXPECT_EQ(visitedLeaves.size(), innerCapacity + innerMidpoint + 2);
}

TEST(BTreeTest, InsertRandomOrder) {
    auto dataFilePath = generateRandomFilePath();
    auto indexFilePath = generateRandomFilePath();
    BTree<int> tree(indexFilePath, "index", dataFilePath, "data");
    const int leafCapacity = LeafNode<int>::getKCapacity();
    const int innerCapacity = InnerNode<int>::getKCapacity();
    // More tuples than two full inner nodes can address: this forces both an
    // inner-root split and at least one inner split under an existing parent.
    const int tupleCount = 2 * leafCapacity * (innerCapacity + 1) + 1;
    std::vector<int> keys(tupleCount);
    std::iota(keys.begin(), keys.end(), 0);
    // Keep failures reproducible while exercising insertions between existing keys.
    constexpr uint32_t seed = 6422;
    std::mt19937 random(seed);
    std::shuffle(keys.begin(), keys.end(), random);
    SCOPED_TRACE(seed);
    for (int key : keys) {
        SCOPED_TRACE(key);
        auto tuple = std::make_unique<Tuple>();
        tuple->addField(std::make_unique<Field>(key));
        tuple->addField(std::make_unique<Field>("Hello World"));
        bool inserted = false;
        ASSERT_NO_THROW(inserted = tree.insert(key, std::move(tuple)));
        ASSERT_TRUE(inserted);
    }

    // Use fresh file readers so every lookup checks persisted index and data
    // pages. Follow the actual separators, rather than predicting split positions.
    FileManager file(indexFilePath);
    FileManager dataFile(dataFilePath);
    auto rootPage = file.load(tree.getRootId().fileManagerPageId);
    ASSERT_FALSE(NodeSerializer<int>::isLeaf({rootPage->pageData.get(), PAGE_SIZE}));
    auto root = NodeSerializer<int>::deserializeInner({rootPage->pageData.get(), PAGE_SIZE});
    ASSERT_EQ(root.getLevel(), 2);
    ASSERT_GE(root.getCount(), 2);
    std::set<size_t> reachedLeaves;
    PageID firstLeafId = INVALID_PAGE_ID;
    for (int key = 0; key < tupleCount; ++key) {
        SCOPED_TRACE(key);
        auto nodeId = tree.getRootId();
        auto parentId = INVALID_PAGE_ID;
        // Each descent must decrease the level, bounding traversal even if a
        // bad child pointer creates a cycle in the persisted tree.
        for (int level = root.getLevel(); level > 0; --level) {
            auto page = file.load(nodeId.fileManagerPageId);
            ASSERT_FALSE(NodeSerializer<int>::isLeaf({page->pageData.get(), PAGE_SIZE}));
            auto inner = NodeSerializer<int>::deserializeInner({page->pageData.get(), PAGE_SIZE});
            ASSERT_EQ(inner.getId(), nodeId);
            ASSERT_EQ(inner.getParentId(), parentId);
            ASSERT_EQ(inner.getLevel(), level);
            ASSERT_GT(inner.getCount(), 0);
            ASSERT_LE(inner.getCount(), innerCapacity);
            ASSERT_TRUE(std::is_sorted(inner.keys.begin(), inner.keys.begin() + inner.getCount()));
            parentId = nodeId;
            nodeId = inner.children[inner.getKeyIndex(key)];
            ASSERT_NE(nodeId, INVALID_PAGE_ID);
            ASSERT_EQ(nodeId.fileManagerId, "index");
        }
        auto page = file.load(nodeId.fileManagerPageId);
        ASSERT_TRUE(NodeSerializer<int>::isLeaf({page->pageData.get(), PAGE_SIZE}));
        auto leaf = NodeSerializer<int>::deserializeLeaf({page->pageData.get(), PAGE_SIZE});
        ASSERT_EQ(leaf.getId(), nodeId);
        ASSERT_EQ(leaf.getParentId(), parentId);
        ASSERT_GT(leaf.getCount(), 0);
        ASSERT_LE(leaf.getCount(), leafCapacity);
        if (key == 0) firstLeafId = nodeId;
        reachedLeaves.insert(nodeId.fileManagerPageId);
        const auto record = leaf.getRecord(key);
        ASSERT_NE(record, INVALID_RECORD);
        ASSERT_EQ(record.pageId.fileManagerId, "data");
        auto dataPage = dataFile.load(record.pageId.fileManagerPageId);
        auto tuple = dataPage->getTuple(record.slotIndex);
        ASSERT_NE(tuple, nullptr);
        Tuple expected;
        expected.addField(std::make_unique<Field>(key));
        expected.addField(std::make_unique<Field>("Hello World"));
        ASSERT_EQ(tuple->serialize(), expected.serialize());
    }

    // Splitting a leaf in the middle must preserve its previous next link.
    // The linked leaves must cover exactly the same pages as the routed lookups.
    std::set<size_t> linkedLeaves;
    auto leafId = firstLeafId;
    int recordCount = 0;
    while (leafId != INVALID_PAGE_ID) {
        ASSERT_EQ(leafId.fileManagerId, "index");
        ASSERT_TRUE(linkedLeaves.insert(leafId.fileManagerPageId).second);
        ASSERT_TRUE(reachedLeaves.contains(leafId.fileManagerPageId));
        auto page = file.load(leafId.fileManagerPageId);
        ASSERT_TRUE(NodeSerializer<int>::isLeaf({page->pageData.get(), PAGE_SIZE}));
        auto leaf = NodeSerializer<int>::deserializeLeaf({page->pageData.get(), PAGE_SIZE});
        recordCount += leaf.getCount();
        leafId = leaf.getNext();
    }
    EXPECT_EQ(linkedLeaves, reachedLeaves);
    EXPECT_EQ(recordCount, tupleCount);
}
