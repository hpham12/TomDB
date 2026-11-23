//
// Created by Hieu Pham on 11/14/26.
//

#include "metadata.h"
#include "test_utils.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <filesystem>
#include <gtest/gtest.h>

class MetadataTest : public testing::Test {
protected:
    const std::string rootPath = std::filesystem::temp_directory_path().append("/");
    const std::string fileName = generateRandomFileName();
    const std::filesystem::path path = rootPath + fileName;

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
};

using MetadataConstructorTest = MetadataTest;
using MetadataSerializeTest = MetadataTest;
using MetadataAddDocumentTest = MetadataTest;

TEST_F(MetadataConstructorTest, CreatesMissingFile) {
    ASSERT_FALSE(std::filesystem::exists(path));

    Metadata metadata(rootPath, fileName);

    ASSERT_TRUE(std::filesystem::is_regular_file(path));
    EXPECT_EQ(std::filesystem::file_size(path), 0u);
    EXPECT_FALSE(metadata.containsDocument("users"));
}

TEST_F(MetadataConstructorTest, OpensExistingEmptyFile) {
    std::ofstream(path).close();
    ASSERT_EQ(std::filesystem::file_size(path), 0u);

    Metadata metadata(rootPath, fileName);

    EXPECT_FALSE(metadata.containsDocument("users"));
    EXPECT_EQ(std::filesystem::file_size(path), 0u);
}

TEST_F(MetadataConstructorTest, LoadsDocumentsFromExistingFile) { {
        // Encode the fixture directly so this test does not depend on serialize().
        std::ofstream output(path, std::ios::binary);
        const std::uint16_t count = 2;
        output.write(reinterpret_cast<const char *>(&count), sizeof(count));
        output.write("\x05users", 6);
        output.write("\x06orders", 7);
        output.close();
        ASSERT_TRUE(output.good());
    }

    Metadata metadata(rootPath, fileName);

    EXPECT_TRUE(metadata.containsDocument("users"));
    EXPECT_TRUE(metadata.containsDocument("orders"));
    EXPECT_FALSE(metadata.containsDocument("missing"));

    EXPECT_EQ(metadata.getIndex("missing"), nullptr);
    EXPECT_NE(metadata.getIndex("users"), nullptr);
    EXPECT_NE(metadata.getIndex("orders"), nullptr);

}

TEST_F(MetadataSerializeTest, WritesZeroDocumentCountForEmptyMetadata) {
    Metadata metadata(rootPath, fileName);

    const auto bytes = metadata.serialize();

    std::uint16_t count{};
    std::memcpy(&count, bytes.data(), sizeof(count));
    EXPECT_EQ(count, 0U);
}

TEST_F(MetadataSerializeTest, WritesLengthAndNameForEachDocument) {
    Metadata metadata(rootPath, fileName);
    const std::uint16_t expectedCount = 2;
    std::array<char, sizeof(expectedCount) + 13> input{};
    const std::string_view usersFirst("\x05users\x06orders", 13);
    const std::string_view ordersFirst("\x06orders\x05users", 13);
    std::memcpy(input.data(), &expectedCount, sizeof(expectedCount));
    std::memcpy(input.data() + sizeof(expectedCount), usersFirst.data(), usersFirst.size());
    metadata.deserialize(input);
    ASSERT_TRUE(metadata.containsDocument("users"));
    ASSERT_TRUE(metadata.containsDocument("orders"));

    const auto bytes = metadata.serialize();

    std::uint16_t count{};
    std::memcpy(&count, bytes.data(), sizeof(count));
    EXPECT_EQ(count, expectedCount);

    // The document set does not guarantee iteration order.
    const std::string_view payload(bytes.data() + sizeof(count), usersFirst.size());
    EXPECT_TRUE(payload == usersFirst || payload == ordersFirst)
            << "Expected each document name to be preceded by its byte length";
}

TEST_F(MetadataAddDocumentTest, AddsNewDocument) {
    Metadata metadata(rootPath, fileName);

    EXPECT_FALSE(metadata.containsDocument("users"));
    EXPECT_TRUE(metadata.addDocument("users"));
    EXPECT_TRUE(metadata.containsDocument("users"));
}

TEST_F(MetadataAddDocumentTest, RejectsEmptyName) {
    Metadata metadata(rootPath, fileName);

    EXPECT_FALSE(metadata.addDocument(""));
    EXPECT_FALSE(metadata.containsDocument(""));
}

TEST_F(MetadataAddDocumentTest, EnforcesNameLengthLimit) {
    Metadata metadata(rootPath, fileName);
    const std::string maxLengthName(MAX_FILE_NAME_SIZE, 'a');
    const std::string tooLongName(MAX_FILE_NAME_SIZE + 1, 'a');

    EXPECT_TRUE(metadata.addDocument(maxLengthName));
    EXPECT_TRUE(metadata.containsDocument(maxLengthName));
    EXPECT_FALSE(metadata.addDocument(tooLongName));
    EXPECT_FALSE(metadata.containsDocument(tooLongName));
}

TEST_F(MetadataAddDocumentTest, AddingSameNameTwiceStoresOneDocument) {
    Metadata metadata(rootPath, fileName);

    ASSERT_TRUE(metadata.addDocument("users"));
    EXPECT_FALSE(metadata.addDocument("users"));
}

using MetadataFlushTest = MetadataTest;

TEST_F(MetadataFlushTest, PersistChangesInDocuments) {
    Metadata metadata(rootPath, fileName);
    ASSERT_EQ(std::filesystem::file_size(path), 0u);
    ASSERT_TRUE(metadata.addDocument("users"));
    ASSERT_TRUE(metadata.addDocument("orders"));

    metadata.flush();

    std::ifstream input(path, std::ios::binary);
    ASSERT_TRUE(input.is_open());
    std::uint16_t count{};
    ASSERT_TRUE(input.read(reinterpret_cast<char*>(&count), sizeof(count)));
    EXPECT_EQ(count, 2u);

    std::array<char, 13> payload{};
    ASSERT_TRUE(input.read(payload.data(), payload.size()));
    const std::string_view records(payload.data(), payload.size());
    EXPECT_TRUE(records == "\x05users\x06orders" || records == "\x06orders\x05users");
}

using MetadataRemoveDocumentTest = MetadataTest;

TEST_F(MetadataRemoveDocumentTest, RemovesExistingDocument) {
    Metadata metadata(rootPath, fileName);
    ASSERT_TRUE(metadata.addDocument("users"));
    ASSERT_TRUE(metadata.addDocument("orders"));

    EXPECT_TRUE(metadata.removeDocument("users"));
    EXPECT_FALSE(metadata.containsDocument("users"));
    EXPECT_TRUE(metadata.containsDocument("orders"));
    EXPECT_FALSE(metadata.removeDocument("users"));
}

TEST_F(MetadataRemoveDocumentTest, ReturnsFalseForMissingDocument) {
    Metadata metadata(rootPath, fileName);
    ASSERT_TRUE(metadata.addDocument("users"));

    EXPECT_FALSE(metadata.removeDocument("missing"));
    EXPECT_TRUE(metadata.containsDocument("users"));
}

using MetadataRemoveDocumentTest = MetadataTest;

TEST_F(MetadataTest, GetIndex) {
    Metadata metadata(rootPath, fileName);
    ASSERT_TRUE(metadata.addDocument("users"));

    EXPECT_EQ(metadata.getIndex("not_valid"), nullptr);
    EXPECT_NE(metadata.getIndex("users"), nullptr);
}
