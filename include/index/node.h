//
// Created by Hieu Pham on 10/4/26.
//

#ifndef TOMDB_NODE_H
#define TOMDB_NODE_H
#include <utility>
#include <vector>

#include "commons.h"

class Node {
    PageID id = INVALID_PAGE_ID;
    PageID parentId = INVALID_PAGE_ID;
    uint16_t level;
    uint16_t count;

    template<class K>
    friend class BTree;

public:
    [[nodiscard]] PageID getParentId() const { return this->parentId; }
    [[nodiscard]] PageID getId() const { return id; }

    virtual bool isLeaf() {
        return level == 0;
    }

    Node(uint16_t level = 0, uint16_t count = 0)
        : level(level), count(count) {
    }

    virtual ~Node() = default;
};

struct Record {
    PageID pageId = INVALID_PAGE_ID;
    size_t slotIndex = 0;
};

template<class T>
class LeafNode : public Node {
    static constexpr uint16_t kCapacity = (PAGE_SIZE - (sizeof(PageID) * 2) - (sizeof(uint16_t) * 2))/(sizeof(Record) + sizeof(T));
    std::array<Record, kCapacity> records{};
    std::array<T, kCapacity> keys;

    template<class K>
    friend class BTree;

public:
    LeafNode() = default;
};

template<class T>
class InnerNode : public Node {
    static constexpr uint16_t kCapacity = (PAGE_SIZE - (sizeof(PageID) * 3) - (sizeof(uint16_t) * 2))/(sizeof(PageID) + sizeof(T));
    std::array<T, kCapacity> keys;
    std::array<PageID, kCapacity + 1> children;

    template<class K>
    friend class BTree;

public:
    InnerNode() = default;
};

#endif //TOMDB_NODE_H
