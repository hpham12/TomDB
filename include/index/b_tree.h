//
// Created by Hieu Pham on 10/4/26.
//

#ifndef TOMDB_B_TREE_H
#define TOMDB_B_TREE_H
#include <filesystem>

#include "node.h"
#include "buffer/buffer_manager.h"
#include "records/tuple.h"

template <class K>
class BTree {
  std::filesystem::path indexFilePath;
  std::filesystem::path dataFilePath;
  PageID rootId;
  std::unique_ptr<BufferManager> bufferManager = std::make_unique<BufferManager>();
  std::string indexFileId;
  std::string dataFileId;
  uint16_t innerNodeNumKeys;
  uint16_t leafNodeNumKeys;

public:
  BTree(std::filesystem::path indexFilePath, std::string indexFileId,
    std::filesystem::path dataFilePath, std::string dataFileId);
  bool insert(K key, std::unique_ptr<Tuple> value);
  bool remove(K key);
  std::unique_ptr<Tuple> get(K key);
  bool update(K key, std::unique_ptr<Tuple> newValue);
};

#endif //TOMDB_B_TREE_H
