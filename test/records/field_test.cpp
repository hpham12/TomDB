//
// Created by Hieu Pham on 9/1/26.
//

#include "records/field.h"

#include <gtest/gtest.h>

TEST(FieldTest, IntFieldInitialization) {
    const int i = 123456;
    const Field field(i, "num");
    EXPECT_EQ(field.name, "num");
    EXPECT_EQ(field.type, FieldType::INTEGER);
    EXPECT_EQ(field.size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(field.value.get()), i);
}

TEST(FieldTest, IntFieldSerialization) {
    const int i = 123123123;
    const Field field(i, "num");
    EXPECT_EQ(field.name, "num");
    const std::string serializedType {'\x00','\x00','\x00','\x00'};
    const std::string serializedSize {'\x04', '\x00'};
    const std::string serializedValue {'\xB3', '\xB5', '\x56', '\x07'};
    const std::string expected = serializedType + serializedSize + serializedValue + std::string{"\x03num", 4};

    EXPECT_EQ(field.serialize(), expected);
}

TEST(FieldTest, IntFieldDeserialization) {
    const int i = 123123123;
    const Field field(i, "num");
    std::stringstream stream;
    stream << field.serialize();

    auto deserialized = Field::deserialize(stream);
    EXPECT_EQ(deserialized->name, field.name);
    EXPECT_EQ(deserialized->type, FieldType::INTEGER);
    EXPECT_EQ(deserialized->size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(deserialized->value.get()), i);
}

TEST(FieldTest, CloneIntField) {
    const int i = 123456;
    const Field field(i, "num");
    auto cloned = field.clone();
    EXPECT_EQ(cloned->name, field.name);
    EXPECT_EQ(cloned->type, FieldType::INTEGER);
    EXPECT_EQ(cloned->size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(cloned->value.get()), i);
}

TEST(FieldTest, IntFieldGetSize) {
    const int i = 123456;
    const Field field(i, "num");
    EXPECT_EQ(field.name, "num");
    EXPECT_EQ(field.getSize(), 4 + 2 + 4 + 1 + 3);
}

TEST(FieldTest, FloatFieldInitialization) {
    const float f = 123.123;
    const Field field(f, "num");
    EXPECT_EQ(field.name, "num");
    EXPECT_EQ(field.type, FieldType::FLOAT);
    EXPECT_EQ(field.size, sizeof(float));
    EXPECT_FLOAT_EQ(*reinterpret_cast<float*>(field.value.get()), f);
}

TEST(FieldTest, FloatFieldSerialization) {
    const float f = 123.123;
    const Field field(f, "num");
    EXPECT_EQ(field.name, "num");

    const std::string serializedType {'\x01','\x00','\x00','\x00'};
    const std::string serializedSize {'\x04', '\x00'};
    const std::string serializedValue {'\xFA', '\x3E', '\xF6', '\x42'};
    const std::string expected = serializedType + serializedSize + serializedValue + std::string{"\x03num", 4};

    EXPECT_EQ(field.serialize(), expected);
}

TEST(FieldTest, FloatFieldDeserialization) {
    const float f = 123.123;
    const Field field(f, "num");
    std::stringstream stream;
    stream << field.serialize();

    auto deserialized = Field::deserialize(stream);
    EXPECT_EQ(deserialized->name, field.name);
    EXPECT_EQ(deserialized->type, FieldType::FLOAT);
    EXPECT_EQ(deserialized->size, sizeof(float));
    EXPECT_FLOAT_EQ(*reinterpret_cast<float*>(deserialized->value.get()), f);
}

TEST(FieldTest, CloneFloatField) {
    const float f = 123.456;
    const Field field(f, "num");
    auto cloned = field.clone();
    EXPECT_EQ(cloned->name, field.name);
    EXPECT_EQ(cloned->type, FieldType::FLOAT);
    EXPECT_EQ(cloned->size, sizeof(float));
    EXPECT_EQ(*reinterpret_cast<float*>(cloned->value.get()), f);
}

TEST(FieldTest, FloatFieldGetSize) {
    const float f = 123.456;
    const Field field(f, "num");
    EXPECT_EQ(field.name, "num");
    EXPECT_EQ(field.getSize(), 4 + 2 + 4 + 1 + 3);
}

TEST(FieldTest, StringFieldInitialization) {
    const std::string s = "Hello World";
    const Field field(s, "text");
    EXPECT_EQ(field.name, "text");
    EXPECT_EQ(field.type, FieldType::STRING);
    EXPECT_EQ(field.size, s.length());
    EXPECT_STREQ(field.value.get(), s.c_str());
}

TEST(FieldTest, StringFieldSerialization) {
    const std::string s = "Hello World";
    const Field field(s, "text");
    EXPECT_EQ(field.name, "text");
    const std::string serializedType {'\x02','\x00','\x00','\x00'};
    const std::string serializedSize {'\x0B', '\x00'};
    const std::string serializedValue {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
    const std::string expected = serializedType + serializedSize + serializedValue + std::string{"\x04text", 5};
    EXPECT_EQ(field.serialize(), expected);
}

TEST(FieldTest, StringFieldDeserialization) {
    const std::string s = "Hello World";
    const Field field(s, "text");
    std::stringstream stream;
    stream << field.serialize();

    auto deserialized = Field::deserialize(stream);
    EXPECT_EQ(deserialized->name, field.name);
    EXPECT_EQ(deserialized->type, FieldType::STRING);
    EXPECT_EQ(deserialized->size, s.length());
    EXPECT_STREQ(deserialized->value.get(), s.c_str());
}

TEST(FieldTest, CloneStringField) {
    const std::string s = "Hello World";
    const Field field(s, "text");
    auto cloned = field.clone();
    EXPECT_EQ(cloned->name, field.name);
    EXPECT_EQ(cloned->type, FieldType::STRING);
    EXPECT_EQ(cloned->size, s.length());
    EXPECT_STREQ(cloned->value.get(), "Hello World");
}

TEST(FieldTest, StringFieldGetSize) {
    const std::string s = "Hello World";
    const Field field(s, "text");
    EXPECT_EQ(field.name, "text");
    EXPECT_EQ(field.getSize(), s.length() + 2 + 4 + 1 + 4);
}
