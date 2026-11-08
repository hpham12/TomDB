//
// Created by Hieu Pham on 10/4/26.
//

#include <cstring>

#include "index/node_serializer.h"
#include "index/node.h"

template<typename K>
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

        std::unique_ptr<Node> currentNode;
        while (true) {
            std::span<const char> bytes(page->pageData.get(), PAGE_SIZE);
            if (NodeSerializer<K>::isLeaf(bytes)) {
                auto leafNode = NodeSerializer<K>::deserializeLeaf(bytes);
                currentNode = std::make_unique<LeafNode<K>>(leafNode);
            } else {
                auto innerNode = NodeSerializer<K>::deserializeInner(bytes);
                currentNode = std::make_unique<InnerNode<K>>(innerNode);
            }

            if (currentNode->getParentId() == INVALID_PAGE_ID) {
                break;
            }
            auto parentId = currentNode->getParentId();
            page = bufferManager->pinPage(parentId, EXCLUSIVE);
        }

        rootId = currentNode->getId();
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

template<typename K>
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
                bufferManager->unpinPage(rootId);
                splitLeafNode(&rootNode);
                rootId = rootNode.getParentId();
            }
        } else {
            auto rootNode = NodeSerializer<K>::deserializeInner(bytes);
            if (rootNode.isFull()) {
                bufferManager->unpinPage(rootId);
                splitInnerNode(&rootNode);
                rootId = rootNode.getParentId();
            }
        }
    }

    auto currentNodeId = rootId;
    std::shared_ptr<Node> currentNode;
    auto rootPage = bufferManager->pinPage(currentNodeId, SHARED);
    std::span<const char> bytes(rootPage->pageData.get(), PAGE_SIZE);

    auto isLeaf = NodeSerializer<K>::isLeaf(bytes);

    if (isLeaf) {
        auto leafNode = NodeSerializer<K>::deserializeLeaf(bytes);
        currentNode = std::make_unique<LeafNode<K>>(leafNode);
    } else {
        auto innerNode = NodeSerializer<K>::deserializeInner(bytes);
        currentNode = std::make_unique<InnerNode<K>>(innerNode);
    }

    std::shared_ptr<Node> parentNode = nullptr;
    while (!currentNode->isLeaf()) {
        std::shared_ptr<InnerNode<K>> castedNode = static_pointer_cast<InnerNode<K>>(currentNode);
        int keyIndex = castedNode->getKeyIndex(key);

        // load child and preemptively split it
        PageID childId = castedNode->children[keyIndex];
        auto childPage = bufferManager->pinPage(childId, EXCLUSIVE);
        std::span<const char> childBytes(childPage->pageData.get(), PAGE_SIZE);
        auto isChildLeaf = NodeSerializer<K>::isLeaf(childBytes);
        std::shared_ptr<Node> childNode;
        if (isChildLeaf) {
            auto leaf = NodeSerializer<K>::deserializeLeaf(childBytes);
            childNode = std::make_unique<LeafNode<K>>(leaf);
            if (childNode->isFull()) {
                bufferManager->unpinPage(childId);
                splitLeafNode(static_cast<LeafNode<K>*>(childNode.get()), castedNode.get());
                bufferManager->pinPage(childId, EXCLUSIVE);
            }
        } else {
            auto inner = NodeSerializer<K>::deserializeInner(childBytes);
            childNode = std::make_unique<InnerNode<K>>(inner);
            if (childNode->isFull()) {
                bufferManager->unpinPage(childId);
                splitInnerNode(static_cast<InnerNode<K>*>(childNode.get()), castedNode.get());
                bufferManager->pinPage(childId, EXCLUSIVE);
            }
        }

        keyIndex = castedNode->getKeyIndex(key);

        auto newChildId = castedNode->children[keyIndex];
        if (newChildId != childId) {
            childBytes = {childPage->pageData.get(), PAGE_SIZE};
            isChildLeaf = NodeSerializer<K>::isLeaf(childBytes);
            if (isChildLeaf) {
                auto leaf = NodeSerializer<K>::deserializeLeaf(childBytes);
                childNode = std::make_unique<LeafNode<K>>(leaf);
            } else {
                auto inner = NodeSerializer<K>::deserializeInner(childBytes);
                childNode = std::make_unique<InnerNode<K>>(inner);
            }
        }

        parentNode = currentNode;
        currentNode = childNode;
    }

    // Since current node might have been split, we need to determine whether they key should go to
    // currentNode, or its right sibling
    std::shared_ptr<LeafNode<K>> targetNode;
    if (parentNode != nullptr) {
        int keyIndex = static_pointer_cast<InnerNode<K>>(parentNode)->getKeyIndex(key);
        auto childId = static_pointer_cast<InnerNode<K>>(parentNode)->children[keyIndex];

        if (childId != currentNode->id) {
            auto childPage = bufferManager->pinPage(childId, EXCLUSIVE);
            std::span<const char> childBytes(childPage->pageData.get(), PAGE_SIZE);
            auto leaf = NodeSerializer<K>::deserializeLeaf(childBytes);
            targetNode = std::make_shared<LeafNode<K>>(leaf);
        } else {
            targetNode = static_pointer_cast<LeafNode<K>>(currentNode);
        }
    } else {
        targetNode = static_pointer_cast<LeafNode<K>>(currentNode);
    }


    PageID dataPageId;
    if (bufferManager->getNumPages(dataFileId) == 0) {
        dataPageId = bufferManager->getAvailablePageId(dataFileId);
    } else {
        dataPageId = bufferManager->getActivePageId(dataFileId);
    }

    auto dataPage = bufferManager->pinPage(dataPageId, SHARED);

    size_t slotIndx = dataPage->addTuple(value->clone(), nullptr);
    if (slotIndx == INVALID_VALUE) {
        bufferManager->extend(dataFileId);
        dataPageId = bufferManager->getAvailablePageId(dataFileId);
        dataPage = bufferManager->pinPage(dataPageId, SHARED);
        slotIndx = dataPage->addTuple(value->clone(), nullptr);
    }
    targetNode->records[targetNode->count] = Record{.pageId = dataPageId, .slotIndex = slotIndx};
    targetNode->keys[targetNode->count] = key;
    targetNode->incrementCount();
    auto currentIndexPage = bufferManager->pinPage(targetNode->id, SHARED);
    memcpy(currentIndexPage->pageData.get(), NodeSerializer<K>::serialize(*targetNode).data(), PAGE_SIZE);
    bufferManager->flushPage(targetNode->id);
    bufferManager->flushPage(dataPageId);

    return true;
}

