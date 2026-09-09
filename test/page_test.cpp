//
// Created by Hieu Pham on 9/7/26.
//
#include <gtest/gtest.h>
#include "page.h"
#include "tuple.h"
#include <algorithm>

std::unique_ptr<Tuple> createLargeTestTuple();
std::unique_ptr<Tuple> createSmallTestTuple();


TEST(PageTest, AddTuple) {
    Page page;
    auto tuple = std::make_unique<Tuple>();

    int i = 123456;
    tuple->addField(std::make_unique<Field>(i));

    float f = 123.456;
    tuple->addField(std::make_unique<Field>(f));

    std::string s = "Hello World";
    tuple->addField(std::make_unique<Field>(s));

    char *reason = nullptr;
    auto tupleSize = tuple->getSize();

    size_t index = page.addTuple(std::move(tuple), reason);

    ASSERT_EQ(index, 0);
    ASSERT_EQ(reason, nullptr);

    Slot* slots = reinterpret_cast<Slot*>(page.pageData.get());
    auto offset = slots[index].offset;
    std::stringstream stream;

    stream.write(page.pageData.get() + offset, tupleSize);

    auto deserialized = Tuple::deserialize(stream);
    EXPECT_EQ(deserialized->getSize(), 41);

    auto intField = deserialized->getField(0);
    auto floatField = deserialized->getField(1);
    auto stringField = deserialized->getField(2);

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

TEST(PageTest, AddTupleFailsWhenRunningOutOfSpace) {
    Page page;
    std::unique_ptr<char[]> failReason = std::make_unique<char[]>(512);
    auto tupleSize = createLargeTestTuple()->getSize();
    auto maxTuplesCanBeAdded = (PAGE_SIZE - page.metadata_size)/tupleSize;
    for (size_t i = 0; i < maxTuplesCanBeAdded; i++) {
        size_t insertedIndex = page.addTuple(createLargeTestTuple(), failReason.get());
        ASSERT_EQ(insertedIndex, i);
    }

    // This one goes beyond the max slot limit
    size_t insertedIndex = page.addTuple(createLargeTestTuple(), failReason.get());
    ASSERT_EQ(insertedIndex, INVALID_VALUE);
    ASSERT_STREQ("Tuple cannot fit in page", failReason.get());
}

TEST(PageTest, AddTupleFailsRunningOutOfSlots) {
    Page page;
    std::unique_ptr<char[]> failReason = std::make_unique<char[]>(512);
    for (size_t i = 0; i < MAX_SLOTS; i++) {
        size_t insertedIndex = page.addTuple(createSmallTestTuple(), failReason.get());
        ASSERT_EQ(insertedIndex, i);
    }

    // This one goes beyond the max slot limit
    size_t insertedIndex = page.addTuple(createSmallTestTuple(), failReason.get());
    ASSERT_EQ(insertedIndex, INVALID_VALUE);
    ASSERT_STREQ("No empty slot is large enough to hold tuple", failReason.get());
}

/**
 * Create a large test tuple that consists of int, float, and string.
 */
std::unique_ptr<Tuple> createLargeTestTuple() {
    Page page;
    auto tuple = std::make_unique<Tuple>();

    int i = 123456;
    tuple->addField(std::make_unique<Field>(i));
    tuple->addField(std::make_unique<Field>(i));

    float f = 123.456;
    tuple->addField(std::make_unique<Field>(f));
    tuple->addField(std::make_unique<Field>(f));

    std::string s = "Hello World";
    tuple->addField(std::make_unique<Field>(s));
    tuple->addField(std::make_unique<Field>(s));

    return tuple;
}

/**
 * Create a small test tuple that consists of int only
 */
std::unique_ptr<Tuple> createSmallTestTuple() {
    Page page;
    auto tuple = std::make_unique<Tuple>();

    int i = 123456;
    tuple->addField(std::make_unique<Field>(i));

    return tuple;
}