//
// Created by Hieu Pham on 10/4/26.
//

#ifndef TOMDB_NODE_H
#define TOMDB_NODE_H
#include <array>

#include "commons.h"

class Node {
protected:
    PageID id = INVALID_PAGE_ID;
    PageID parentId = INVALID_PAGE_ID;
    uint16_t level;
    uint16_t count;

public:
    [[nodiscard]] PageID getParentId() const { return this->parentId; }
    [[nodiscard]] PageID getId() const { return id; }

    virtual bool isLeaf() {
        return level == 0;
    }

    Node(uint16_t level = 0, uint16_t count = 0)
        : level(level), count(count) {
    }

    [[nodiscard]] uint16_t getLevel() const { return level; }
    [[nodiscard]] uint16_t getCount() const { return count; }

    virtual ~Node() = default;
};

template<class K>
class LeafNode : public Node {
    static constexpr uint16_t kCapacity = (PAGE_SIZE - (sizeof(PageID) * 2) - (sizeof(uint16_t) * 2))/(sizeof(Record) + sizeof(K));
    std::array<Record, kCapacity> records{};
    std::array<K, kCapacity> keys;
    PageID next = INVALID_PAGE_ID;

    FRIEND_TEST(LeafNodeTest, GetKeyIndex);

public:
    LeafNode() : Node(0, 0) {}

    /**
     * Get the record, given the <code>key</code>
     *
     * @return result record if key is found, or <code>nullptr</code> otherwise
     */
    Record getRecord(K key) const;

    /**
     * Get the index where <code>key</code> will be placed in the <code>keys</code> array
     */
    size_t getKeyIndex(K key) const;

    /**
     * Add a key and record to the node. This method will throw an exception if the node is full
     */
    bool add(K key, Record record);

    static uint16_t getKCapacity() {
        return kCapacity;
    }
};

template<class K>
class InnerNode : public Node {
    static constexpr uint16_t kCapacity = (PAGE_SIZE - (sizeof(PageID) * 3) - (sizeof(uint16_t) * 2))/(sizeof(PageID) + sizeof(K));
    std::array<K, kCapacity> keys;
    std::array<PageID, kCapacity + 1> children;

    FRIEND_TEST(InnerNodeTest, GetKeyIndex);


public:
    InnerNode() : Node(1, 0) {}

    /**
     * Get the index where <code>key</code> will be placed in the <code>keys</code> array
     */
    size_t getKeyIndex(K key) const;

    /**
     * Add a key and child to the node. This method will throw an exception if the node is full.
     * The <code>child</code> will need to contain all keys that are greater than or equal to <code>key</code>
     */
    bool add(K key, PageID child);

    static uint16_t getKCapacity() {
        return kCapacity;
    }
};

#include "node.tpp"

#endif //TOMDB_NODE_H