template<typename K>
bool BTree<K>::remove(K key) {
    return false;
}

template<typename K>
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

template<typename K>
bool BTree<K>::update(K key, std::unique_ptr<Tuple> newValue) {
    return false;
}

template<typename K>
std::vector<std::unique_ptr<Tuple> > getAll(K from, K to) {
    throw std::logic_error("Method not implemented");
}

template<typename K>
void BTree<K>::splitInnerNode(InnerNode<K> *childNode, InnerNode<K> *parentNode) {
    throw std::logic_error("Method not implemented");
}

template<typename K>
void BTree<K>::splitInnerNode(InnerNode<K> *node) {
    // Find median key
    auto midIndex = InnerNode<K>::kCapacity / 2;
    auto midKey = node->keys[midIndex];

    // Split into sibling node
    node->setCount(midIndex);

    auto newNodeId = bufferManager->getAvailablePageId(indexFileId);

    InnerNode<K> newNode;
    newNode.id = newNodeId;

    newNode.children[0] = node->children[midIndex + 2];

    for (int i = midIndex + 1; i < InnerNode<K>::kCapacity; i++) {
        newNode.keys[newNode.count] = node->keys[i];
        newNode.children[newNode.count + 1] = node->children[i + 1];
        newNode.incrementCount();
    }

    // create new parent node
    auto newParentNodeId = bufferManager->getAvailablePageId(indexFileId);

    InnerNode<K> newParentNode;
    newParentNode.id = newParentNodeId;
    newParentNode.keys[0] = midKey;
    newParentNode.children[0] = node->id;
    newParentNode.children[1] = newNodeId;
    newParentNode.setCount(1);
    newParentNode.level = node->level + 1;

    // Make sure new node points to parent
    newNode.parentId = newParentNodeId;
    node->parentId = newParentNodeId;

    // flush change to new pages
    auto newNodePage = bufferManager->pinPage(newNodeId, SHARED);
    memcpy(newNodePage->pageData.get(), NodeSerializer<K>::serialize(newNode).data(), PAGE_SIZE);

    auto nodePage = bufferManager->pinPage(node->id, SHARED);
    memcpy(nodePage->pageData.get(), NodeSerializer<K>::serialize(*node).data(), PAGE_SIZE);

    auto newParentNodePage = bufferManager->pinPage(newParentNodeId, SHARED);
    memcpy(newParentNodePage->pageData.get(), NodeSerializer<K>::serialize(newParentNode).data(), PAGE_SIZE);

    bufferManager->flushPage(newNodeId);
    bufferManager->flushPage(node->id);
    bufferManager->flushPage(newParentNodeId);
}

