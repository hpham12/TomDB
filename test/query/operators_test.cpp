//
// Created by Hieu Pham on 11/16/26.
//

#include "query/operators.h"

#include "../test_utils.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

TEST(InsertOperatorTest, TestOpenWithInvalidDocument) {
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    InsertOperator op{nullptr, 2, "invalidDocument", metadata};

    EXPECT_FALSE(op.isIndexValid());
}

TEST(InsertOperatorTest, TestE2ELifeCycle) {
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    metadata->addDocument("document1");
    std::unique_ptr<Tuple> tuple = std::make_unique<Tuple>();

    tuple->addField(std::make_unique<Field>(123123123, "name"));
    tuple->addField(std::make_unique<Field>((float) 123.123, "name"));
    tuple->addField(std::make_unique<Field>("Hello World", "Hello World"));
    InsertOperator op{tuple->clone(), 1, "document1", metadata};

    op.open();
    ASSERT_TRUE(op.isIndexValid());
    ASSERT_TRUE(op.next());
    ASSERT_EQ(op.getOutput(), nullptr);
    ASSERT_FALSE(op.next());
    ASSERT_NO_THROW(op.close());
}

TEST(SelectOperatorTest, TestOpenWithInvalidDocument) {
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    SelectOperator op{1, "invalidDocument", {}, false, metadata};

    EXPECT_FALSE(op.isIndexValid());
}

TEST(SelectOperatorTest, TestE2ELifeCycle) {
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    metadata->addDocument("document2");
    std::unique_ptr<Tuple> tuple = std::make_unique<Tuple>();

    tuple->addField(std::make_unique<Field>(123123123, "name1"));
    tuple->addField(std::make_unique<Field>((float) 123.123, "name2"));
    tuple->addField(std::make_unique<Field>("Hello World", "name3"));

    SelectOperator op{1, "document2", {"name1", "name2", "name3"}, false, metadata};

    op.open();
    op.index->insert(1, tuple->clone());

    ASSERT_TRUE(op.isIndexValid());
    ASSERT_TRUE(op.next());
    auto output = op.getOutput();
    auto field1 = output->getField(0);
    EXPECT_EQ(field1->name, "name1");
    EXPECT_EQ(field1->type, FieldType::INTEGER);
    EXPECT_EQ(field1->size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(field1->value.get()), 123123123);

    auto field2 = output->getField(1);
    EXPECT_EQ(field2->name, "name2");
    EXPECT_EQ(field2->type, FieldType::FLOAT);
    EXPECT_EQ(field2->size, sizeof(float));
    EXPECT_FLOAT_EQ(*reinterpret_cast<float*>(field2->value.get()), 123.123);

    auto field3 = output->getField(2);
    EXPECT_EQ(field3->name, "name3");
    EXPECT_EQ(field3->type, FieldType::STRING);
    EXPECT_EQ(field3->size, 11);
    EXPECT_STREQ(field3->value.get(), "Hello World");

    ASSERT_FALSE(op.next());
    ASSERT_NO_THROW(op.close());
}

TEST(CreateDocumentOperatorTest, TestE2ELifeCycleWithExistingDocument) {
    const auto metadataPath = generateRandomFilePath();
    ASSERT_FALSE(std::filesystem::exists(metadataPath));
    auto metadata = std::make_shared<Metadata>(metadataPath);
    metadata->addDocument("document3");

    CreateDocumentOperator op{"document3", metadata, "./tmp/"};
    ASSERT_NO_THROW(op.open());
    ASSERT_FALSE(op.next());
    ASSERT_NO_THROW(op.close());
    ASSERT_EQ(op.getOutput(), nullptr);

    metadata->filestream.close();
    std::filesystem::remove(metadataPath);
}

