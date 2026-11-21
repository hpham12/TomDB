#include "query/insert_json.h"

#include <cstring>
#include <gtest/gtest.h>
#include <string>

TEST(InsertJsonTest, ParsesScalarsInObjectOrder) {
    auto result = parseInsertJson(R"({"name":"Hieu","id":17,"score":-2.5,"age":-3})");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->key, 17u);

    auto name = result->tuple->getField(0);
    EXPECT_EQ(name->name, "name");
    ASSERT_EQ(name->type, STRING);
    EXPECT_EQ(std::string(name->value.get(), name->size), "Hieu");

    auto id = result->tuple->getField(1);
    EXPECT_EQ(id->name, "id");
    ASSERT_EQ(id->type, INTEGER);
    int idValue;
    std::memcpy(&idValue, id->value.get(), sizeof(idValue));
    EXPECT_EQ(idValue, 17);

    auto score = result->tuple->getField(2);
    EXPECT_EQ(score->name, "score");
    ASSERT_EQ(score->type, FLOAT);
    float scoreValue;
    std::memcpy(&scoreValue, score->value.get(), sizeof(scoreValue));
    EXPECT_FLOAT_EQ(scoreValue, -2.5f);

    auto age = result->tuple->getField(3);
    EXPECT_EQ(age->name, "age");
    ASSERT_EQ(age->type, INTEGER);
    int ageValue;
    std::memcpy(&ageValue, age->value.get(), sizeof(ageValue));
    EXPECT_EQ(ageValue, -3);
    EXPECT_THROW(result->tuple->getField(4), std::out_of_range);
}

TEST(InsertJsonTest, DecodesEscapesAndUnicode) {
    auto result = parseInsertJson(R"({"\u0069d":0,"value":"quote: \" slash: \\ \uD83D\uDE03"})");
    ASSERT_TRUE(result);
    auto field = result->tuple->getField(1);
    EXPECT_EQ(field->name, "value");
    EXPECT_EQ(std::string(field->value.get(), field->size), "quote: \" slash: \\ \xF0\x9F\x98\x83");
}

TEST(InsertJsonTest, RejectsInvalidObjects) {
    const char* invalid[] = {
        "{}", R"({"name":"Ada"})", R"({"id":"1"})", R"({"id":-1})",
        R"({"id":1.0})", R"({"id":1e0})", R"({"id":1,"id":2})",
        R"({"id":1,"\u0069d":2})", R"({"id":1,"x":1,"x":2})",
        R"({"id":1,})", R"({"id":01})", R"({"id":1} trailing)",
        R"({"id":1,"x":true})", R"({"id":1,"x":null})",
        R"({"id":1,"x":[]})", R"({"id":1,"x":{}})",
        R"({"id":1,"x":"\u0000"})", R"({"id":1,"x":"\uD800"})",
        R"({"id":2147483648})", R"({"id":18446744073709551616})",
        R"({"id":1,"x":2147483648})", R"({"id":1,"x":1e100})",
        R"({"id":1,"x":1e-100})", R"({"id":1,"x":"\q"})"
    };
    for (const auto* json : invalid) {
        EXPECT_FALSE(parseInsertJson(json)) << json;
    }
}

TEST(InsertJsonTest, RejectsStringsExceedingFieldCapacity) {
    const std::string json = R"({"id":1,"value":")" + std::string(65536, 'a') + R"("})";
    EXPECT_FALSE(parseInsertJson(json));
}
