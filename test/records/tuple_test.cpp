//
// Created by Hieu Pham on 9/5/26.
//
#include "records/tuple.h"
#include "records/field.h"
#include <iostream>

#include <gtest/gtest.h>

TEST(TupleTest, Deserialize) {
    std::vector<std::unique_ptr<Field>> fields;

    int i = 123456;
    fields.push_back(std::make_unique<Field>(i, "name"));

    float f = 123.456;
    fields.push_back(std::make_unique<Field>(f, "name"));

    std::string s = "Hello World";
    fields.push_back(std::make_unique<Field>(s, "Hello World"));

    uint32_t totalSize = sizeof(uint32_t);
    for (auto &field : fields) {
        totalSize += field->getSize();
    }

    std::stringstream stream;
    stream.write(reinterpret_cast<const char*>(&totalSize), sizeof(totalSize));

    for (const auto & field : fields) {
        stream << field->serialize();
    }

    stream << "Other Data";

    auto tuple = Tuple::deserialize(stream);
    EXPECT_EQ(tuple->getSize(), 63);

    auto intField = tuple->getField(0);
    auto floatField = tuple->getField(1);
    auto stringField = tuple->getField(2);

    EXPECT_EQ(intField->name, "name");
    EXPECT_EQ(intField->type, FieldType::INTEGER);
    EXPECT_EQ(intField->size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(intField->value.get()), i);

    EXPECT_EQ(floatField->name, "name");
    EXPECT_EQ(floatField->type, FieldType::FLOAT);
    EXPECT_EQ(floatField->size, sizeof(int));
    EXPECT_FLOAT_EQ(*reinterpret_cast<float*>(floatField->value.get()), f);

    EXPECT_EQ(stringField->name, "Hello World");
    EXPECT_EQ(stringField->type, FieldType::STRING);
    EXPECT_EQ(stringField->size, s.length());
    EXPECT_STREQ(stringField->value.get(), s.c_str());
}

