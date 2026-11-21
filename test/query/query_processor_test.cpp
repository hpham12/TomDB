#include "query/query_processor.h"

#include <gtest/gtest.h>

#include "gmock/gmock-matchers.h"

TEST(QueryProcessorTest, ParseSelectQueryWithoutDisplay) {
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();

    auto op = QueryProcessor::parse(
        "SELECT name, age FROM users WHERE id = 1",
        metadata,
        false
    );

    auto *selectOp = static_cast<SelectOperator*>(op.get());

    ASSERT_EQ(selectOp->document, "users");
    ASSERT_TRUE(selectOp->fieldNames.contains("name"));
    ASSERT_TRUE(selectOp->fieldNames.contains("age"));

    // Select all
    op = QueryProcessor::parse(
        "SELECT * FROM users WHERE id = 1",
        metadata,
        false
    );

    selectOp = static_cast<SelectOperator*>(op.get());

    ASSERT_EQ(selectOp->document, "users");
    ASSERT_TRUE(selectOp->selectAll);
}

TEST(QueryProcessorTest, ParseSelectQueryWithDisplay) {
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();

    auto op = QueryProcessor::parse(
        "SELECT name, age FROM users WHERE id = 1",
        metadata,
        false
    );

    ASSERT_NO_THROW(static_cast<PrintOperator*>(op.get()));
}

TEST(QueryProcessorTest, ParseInsertQuery) {
    std::shared_ptr<Metadata> metadata;

    auto op = QueryProcessor::parse(
        R"(INSERT INTO users VALUE {"id": 1, "name": "Tom"})",
        metadata,
        false
    );

    auto *insertOp = static_cast<InsertOperator*>(op.get());
    ASSERT_NE(insertOp, nullptr);
    ASSERT_EQ(insertOp->document, "users");
    ASSERT_EQ(insertOp->keyToInsert, 1u);
    ASSERT_NE(insertOp->tupleToInsert, nullptr);
    ASSERT_EQ(insertOp->tupleToInsert->getFieldCount(), 2u);
    auto name = insertOp->tupleToInsert->getField(1);
    ASSERT_EQ(name->name, "name");
    ASSERT_EQ(name->type, STRING);
    ASSERT_STREQ(name->value.get(), "Tom");
}

TEST(QueryProcessorTest, ParseCreateDocument) {
    std::shared_ptr<Metadata> metadata;

    auto op = QueryProcessor::parse("CREATE DOCUMENT users", metadata, false);

    auto *createOp = dynamic_cast<CreateDocumentOperator*>(op.get());
    ASSERT_NE(createOp, nullptr);
    ASSERT_EQ(createOp->document, "users");
}

TEST(QueryProcessorTest, ParseDropDocument) {
    std::shared_ptr<Metadata> metadata;

    auto op = QueryProcessor::parse("REMOVE DOCUMENT users", metadata, false);

    auto *dropOp = dynamic_cast<DropDocumentOperator*>(op.get());
    ASSERT_NE(dropOp, nullptr);
    ASSERT_EQ(dropOp->document, "users");
}
