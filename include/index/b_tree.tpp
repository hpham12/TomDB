//
// Created by Hieu Pham on 10/4/26.
//

#include <cstring>

#include "index/node_serializer.h"
#include "index/node.h"

template <typename K>
BTree<K>::BTree(std::filesystem::path indexFilePath, std::string indexFileId,
                std::filesystem::path dataFilePath, std::string dataFileId) {
    this->indexFilePath = indexFilePath;
    this->indexFileId = indexFileId;
    this->dataFilePath = dataFilePath;
    this->dataFileId = dataFileId;
    bufferManager->registerFileManager(indexFileId, indexFilePath);
    bufferManager->registerFileManager(dataFileId, dataFilePath);

    // Index file exists
    auto numPages = bufferManager->getNumPages(indexFileId);

    if (numPages > 0) {
        auto page = bufferManager->pinPage(PageID{indexFileId, numPages - 1}, EXCLUSIVE);

        Node currentNode;
        while (true) {
            std::span<const char> bytes(page->pageData.get(), PAGE_SIZE);
            if (NodeSerializer<K>::isLeaf(bytes)) {
                currentNode = static_cast<Node>(NodeSerializer<int>::deserializeLeaf(bytes));
            } else {
                currentNode = static_cast<Node>(NodeSerializer<int>::deserializeInner(bytes));
            }

            if (currentNode.getParentId() == INVALID_PAGE_ID) {
                break;
            }
            auto parentId = currentNode.getParentId();
            page = bufferManager->pinPage(parentId, EXCLUSIVE);
        }

        rootId = currentNode.getId();
        return;
    }

    // Index file does not exist yet
    PageID id = bufferManager->getAvailablePageId(indexFileId);
    LeafNode<K> root;
    root.setId(id);
    auto page = bufferManager->pinPage(id, EXCLUSIVE);
    memcpy(page->pageData.get(), NodeSerializer<K>::serialize(root).data(), PAGE_SIZE);
    bufferManager->flushPage(id);
    rootId = id;
}

template <typename K>
bool BTree<K>::insert(K key, std::unique_ptr<Tuple> value) {
    // Approach: Traverse top-down and preemptively split full nodes

    // Split root node if full
    {
        auto rootPage = bufferManager->pinPage(rootId, EXCLUSIVE);
        std::span<const char> bytes(rootPage->pageData.get(), PAGE_SIZE);

        auto isLeaf = NodeSerializer<K>::isLeaf(bytes);

        if (isLeaf) {
            auto rootNode = NodeSerializer<K>::deserializeLeaf(bytes);
            if (rootNode.isFull()) {
                splitLeafNode(rootNode);
                rootId = rootNode.getParentId();
            }

        } else {
            auto rootNode = NodeSerializer<K>::deserializeInner(bytes);
            if (rootNode.isFull()) {
                splitInnerNode(rootNode);
                rootId = rootNode.getParentId();
            }
        }
    }

    auto currentNodeId = rootId;
    auto rootPage = bufferManager->pinPage(currentNodeId, EXCLUSIVE);
    std::span<const char> bytes(rootPage->pageData.get(), PAGE_SIZE);

    auto isLeaf = NodeSerializer<K>::isLeaf(bytes);

    Node currentNode;
    if (isLeaf) {
        currentNode = NodeSerializer<K>::deserializeLeaf(bytes);
    } else {
        currentNode = NodeSerializer<K>::deserializeInner(bytes);
    }

    while (!currentNode.isLeaf()) {
        auto castedNode = static_cast<InnerNode<K>>(currentNode);
        auto keyIndex = castedNode.getKeyIndex(key);

        // load child and preemptively split it
        PageID childId = castedNode.children[keyIndex];
        auto childPage = bufferManager->pinPage(childId, EXCLUSIVE);
        std::span<const char> childBytes(childPage->pageData.get(), PAGE_SIZE);
        auto isChildLeaf = NodeSerializer<K>::isLeaf(childBytes);
        Node childNode;
        if (isChildLeaf) {
            childNode = NodeSerializer<K>::deserializeLeaf(bytes);
            if (childNode.isFull()) {
                splitLeafNode(childNode, castedNode);
            }
        } else {
            childNode = NodeSerializer<K>::deserializeInner(bytes);
            if (childNode.isFull()) {
                splitInnerNode(childNode, castedNode);
            }
        }

        keyIndex = castedNode.getKeyIndex(key);

        auto newChildId = castedNode.children[keyIndex];
        if (newChildId != childId) {
            childPage = bufferManager->pinPage(childId, EXCLUSIVE);
            childBytes = {childPage->pageData.get(), PAGE_SIZE};
            isChildLeaf = NodeSerializer<K>::isLeaf(childBytes);
            if (isChildLeaf) {
                childNode = NodeSerializer<K>::deserializeLeaf(bytes);
            } else {
                childNode = NodeSerializer<K>::deserializeInner(bytes);
            }
        }

        currentNode = childNode;
    }

    // At this point, currentNode will be leaf node, so we can add the record directly
    auto castedNode = static_cast<LeafNode<K>>(currentNode);
    PageID dataPageId;
    if (bufferManager->getNumPages(dataFileId) == 0) {
        dataPageId = bufferManager->getAvailablePageId(dataFileId);
    } else {
        dataPageId = bufferManager->getActivePageId(dataFileId);
    }

    auto dataPage = bufferManager->pinPage(dataPageId, SHARED);
    size_t slotIndx = dataPage->addTuple(std::move(value), nullptr);
    if (slotIndx == INVALID_VALUE) {
        bufferManager->extend(dataFileId);
        dataPageId = bufferManager->getAvailablePageId(dataFileId);
        dataPage = bufferManager->pinPage(dataPageId, SHARED);
        slotIndx = dataPage->addTuple(std::move(value), nullptr);
    }
    castedNode.records[castedNode.count] = Record{.pageId=dataPageId, .slotIndex=slotIndx};
    castedNode.keys[castedNode.count] = key;
    castedNode.incrementCount();
    auto currentIndexPage = bufferManager->pinPage(castedNode.id, EXCLUSIVE);
    memcpy(currentIndexPage->pageData.get(), NodeSerializer<K>::serialize(castedNode).data(), PAGE_SIZE);
    bufferManager->flushPage(castedNode.id);
    bufferManager->flushPage(dataPageId);

    return true;
}

