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

    uint32_t totalSize = 0;
    for (auto &field : fields) {
        totalSize += field->size;
    }

    std::stringstream stream;
    stream.write(reinterpret_cast<const char*>(&totalSize), sizeof(totalSize));

    for (const auto & field : fields) {
        stream << field->serialize();
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

TEST(TupleTest, Serialize) {
    Tuple tuple{};

    int i = 123123123;
    tuple.addField(std::make_unique<Field>(i));
    const std::string serializedIntType {'\x00','\x00','\x00','\x00'};
    const std::string serializedIntSize {'\x04', '\x00'};
    const std::string serializedIntValue {'\xB3', '\xB5', '\x56', '\x07'};
    const std::string serializedIntField = serializedIntType + serializedIntSize + serializedIntValue;

    float f = 123.123;
    tuple.addField(std::make_unique<Field>(f));

    const std::string serializedFloatType {'\x01','\x00','\x00','\x00'};
    const std::string serializedFloatSize {'\x04', '\x00'};
    const std::string serializedFloatValue {'\xFA', '\x3E', '\xF6', '\x42'};
    const std::string serializedFloatField = serializedFloatType + serializedFloatSize + serializedFloatValue;

    std::string s = "Hello World";
    tuple.addField(std::make_unique<Field>(s));
    const std::string serializedStringType {'\x02','\x00','\x00','\x00'};
    const std::string serializedStringSize {'\x0B', '\x00'};
    const std::string serializedStringValue {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
    const std::string serializedStringField = serializedStringType + serializedStringSize + serializedStringValue;

    std::string serializedTupleSize{'\x13', '\x00', '\x00', '\x00'};

    std::string expected =
        serializedTupleSize + serializedIntField + serializedFloatField + serializedStringField;

    EXPECT_EQ(tuple.serialize(), expected);
}

TEST(TupleTest, GetSize) {
    Tuple tuple{};

    tuple.addField(std::make_unique<Field>(123123123));
    tuple.addField(std::make_unique<Field>((float) 123.123));
    tuple.addField(std::make_unique<Field>("Hello World"));

    EXPECT_EQ(tuple.getSize(), 19);
}

TEST(TupleTest, GetField) {
    Tuple tuple{};

    tuple.addField(std::make_unique<Field>(123123123));
    tuple.addField(std::make_unique<Field>((float) 123.123));
    tuple.addField(std::make_unique<Field>("Hello World"));

    auto field1 = tuple.getField(0);
    EXPECT_EQ(field1->type, FieldType::INTEGER);
    EXPECT_EQ(field1->size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(field1->value.get()), 123123123);

    auto field2 = tuple.getField(1);
    EXPECT_EQ(field2->type, FieldType::FLOAT);
    EXPECT_EQ(field2->size, sizeof(float));
    EXPECT_FLOAT_EQ(*reinterpret_cast<float*>(field2->value.get()), 123.123);

    auto field3 = tuple.getField(2);
    EXPECT_EQ(field3->type, FieldType::STRING);
    EXPECT_EQ(field3->size, 11);
    EXPECT_EQ(*reinterpret_cast<std::string*>(field3->value.get()), "Hello World");
}

TEST(TupleTest, GetFieldOutOfRange) {
    Tuple tuple{};
    tuple.addField(std::make_unique<Field>(123123123));
    EXPECT_THROW(tuple.getField(3), std::out_of_range);
}