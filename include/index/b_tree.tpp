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
    auto currentIndexPage = bufferManager->pinPage(rootId, EXCLUSIVE);
    std::span<const char> bytes(currentIndexPage->pageData.get(), PAGE_SIZE);

    auto isLeaf = NodeSerializer<K>::isLeaf(bytes);

    if (isLeaf) {
        auto leafNode = NodeSerializer<K>::deserializeLeaf(bytes);
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
        leafNode.records[leafNode.count] = Record{.pageId=dataPageId, .slotIndex=slotIndx};
        leafNode.keys[leafNode.count] = key;
        leafNode.incrementCount();
        memcpy(currentIndexPage->pageData.get(), NodeSerializer<K>::serialize(leafNode).data(), PAGE_SIZE);
        bufferManager->flushPage(leafNode.id);
        bufferManager->flushPage(dataPageId);
    }

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