template <typename K>
bool BTree<K>::remove(K key) {
    return false;
}

template <typename K>
std::unique_ptr<Tuple> BTree<K>::get(K key) {
    auto &pageData = bufferManager->pinPage(rootId, SHARED)->pageData;
    std::span<const char> bytes(pageData.get(), PAGE_SIZE);
    auto leafNode = NodeSerializer<K>::deserializeLeaf(bytes);

    for (size_t i = 0; i < LeafNode<K>::kCapacity; i++) {
        auto const &k = leafNode.keys[i];
        if (k == key) {
            Record record = leafNode.records[i];
            auto page = bufferManager->pinPage(record.pageId, SHARED);
            auto tuple = page->getTuple(record.slotIndex);
            return tuple;
        }
    }

    return nullptr;
}

template <typename K>
bool BTree<K>::update(K key, std::unique_ptr<Tuple> newValue) {
    return false;
}

template <typename K>
std::vector<std::unique_ptr<Tuple>> getAll(K from , K to) {
    throw std::logic_error("Method not implemented");
}

template <typename K>
void BTree<K>::splitInnerNode(InnerNode<K> childNode, InnerNode<K> parentNode) {
    throw std::logic_error("Method not implemented");
}

template <typename K>
void BTree<K>::splitInnerNode(InnerNode<K> node) {
    // Find median key
    auto midIndex = InnerNode<K>::kCapacity/2;
    auto midKey = node.keys[midIndex];

    // Split into sibling node
    node.setCount(midIndex);

    auto newNodeId = bufferManager->getAvailablePageId(indexFileId);

    InnerNode<K> newNode;

    newNode.id = newNodeId;

    newNode.children[0] = node.children[midIndex + 2];

    for (int i = midIndex + 1; i < InnerNode<K>::kCapacity; i++) {
        newNode.addKey(node.keys[i]);
        newNode.addChild(node.children[i + 1]);
        newNode.incrementCount();
    }

    // create new parent node
    auto newParentNodeId = bufferManager->getAvailablePageId(indexFileId);

    InnerNode<K> newParentNode;
    newParentNode.id = newParentNodeId;
    newParentNode.addKey(midKey);
    newParentNode.addChild(node.id);
    newParentNode.addChild(newNodeId);
    newParentNode.setCount(1);
    newParentNode.level = node.level + 1;

    // Make sure new node points to parent
    newNode.parentId = newParentNodeId;

    // flush change to new pages
    auto newNodePage = bufferManager->pinPage(newNodeId, EXCLUSIVE);
    memcpy(newNodePage->pageData.get(), NodeSerializer<K>::serialize(newNode).data(), PAGE_SIZE);

    auto nodePage = bufferManager->pinPage(node.id, EXCLUSIVE);
    memcpy(nodePage->pageData.get(), NodeSerializer<K>::serialize(node).data(), PAGE_SIZE);

    auto newParentNodePage = bufferManager->pinPage(newParentNodeId, EXCLUSIVE);
    memcpy(newParentNodePage->pageData.get(), NodeSerializer<K>::serialize(newParentNode).data(), PAGE_SIZE);

    bufferManager->flushPage(newNodeId);
    bufferManager->flushPage(node.id);
    bufferManager->flushPage(newParentNodeId);
}

template <typename K>
void splitLeafNode(LeafNode<K> childNode, InnerNode<K> parentNode) {
    throw std::logic_error("Method not implemented");
}

template <typename K>
void splitLeafNode(LeafNode<K> node) {
    throw std::logic_error("Method not implemented");
}
