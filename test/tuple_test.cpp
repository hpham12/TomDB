//
// Created by Hieu Pham on 9/5/26.
//
#include "tuple.h"
#include "field.h"
#include <iostream>

#include <gtest/gtest.h>

TEST(TupleTest, Deserialize) {
    std::vector<std::unique_ptr<Field>> fields;

    int i = 123456;
    fields.push_back(std::make_unique<Field>(i));

    float f = 123.456;
    fields.push_back(std::make_unique<Field>(f));

    std::string s = "Hello World";
    fields.push_back(std::make_unique<Field>(s));

    size_t totalSize = 0;
    for (auto &field : fields) {
        totalSize += field->size;
    }

    std::stringstream stream;
    stream << totalSize << ' ';

    for (const auto & field : fields) {
        stream << field->serialize();
        stream << ' ';
    }

    stream << "Other Data";

    auto tuple = Tuple::deserialize(stream);
    EXPECT_EQ(tuple->getSize(), totalSize);

    auto intField = tuple->getField(0);
    auto floatField = tuple->getField(1);
    auto stringField = tuple->getField(2);

    EXPECT_EQ(intField->type, FieldType::INTEGER);
    EXPECT_EQ(intField->size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(intField->value.get()), i);

    EXPECT_EQ(floatField->type, FieldType::FLOAT);
    EXPECT_EQ(floatField->size, sizeof(int));
    EXPECT_FLOAT_EQ(*reinterpret_cast<float*>(floatField->value.get()), f);

    EXPECT_EQ(stringField->type, FieldType::STRING);
    EXPECT_EQ(stringField->size, s.length());
    EXPECT_STREQ(stringField->value.get(), s.c_str());
}

// TEST(TupleTest, Serialize) {
//
// }
//
// TEST(TupleTest, GetSize) {
//
// }
//
// TEST(TupleTest, AddField) {
//
// }