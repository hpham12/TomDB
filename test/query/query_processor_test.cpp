#include "query/insert_json.h"
#include "query/query_processor.h"

#include <gtest/gtest.h>

TEST(QueryProcessorInsertTest, ExtractsDocumentAndJson) {
    const std::string query = R"( insert into people VALUE {"id": 42, "name": "Hieu"}; )";
    std::smatch match;

    ASSERT_TRUE(std::regex_match(query, match, INSERT_PATTERN));
    EXPECT_EQ(match[1].str(), "people");
    auto parsed = parseInsertJson(match[2].str());
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->key, 42u);
}

TEST(QueryProcessorInsertTest, RejectsInvalidQueryForms) {
    for (const std::string query : {
             "INSERT INTO people VALUES {\"id\": 1}",
             "INSERT INTO people VALUE {\"id\": 1} trailing",
             "INSERT INTO 1people VALUE {\"id\": 1}",
             "INSERT INTO people VALUE [1]",
         }) {
        EXPECT_FALSE(std::regex_match(query, INSERT_PATTERN)) << query;
    }
}

TEST(QueryProcessorDocumentTest, ExtractsCreateDocumentName) {
    const std::string query = "  create DOCUMENT user_profiles;  ";
    std::smatch match;

    ASSERT_TRUE(std::regex_match(query, match, CREATE_PATTERN));
    EXPECT_EQ(match[1].str(), "user_profiles");
}

TEST(QueryProcessorDocumentTest, ExtractsRemoveDocumentName) {
    const std::string query = "\tReMoVe document Users_2 \n";
    std::smatch match;

    ASSERT_TRUE(std::regex_match(query, match, REMOVE_PATTERN));
    EXPECT_EQ(match[1].str(), "Users_2");
}

TEST(QueryProcessorDocumentTest, RejectsInvalidDocumentCommands) {
    for (const std::string query : {
             "CREATE DOCUMENT",
             "REMOVE DOCUMENT",
             "CREATE DOCUMENT 9users",
             "REMOVE DOCUMENT users-extra",
             "CREATE DOCUMENT users extra",
             "REMOVE DOCUMENT users; extra",
         }) {
        EXPECT_FALSE(std::regex_match(query, CREATE_PATTERN)) << query;
        EXPECT_FALSE(std::regex_match(query, REMOVE_PATTERN)) << query;
    }
}

TEST(QueryProcessorSelectTest, ExtractsWildcardAndLookup) {
    const std::string query = "  select * FROM users WHERE id = 42;  ";
    std::smatch match;

    ASSERT_TRUE(std::regex_match(query, match, SELECT_PATTERN));
    EXPECT_EQ(match[1].str(), "*");
    EXPECT_EQ(match[2].str(), "users");
    EXPECT_EQ(match[3].str(), "42");
}

TEST(QueryProcessorSelectTest, ExtractsNamedFieldsAndLookup) {
    const std::string query = "SELECT name, age FROM users WHERE ID = 1";
    std::smatch match;

    ASSERT_TRUE(std::regex_match(query, match, SELECT_PATTERN));
    EXPECT_EQ(match[1].str(), "name, age");
    EXPECT_EQ(match[2].str(), "users");
    EXPECT_EQ(match[3].str(), "1");
}

TEST(QueryProcessorSelectTest, RejectsWildcardMixedWithNames) {
    for (const std::string query : {
             "SELECT *, name FROM users WHERE ID = 1",
             "SELECT name, * FROM users WHERE ID = 1",
             "SELECT ** FROM users WHERE ID = 1",
             "SELECT * FROM users WHERE ID = -1",
         }) {
        EXPECT_FALSE(std::regex_match(query, SELECT_PATTERN)) << query;
    }
}
