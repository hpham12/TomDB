//
// Created by Hieu Pham on 11/14/26.
//

#include "metadata.h"

#include <cstring>
#include <filesystem>
#include <iostream>

Metadata::Metadata(std::string_view path) {
    auto filePath = std::filesystem::path(path);
    if (!std::filesystem::exists(filePath)) {
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }

        std::ofstream create(filePath);
    }

    filestream.open(filePath, std::fstream::in | std::fstream::out);

    if (!filestream.is_open()) {
        std::cerr << "Could not open metadata file " << filePath << '\n';
        exit(1);
    }

    filestream.seekg(0, std::ios::end);
    std::streampos fileSize = filestream.tellg();

    if (filestream.fail() || fileSize == std::streampos(-1)) {
        std::cerr << "Could not determine file size for " << filePath << '\n';
        exit(1);
    }

    if (fileSize == 0) {
        return;
    }

    if (fileSize > METADATA_SIZE) {
        std::cerr << "Metadata file is corrupted!" << '\n';
        exit(1);
    }

    std::array<char, METADATA_SIZE> buffer;

    filestream.seekg(0, std::ios::beg);
    filestream.read(buffer.data(), fileSize);
    deserialize(buffer);
}

std::array<char, METADATA_SIZE> Metadata::serialize() const {
    size_t offset = 0;
    size_t currentSize = 0;
    uint16_t totalDocuments = documents.size();
    std::array<char, METADATA_SIZE> buffer;
    memcpy(buffer.data(), &totalDocuments, sizeof(uint16_t));
    currentSize += sizeof(uint16_t);
    offset += sizeof(uint16_t);

    for (const auto &document : documents) {
        uint8_t documentNameLength = document.size();
        if (currentSize + documentNameLength + sizeof(documentNameLength) > METADATA_SIZE) {
            throw std::out_of_range("Metadata size is too big");
        }
        memcpy(buffer.data() + offset, &documentNameLength, sizeof(uint8_t));
        offset += sizeof(documentNameLength);
        document.copy(buffer.data() + offset, documentNameLength);
        offset += documentNameLength;
    }

    return buffer;
}

void Metadata::deserialize(std::span<char> bytes) {
    uint16_t totalDocuments{};
    std::memcpy(&totalDocuments, bytes.subspan(0, sizeof(uint16_t)).data(), sizeof(uint16_t));

    uint16_t documentsCount = 0;
    size_t currentOffset = sizeof(totalDocuments);

    while (documentsCount < totalDocuments) {
        uint8_t documentNameLength{};
        std::memcpy(&documentNameLength, bytes.subspan(currentOffset, sizeof(uint8_t)).data(), sizeof(uint8_t));
        currentOffset += sizeof(uint8_t);
        auto sub = bytes.subspan(currentOffset, documentNameLength);
        documents.emplace(sub.data(), sub.size());
        currentOffset += documentNameLength; // +1 for null-terminator
        documentsCount++;
    }
}

bool Metadata::containsDocument(const std::string &document) const {
    return documents.contains(document);
}

void Metadata::flush() {
    std::array<char, METADATA_SIZE> buffer = serialize();
    filestream.seekp(0, std::ios::beg);
    filestream.write(buffer.data(), METADATA_SIZE);
    filestream.flush();
}

bool Metadata::addDocument(const std::string& document) {
    bool sizeInvalid = document.empty() || document.size() > std::numeric_limits<uint8_t>::max();
    bool documentAlreadyExists = documents.contains(document);

    if (sizeInvalid || documentAlreadyExists) {
        return false;
    }

    documents.insert(document);
    return true;
}

bool Metadata::removeDocument(const std::string& document) {
    if (!documents.contains(document)) {
        return false;
    }

    documents.erase(document);
    return true;
}
