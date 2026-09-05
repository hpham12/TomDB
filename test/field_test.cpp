//
// Created by Hieu Pham on 9/1/26.
//

#include "field.h"
#include <iostream>

#include <gtest/gtest.h>

TEST(FieldTest, IntFieldInitialization) {
    const int i = 123456;
    const Field field(i);
    EXPECT_EQ(field.type, FieldType::INTEGER);
    EXPECT_EQ(field.size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(field.value.get()), i);
}

TEST(FieldTest, IntFieldSerialization) {
    const int i = 123123123;
    const Field field(i);
    const std::string serializedType {'\x00','\x00','\x00','\x00'};
    const std::string serializedSize {'\x04', '\x00'};
    const std::string serializedValue {'\xB3', '\xB5', '\x56', '\x07'};
    const std::string expected = serializedType + serializedSize + serializedValue;

    EXPECT_EQ(field.serialize(), expected);
}

TEST(FieldTest, IntFieldDeserialization) {
    const int i = 123123123;
    const Field field(i);
    std::stringstream stream;
    stream << field.serialize();

    auto deserialized = Field::deserialize(stream);
    EXPECT_EQ(deserialized->type, FieldType::INTEGER);
    EXPECT_EQ(deserialized->size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(deserialized->value.get()), i);
}

TEST(FieldTest, FloatFieldInitialization) {
    const float f = 123.123;
    const Field field(f);
    EXPECT_EQ(field.type, FieldType::FLOAT);
    EXPECT_EQ(field.size, sizeof(float));
    EXPECT_FLOAT_EQ(*reinterpret_cast<float*>(field.value.get()), f);
}

TEST(FieldTest, FloatFieldSerialization) {
    const float f = 123.123;
    const Field field(f);

    const std::string serializedType {'\x01','\x00','\x00','\x00'};
    const std::string serializedSize {'\x04', '\x00'};
    const std::string serializedValue {'\xFA', '\x3E', '\xF6', '\x42'};
    const std::string expected = serializedType + serializedSize + serializedValue;

    EXPECT_EQ(field.serialize(), expected);
}

TEST(FieldTest, FloatFieldDeserialization) {
    const float f = 123.123;
    const Field field(f);
    std::stringstream stream;
    stream << field.serialize();

    auto deserialized = Field::deserialize(stream);
    EXPECT_EQ(deserialized->type, FieldType::FLOAT);
    EXPECT_EQ(deserialized->size, sizeof(float));
    EXPECT_FLOAT_EQ(*reinterpret_cast<float*>(deserialized->value.get()), f);
}

TEST(FieldTest, StringFieldInitialization) {
    const std::string s = "Hello World";
    const Field field(s);
    EXPECT_EQ(field.type, FieldType::STRING);
    EXPECT_EQ(field.size, s.length());
    EXPECT_STREQ(field.value.get(), s.c_str());
}

TEST(FieldTest, StringFieldSerialization) {
    const std::string s = "Hello World";
    const Field field(s);
    const std::string serializedType {'\x02','\x00','\x00','\x00'};
    const std::string serializedSize {'\x0B', '\x00'};
    const std::string serializedValue {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
    const std::string expected = serializedType + serializedSize + serializedValue;
    EXPECT_EQ(field.serialize(), expected);
}

TEST(FieldTest, StringFieldDeserialization) {
    const std::string s = "Hello World";
    const Field field(s);
    std::stringstream stream;
    stream << field.serialize();

    auto deserialized = Field::deserialize(stream);
    EXPECT_EQ(deserialized->type, FieldType::STRING);
    EXPECT_EQ(deserialized->size, s.length());
    EXPECT_STREQ(deserialized->value.get(), s.c_str());
}

TEST(FieldTest, Clone) {
    const int i = 123456;
    const Field field(i);
    auto cloned = field.clone();
    EXPECT_EQ(cloned->type, FieldType::INTEGER);
    EXPECT_EQ(cloned->size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(cloned->value.get()), i);
}
