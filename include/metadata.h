//
// Created by Hieu Pham on 11/14/26.
//

#ifndef TOMDB_METADATA_H
#define TOMDB_METADATA_H
#include <fstream>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "index/b_tree.h"

inline constexpr size_t METADATA_SIZE = 8192;

class Metadata {
    std::unordered_set<std::string> documents;
    size_t currentSize = 0;
    std::unordered_map<std::string, std::shared_ptr<BTree<Key> > > indices;
    std::string rootPath;

    void initializeIndices();

    void initializeIndex(const std::string &document);

public:
    /**
     * Serialize metadata, using the following format:
     * <code>{totalDocuments}{document1NameSize}{document1Name}{document2NameSize}{document2Name}...</code>
     */
    std::array<char, METADATA_SIZE> serialize() const;

    std::fstream filestream;

    void deserialize(std::span<char> bytes);

    bool containsDocument(const std::string &document) const;

    bool addDocument(const std::string &document);

    bool removeDocument(const std::string &document);

    void flush();

    Metadata(std::string_view rootPath = DEFAULT_ROOT_PATH, std::string_view metadataFileName = DEFAULT_METADATA_FILE_NAME);

    std::shared_ptr<BTree<Key> > getIndex(const std::string &document) {
        if (indices.contains(document)) {
            return indices[document];
        }

        std::cerr << "Document " << document << " does not exist" << '\n';
        return nullptr;
    }
};


#endif //TOMDB_METADATA_H