TEST(TupleTest, Serialize) {
    Tuple tuple{};

    int i = 123123123;
    tuple.addField(std::make_unique<Field>(i, "name"));
    const std::string serializedIntType {'\x00','\x00','\x00','\x00'};
    const std::string serializedIntSize {'\x04', '\x00'};
    const std::string serializedIntValue {'\xB3', '\xB5', '\x56', '\x07'};
    const std::string serializedIntField = serializedIntType + serializedIntSize + serializedIntValue + std::string{"\x04name", 5};

    float f = 123.123;
    tuple.addField(std::make_unique<Field>(f, "name"));

    const std::string serializedFloatType {'\x01','\x00','\x00','\x00'};
    const std::string serializedFloatSize {'\x04', '\x00'};
    const std::string serializedFloatValue {'\xFA', '\x3E', '\xF6', '\x42'};
    const std::string serializedFloatField = serializedFloatType + serializedFloatSize + serializedFloatValue + std::string{"\x04name", 5};

    std::string s = "Hello World";
    tuple.addField(std::make_unique<Field>(s, "Hello World"));
    const std::string serializedStringType {'\x02','\x00','\x00','\x00'};
    const std::string serializedStringSize {'\x0B', '\x00'};
    const std::string serializedStringValue {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
    const std::string serializedStringField = serializedStringType + serializedStringSize + serializedStringValue + std::string{"\x0bHello World", 12};

    std::string serializedTupleSize{'\x3f', '\x00', '\x00', '\x00'};

    std::string expected =
        serializedTupleSize + serializedIntField + serializedFloatField + serializedStringField;

    EXPECT_EQ(tuple.serialize(), expected);
}

TEST(TupleTest, GetSize) {
    Tuple tuple{};

    tuple.addField(std::make_unique<Field>(123123123, "name"));
    tuple.addField(std::make_unique<Field>((float) 123.123, "name"));
    tuple.addField(std::make_unique<Field>("Hello World", "Hello World"));

    // 59: total serialized field sizes, including names
    // 4: tuple size
    EXPECT_EQ(tuple.getSize(), 59 + 4);
}

TEST(TupleTest, GetField) {
    Tuple tuple{};

    tuple.addField(std::make_unique<Field>(123123123, "name"));
    tuple.addField(std::make_unique<Field>((float) 123.123, "name"));
    tuple.addField(std::make_unique<Field>("Hello World", "Hello World"));

    auto field1 = tuple.getField(0);
    EXPECT_EQ(field1->name, "name");
    EXPECT_EQ(field1->type, FieldType::INTEGER);
    EXPECT_EQ(field1->size, sizeof(int));
    EXPECT_EQ(*reinterpret_cast<int*>(field1->value.get()), 123123123);

    auto field2 = tuple.getField(1);
    EXPECT_EQ(field2->name, "name");
    EXPECT_EQ(field2->type, FieldType::FLOAT);
    EXPECT_EQ(field2->size, sizeof(float));
    EXPECT_FLOAT_EQ(*reinterpret_cast<float*>(field2->value.get()), 123.123);

    auto field3 = tuple.getField(2);
    EXPECT_EQ(field3->name, "Hello World");
    EXPECT_EQ(field3->type, FieldType::STRING);
    EXPECT_EQ(field3->size, 11);
    EXPECT_STREQ(field3->value.get(), "Hello World");
}

TEST(TupleTest, GetFieldOutOfRange) {
    Tuple tuple{};
    tuple.addField(std::make_unique<Field>(123123123, "name"));
    EXPECT_THROW(tuple.getField(3), std::out_of_range);
}

TEST(TupleTest, DeserializeConsumesExactlyOneTuple) {
    Tuple tuple1{};
    Tuple tuple2{};

    tuple1.addField(std::make_unique<Field>(-42, "name"));
    tuple2.addField(std::make_unique<Field>(std::string("second"), "second"));

    std::stringstream stream(tuple1.serialize() + tuple2.serialize());

    auto deserialized1 = Tuple::deserialize(stream);
    EXPECT_EQ(deserialized1->serialize(), tuple1.serialize());
    EXPECT_EQ(deserialized1->getField(0)->name, "name");

    auto deserialized2 = Tuple::deserialize(stream);
    EXPECT_EQ(deserialized2->serialize(), tuple2.serialize());
    EXPECT_EQ(deserialized2->getField(0)->name, "second");
    EXPECT_EQ(stream.peek(), std::char_traits<char>::eof());
}

TEST(TupleTest, EmptyTupleRoundTrip) {
    Tuple tuple{};

    std::stringstream stream(tuple.serialize());
    auto deserialized = Tuple::deserialize(stream);

    EXPECT_EQ(deserialized->getSize(), sizeof(uint32_t));
    EXPECT_EQ(deserialized->serialize(), tuple.serialize());
    EXPECT_THROW(deserialized->getField(0), std::out_of_range);
}

TEST(TupleTest, ReturnedFieldIsAnIndependentCopy) {
    Tuple tuple{};
    tuple.addField(std::make_unique<Field>(std::string("original"), "original"));

    auto field = tuple.getField(0);
    EXPECT_EQ(field->name, "original");
    field->value[0] = 'X';

    EXPECT_STREQ(tuple.getField(0)->value.get(), "original");
    EXPECT_EQ(tuple.getField(0)->name, "original");
}

TEST(TupleTest, CloneEmptyTuple) {
    Tuple original;
    const Tuple& source = original;
    auto copy = source.clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_NE(copy.get(), &original);
    EXPECT_EQ(copy->getSize(), sizeof(uint32_t));
    EXPECT_EQ(copy->serialize(), original.serialize());
    EXPECT_THROW(copy->getField(0), std::out_of_range);
}

TEST(TupleTest, ClonePreservesFieldsAndOwnsIndependentCopy) {
    auto original = std::make_unique<Tuple>();
    original->addField(std::make_unique<Field>(-42, "name"));
    original->addField(std::make_unique<Field>(123.125f, "name"));
    original->addField(std::make_unique<Field>("Hello World", "Hello World"));
    const auto expected = original->serialize();
    const auto expectedSize = original->getSize();

    // Cloning through a const reference must preserve field types, order and values.
    const Tuple& source = *original;
    auto copy = source.clone();
    ASSERT_NE(copy, nullptr);
    EXPECT_NE(copy.get(), original.get());
    EXPECT_EQ(copy->getSize(), expectedSize);
    EXPECT_EQ(copy->serialize(), expected);
    EXPECT_EQ(original->serialize(), expected);

    // Adding to either tuple must not change the other's field list.
    original->addField(std::make_unique<Field>(99, "name"));
    EXPECT_EQ(copy->serialize(), expected);
    EXPECT_THROW(copy->getField(3), std::out_of_range);
    const auto extendedOriginal = original->serialize();
    copy->addField(std::make_unique<Field>("copy only", "copy only"));
    EXPECT_EQ(original->serialize(), extendedOriginal);
    const auto extendedCopy = copy->serialize();

    // Destroying the original must leave every cloned field usable. getField()
    // itself clones, so comparing its returned pointers would not test ownership.
    original.reset();
    EXPECT_EQ(copy->serialize(), extendedCopy);
    EXPECT_EQ(copy->getField(0)->name, "name");
    EXPECT_EQ(copy->getField(1)->name, "name");
    EXPECT_EQ(copy->getField(2)->name, "Hello World");
    EXPECT_EQ(copy->getField(3)->name, "copy only");
    EXPECT_EQ(copy->getField(0)->serialize(), Field(-42, "name").serialize());
    EXPECT_EQ(copy->getField(1)->serialize(), Field(123.125f, "name").serialize());
    EXPECT_EQ(copy->getField(2)->serialize(), Field("Hello World", "Hello World").serialize());
    EXPECT_EQ(copy->getField(3)->serialize(), Field("copy only", "copy only").serialize());
    EXPECT_THROW(copy->getField(4), std::out_of_range);
}
