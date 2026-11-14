//
// Created by Hieu Pham on 11/14/26.
//

#ifndef TOMDB_METADATA_H
#define TOMDB_METADATA_H
#include <fstream>
#include <span>
#include <string>
#include <unordered_set>

inline constexpr std::string_view DEFAULT_METADATA_FILE_PATH = "/.tomdb/metadata";

inline constexpr size_t METADATA_SIZE = 4096;

class Metadata {
    std::unordered_set<std::string> documents;

public:
    /**
     * Serialize metadata, using the following format:
     * <code>{totalDocuments}{document1NameSize}{document1Name}{document2NameSize}{document2Name}...</code>
     */
    std::array<char, METADATA_SIZE> serialize() const;

    std::fstream filestream;

    void deserialize(std::span<char> bytes);

    bool containsDocument(const std::string &document) const;

    bool addDocument(const std::string& document);

    bool removeDocument(const std::string& document);

    void flush();

    Metadata(std::string_view metadataFilePath = DEFAULT_METADATA_FILE_PATH);
};


#endif //TOMDB_METADATA_H
