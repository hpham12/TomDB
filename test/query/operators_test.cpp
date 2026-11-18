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
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    metadata->addDocument("document3");

    CreateDocumentOperator op{"document3", metadata};
    ASSERT_NO_THROW(op.open());
    ASSERT_FALSE(op.next());
    ASSERT_NO_THROW(op.close());
    ASSERT_EQ(op.getOutput(), nullptr);
}

TEST(CreateDocumentOperatorTest, TestE2ELifeCycleWithNonExistingDocument) {
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();

    CreateDocumentOperator op{"document3", metadata};
    ASSERT_NO_THROW(op.open());
    ASSERT_TRUE(op.next());
    ASSERT_TRUE(std::filesystem::exists("/.tomdb/document3_tomdb.idx"));
    ASSERT_TRUE(std::filesystem::exists("/.tomdb/document3_tomdb.dat"));
    ASSERT_NO_THROW(op.close());
    ASSERT_TRUE(metadata->containsDocument("document3"));
    ASSERT_EQ(op.getOutput(), nullptr);
}

TEST(DropDocumentOperatorTest, TestE2ELifeCycleWithNonExistingDocument) {
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();

    DropDocumentOperator op{"document4", metadata};
    ASSERT_NO_THROW(op.open());
    ASSERT_FALSE(op.next());
}

TEST(DropDocumentOperatorTest, TestE2ELifeCycleWithExistingDocument) {
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    metadata->addDocument("document4");

    // 1. Extract the directory portion and create it if missing
    std::filesystem::create_directories("/.tomdb");
    std::ofstream idx("/.tomdb/document4_tomdb.idx");
    std::ofstream data("/.tomdb/document4_tomdb.dat");

    DropDocumentOperator op{"document4", metadata};
    ASSERT_NO_THROW(op.open());
    ASSERT_TRUE(op.next());
    ASSERT_FALSE(std::filesystem::exists("/.tomdb/document4_tomdb.idx"));
    ASSERT_FALSE(std::filesystem::exists("/.tomdb/document4_tomdb.dat"));
}
