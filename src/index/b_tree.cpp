//
// Created by Hieu Pham on 10/4/26.
//

#include "index/b_tree.h"
#include <cstring>

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

    // TODO: Handle case where index file exists

    //---
}

template <typename K>
bool BTree<K>::insert(K key, std::unique_ptr<Tuple> value) {
    // auto leafNode = std::make_unique<LeafNode<K>>();
    //
    // PageID dataPageId = bufferManager->getAvailablePageId(dataFileId);
    // PageID indexPageId = bufferManager->getAvailablePageId(indexFileId);
    // auto dataPage = bufferManager->pinPage(dataPageId, SHARED);
    // auto indexPage = bufferManager->pinPage(indexPageId, SHARED);
    // size_t slotIndx = dataPage->addTuple(std::move(value), nullptr);
    //
    // leafNode->records[leafNode->count] = Record{.pageId=dataPageId, .slotIndex=slotIndx};
    // leafNode->keys[leafNode->count] = key;
    // leafNode->id = indexPageId;
    // memcpy(indexPage->pageData.get(), reinterpret_cast<char*>(leafNode.get()), PAGE_SIZE);
    //
    // rootId = leafNode->id;
    //
    // bufferManager->flushPage(leafNode->id);
    // bufferManager->flushPage(dataPageId);

    return true;
}

template <typename K>
bool BTree<K>::remove(K key) {
    return false;
}

template <typename K>
std::unique_ptr<Tuple> BTree<K>::get(K key) {
    // TODO: Do this more seriously!
    // auto &pageData = bufferManager->pinPage(rootId, SHARED)->pageData;
    // auto *leafNode = reinterpret_cast<LeafNode<K>*>(pageData.get());
    //
    // for (size_t i = 0; i < 77; i++) {
    //     auto const &k = leafNode->keys[i];
    //     if (k == key) {
    //         Record record = leafNode->records[i];
    //         auto page = bufferManager->pinPage(record.pageId, SHARED);
    //         auto tuple = page->getTuple(record.slotIndex);
    //         return tuple;
    //     }
    // }

    return nullptr;
}

template <typename K>
bool BTree<K>::update(K key, std::unique_ptr<Tuple> newValue) {
    return false;
}