TEST(CreateDocumentOperatorTest, TestE2ELifeCycleWithNonExistingDocument) {
    const auto metadataPath = generateRandomFilePath();
    ASSERT_FALSE(std::filesystem::exists(metadataPath));
    const auto document = std::filesystem::path(metadataPath).stem().string();
    const auto indexPath = "./tmp/" + document + "_tomdb.idx";
    const auto dataPath = "./tmp/" + document + "_tomdb.dat";
    std::filesystem::create_directories("./tmp/");
    ASSERT_FALSE(std::filesystem::exists(indexPath));
    ASSERT_FALSE(std::filesystem::exists(dataPath));
    auto metadata = std::make_shared<Metadata>(metadataPath);

    CreateDocumentOperator op{document, metadata, "./tmp/"};
    ASSERT_NO_THROW(op.open());
    ASSERT_TRUE(op.next());
    ASSERT_TRUE(std::filesystem::exists(indexPath));
    ASSERT_TRUE(std::filesystem::exists(dataPath));
    ASSERT_NO_THROW(op.close());
    ASSERT_TRUE(metadata->containsDocument(document));
    ASSERT_EQ(op.getOutput(), nullptr);

    metadata->filestream.close();
    std::filesystem::remove(metadataPath);
    std::filesystem::remove(indexPath);
    std::filesystem::remove(dataPath);
}

TEST(DropDocumentOperatorTest, TestE2ELifeCycleWithNonExistingDocument) {
    const auto metadataPath = generateRandomFilePath();
    ASSERT_FALSE(std::filesystem::exists(metadataPath));
    auto metadata = std::make_shared<Metadata>(metadataPath);

    DropDocumentOperator op{"document4", metadata, "./tmp/"};
    ASSERT_NO_THROW(op.open());
    ASSERT_FALSE(op.next());

    metadata->filestream.close();
    std::filesystem::remove(metadataPath);
}

TEST(DropDocumentOperatorTest, TestE2ELifeCycleWithExistingDocument) {
    const auto metadataPath = generateRandomFilePath();
    ASSERT_FALSE(std::filesystem::exists(metadataPath));
    const auto document = std::filesystem::path(metadataPath).stem().string();
    const auto indexPath = "./tmp/" + document + "_tomdb.idx";
    const auto dataPath = "./tmp/" + document + "_tomdb.dat";
    std::filesystem::create_directories("./tmp/");
    ASSERT_FALSE(std::filesystem::exists(indexPath));
    ASSERT_FALSE(std::filesystem::exists(dataPath));
    auto metadata = std::make_shared<Metadata>(metadataPath);
    metadata->addDocument(document);

    std::ofstream indexFile(indexPath);
    std::ofstream dataFile(dataPath);
    ASSERT_TRUE(indexFile.is_open());
    ASSERT_TRUE(dataFile.is_open());
    indexFile.close();
    dataFile.close();

    DropDocumentOperator op{document, metadata, "./tmp/"};
    ASSERT_NO_THROW(op.open());
    ASSERT_TRUE(op.next());
    ASSERT_FALSE(std::filesystem::exists(indexPath));
    ASSERT_FALSE(std::filesystem::exists(dataPath));

    metadata->filestream.close();
    std::filesystem::remove(metadataPath);
}

TEST(PrintOperatorTest, TestGetOutput) {
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    metadata->addDocument("document6");
    std::unique_ptr<Tuple> tuple = std::make_unique<Tuple>();

    tuple->addField(std::make_unique<Field>(123, "name1"));
    tuple->addField(std::make_unique<Field>((float) 123.123, "name2"));
    tuple->addField(std::make_unique<Field>("Hello World", "name3"));

    std::unique_ptr<SelectOperator> selectOp = std::make_unique<SelectOperator>(
        1,
        "document6",
        std::unordered_set<std::string>{"name1", "name2", "name3"},
        false,
        metadata);

    selectOp->open();
    selectOp->index->insert(1, tuple->clone());

    PrintOperator printOp(std::move(selectOp));

    ASSERT_NO_THROW(printOp.getOutput());
}
