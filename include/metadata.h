//
// Created by Hieu Pham on 11/14/26.
//

#ifndef TOMDB_METADATA_H
#define TOMDB_METADATA_H
#include <span>
#include <string>
#include <unordered_set>

inline constexpr size_t METADATA_SIZE = 4096;

class Metadata {
    std::unordered_set<std::string> documents;

public:
    std::array<char, METADATA_SIZE> serialize();

    static Metadata deserialize(std::span<char> bytes);

    bool containsDocument(std::string document);

    void flush();
};


#endif //TOMDB_METADATA_H
