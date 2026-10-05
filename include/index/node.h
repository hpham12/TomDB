//
// Created by Hieu Pham on 10/4/26.
//

#ifndef TOMDB_NODE_H
#define TOMDB_NODE_H
#include <utility>
#include <vector>

#include "commons.h"

template <class T>
class Node {
    uint16_t numKeys;
    PageID pageId;
    std::vector<T> keys;
    PageID parentId = INVALID_PAGE_ID;

public:
    [[nodiscard]] uint16_t getNumKeys() const { return numKeys; }
    [[nodiscard]] PageID getParentId() const { return this->parentId; }
    [[nodiscard]] PageID getPageId() const { return pageId; }
    std::vector<T> getKeys() const { return keys; }
    virtual bool isLeaf() = 0;
    Node(uint16_t numKeys, PageID pageId) : numKeys(numKeys), pageId(std::move(pageId)) {}
    void setParentId(const PageID& parentId) { this->parentId = parentId; }
    virtual ~Node() = default;
};

struct Record {
    PageID pageId;
    size_t slotIndex;
};

template <class T>
class LeafNode : public Node<T> {
    uint16_t numValues;
    std::vector<Record> records;
public:
    [[nodiscard]] uint16_t getNumValues() const { return numValues; }
    [[nodiscard]] std::vector<PageID> getRecords() const { return this->getRecords(); }
    LeafNode(uint16_t numKeys, PageID pageId) : Node<T>(numKeys, pageId) {
        this->numValues = numKeys;
    }
    bool isLeaf() override {
        return true;
    }
};

template <class T>
class InnerNode : public Node<T> {
    uint16_t numChildren;
    std::vector<PageID> records;
public:
    [[nodiscard]] uint16_t getNumChildren() const { return numChildren; }
    InnerNode(uint16_t numKeys, PageID pageId) : Node<T>(numKeys, pageId) {
        this->numChildren = numKeys + 1;
    }
    bool isLeaf() override {
        return false;
    }
};

#endif //TOMDB_NODE_H
