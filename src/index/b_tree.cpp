//
// Created by Hieu Pham on 10/4/26.
//

#include "index/b_tree.h"

template <typename K>
BTree<K>::BTree(std::filesystem::path indexFilePath, std::string indexFileId,
    std::filesystem::path dataFilePath, std::string dataFileId) {
    this->indexFilePath = indexFilePath;
    this->indexFileId = indexFileId;
    this->dataFilePath = dataFilePath;
    this->dataFileId = dataFileId;
    bufferManager->registerFileManager(indexFileId, indexFilePath);
    bufferManager->registerFileManager(dataFileId, dataFilePath);


}

template <typename K>
bool BTree<K>::insert(K key, std::unique_ptr<Tuple> value) {

}

template <typename K>
bool BTree<K>::remove(K key) {

}

template <typename K>
std::unique_ptr<Tuple> BTree<K>::get(K key) {

}

template <typename K>
bool update(K key, std::unique_ptr<Tuple> newValue) {

}