template<typename K>
void BTree<K>::splitLeafNode(LeafNode<K> *childNode, InnerNode<K> *parentNode) {
    // Find median key
    auto midIndex = LeafNode<K>::kCapacity / 2;
    auto midKey = childNode->keys[midIndex];

    // Split into sibling node
    childNode->count = midIndex;

    auto newNodeId = bufferManager->getAvailablePageId(indexFileId);

    LeafNode<K> newNode;
    newNode.id = newNodeId;
    childNode->next = newNodeId;

    int index = 0;
    for (int i = midIndex; i < LeafNode<K>::kCapacity; i++) {
        newNode.keys[index] = childNode->keys[i];
        newNode.records[index] = childNode->records[i];
        newNode.incrementCount();
        index++;
    }

    // bubble to split key and child to parent node
    // Because we do preemptive splitting, parent node will always have capacity
    auto keyIndex = parentNode->addKey(midKey);
    parentNode->addChild(newNodeId, keyIndex + 1);
    parentNode->incrementCount();

    // Make sure new node points to parent
    newNode.parentId = parentNode->id;
    // flush change to new pages
    auto newNodePage = bufferManager->pinPage(newNodeId, SHARED);
    memcpy(newNodePage->pageData.get(), NodeSerializer<K>::serialize(newNode).data(), PAGE_SIZE);

    auto nodePage = bufferManager->pinPage(childNode->id, SHARED);
    memcpy(nodePage->pageData.get(), NodeSerializer<K>::serialize(*childNode).data(), PAGE_SIZE);

    auto newParentNodePage = bufferManager->pinPage(parentNode->id, SHARED);
    memcpy(newParentNodePage->pageData.get(), NodeSerializer<K>::serialize(*parentNode).data(), PAGE_SIZE);

    bufferManager->flushPage(newNodeId);
    bufferManager->flushPage(childNode->id);
    bufferManager->flushPage(parentNode->id);
}

template<typename K>
void BTree<K>::splitLeafNode(LeafNode<K> *node) {
    // Find median key
    auto midIndex = LeafNode<K>::kCapacity / 2;
    auto midKey = node->keys[midIndex];

    // Split into sibling node
    node->count = midIndex;

    auto newNodeId = bufferManager->getAvailablePageId(indexFileId);

    LeafNode<K> newNode;
    newNode.id = newNodeId;
    node->next = newNodeId;

    for (int i = midIndex; i < LeafNode<K>::kCapacity; i++) {
        newNode.keys[newNode.count] = node->keys[i];
        newNode.records[newNode.count] = node->records[i];
        newNode.incrementCount();
    }

    // create new parent node
    auto newParentNodeId = bufferManager->getAvailablePageId(indexFileId);

    InnerNode<K> newParentNode;
    newParentNode.id = newParentNodeId;
    newParentNode.keys[0] = midKey;
    newParentNode.children[0] = node->id;
    newParentNode.children[1] = newNodeId;
    newParentNode.setCount(1);
    newParentNode.level = node->level + 1;

    // Make sure new node points to parent
    newNode.parentId = newParentNodeId;
    node->parentId = newParentNodeId;

    // flush change to new pages
    auto newNodePage = bufferManager->pinPage(newNodeId, SHARED);
    memcpy(newNodePage->pageData.get(), NodeSerializer<K>::serialize(newNode).data(), PAGE_SIZE);

    auto nodePage = bufferManager->pinPage(node->id, SHARED);
    memcpy(nodePage->pageData.get(), NodeSerializer<K>::serialize(*node).data(), PAGE_SIZE);

    auto newParentNodePage = bufferManager->pinPage(newParentNodeId, SHARED);
    memcpy(newParentNodePage->pageData.get(), NodeSerializer<K>::serialize(newParentNode).data(), PAGE_SIZE);

    bufferManager->flushPage(newNodeId);
    bufferManager->flushPage(node->id);
    bufferManager->flushPage(newParentNodeId);
}
