//
// Created by Hieu Pham on 10/4/26.
//

#ifndef TOMDB_B_TREE_H
#define TOMDB_B_TREE_H
#include <filesystem>

#include "buffer/buffer_manager.h"
#include "index/node.h"
#include "records/tuple.h"

template <class K>
class BTree {
  std::filesystem::path indexFilePath;
  std::filesystem::path dataFilePath;
  PageID rootId;
  std::unique_ptr<BufferManager> bufferManager = std::make_unique<BufferManager>();
  std::string indexFileId;
  std::string dataFileId;

public:
  BTree(std::filesystem::path indexFilePath, std::string indexFileId,
    std::filesystem::path dataFilePath, std::string dataFileId);
  bool insert(K key, std::unique_ptr<Tuple> value);
  bool remove(K key);
  std::unique_ptr<Tuple> get(K key);
  std::vector<std::unique_ptr<Tuple>> getAll(K from , K to);
  bool update(K key, std::unique_ptr<Tuple> newValue);

  // methods for testing
  [[nodiscard]] PageID getRootId() const {
    return rootId;
  }

  void splitInnerNode(InnerNode<K> *childNode, InnerNode<K> *parentNode);

  // This method should only be used for root node
  void splitInnerNode(InnerNode<K> *childNode);

  void splitLeafNode(LeafNode<K> *childNode,InnerNode<K> *parentNode);

  // This method should only be used for root node
  void splitLeafNode(LeafNode<K> *childNode);
};

#include "b_tree.tpp"

#endif //TOMDB_B_TREE_H
