//
// Created by Hieu Pham on 9/7/26.
//
#include <gtest/gtest.h>
#include "records/page.h"
#include "records/tuple.h"
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

TEST(PageTest, DeleteTuple) {
    Page page;
    std::unique_ptr<char[]> failReason = std::make_unique<char[]>(512);
    for (size_t i = 0; i < 10; i++) {
        page.addTuple(createSmallTestTuple(), failReason.get());
    }

    for (int8_t i = 9; i >= 0; i--) {
        bool deleted = page.deleteTuple(i);
        ASSERT_TRUE(deleted);
    }

    // verify that all slots are empty
    Slot* slots = reinterpret_cast<Slot*>(page.pageData.get());
    for (size_t i = 0; i < MAX_SLOTS; i++) {
        ASSERT_TRUE(slots[i].empty);
    }
}

TEST(PageTest, DeleteTupleFailsWhenIndexOutOfBound) {
    Page page;
    ASSERT_FALSE(page.deleteTuple(MAX_SLOTS + 1));
}

TEST(PageTest, DeleteTupleFailsWhenSlotIsEmpty) {
    Page page;
    ASSERT_FALSE(page.deleteTuple(0));
}

TEST(PageTest, TupleCanExactlyFillRemainingSpace) {
    Page page;
    auto tuple = std::make_unique<Tuple>();

    const size_t overhead = sizeof(uint32_t) + sizeof(FieldType) + sizeof(uint16_t);
    std::string s(PAGE_SIZE - page.metadata_size - overhead, 'x');
    tuple->addField(std::make_unique<Field>(s));

    ASSERT_EQ(page.addTuple(std::move(tuple), nullptr), 0);

    const std::string before(page.pageData.get(), PAGE_SIZE);
    ASSERT_EQ(page.addTuple(createSmallTestTuple(), nullptr), INVALID_VALUE);
    ASSERT_EQ(std::string(page.pageData.get(), PAGE_SIZE), before);
}

TEST(PageTest, ReusesDeletedSlotWithoutChangingNeighbor) {
    Page page;
    ASSERT_EQ(page.addTuple(createSmallTestTuple(), nullptr), 0);
    ASSERT_EQ(page.addTuple(createLargeTestTuple(), nullptr), 1);

    Slot* slots = reinterpret_cast<Slot*>(page.pageData.get());
    const std::string neighbor(page.pageData.get() + slots[1].offset, slots[1].size);

    ASSERT_TRUE(page.deleteTuple(0));
    ASSERT_FALSE(page.deleteTuple(0));
    ASSERT_EQ(page.addTuple(createSmallTestTuple(), nullptr), 0);
    ASSERT_EQ(std::string(page.pageData.get() + slots[1].offset, slots[1].size), neighbor);
    ASSERT_FALSE(page.deleteTuple(MAX_SLOTS));
}

TEST(PageTest, GetTuple) {
    Page page;
    auto tuple = std::make_unique<Tuple>();

    int i = 123456;
    tuple->addField(std::make_unique<Field>(i));

    float f = 123.456;
    tuple->addField(std::make_unique<Field>(f));

    std::string s = "Hello World";
    tuple->addField(std::make_unique<Field>(s));

    char *reason = nullptr;
    ASSERT_EQ(page.addTuple(createLargeTestTuple(), reason), 0);
    ASSERT_EQ(page.addTuple(std::move(tuple), reason), 1);
    ASSERT_EQ(page.addTuple(createSmallTestTuple(), reason), 2);

    auto retrievedTuple = page.getTuple(1);
    ASSERT_NE(retrievedTuple, nullptr);

    auto intField = retrievedTuple->getField(0);
    auto floatField = retrievedTuple->getField(1);
    auto stringField = retrievedTuple->getField(2);

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


TEST(PageTest, GetTupleReturnsNullForEmptySlot) {
    const Page page;
    EXPECT_EQ(page.getTuple(0), nullptr);
    EXPECT_EQ(page.getTuple(MAX_SLOTS - 1), nullptr);
}

TEST(PageTest, GetTupleReturnsNullForOutOfRangeIndex) {
    Page page;
    ASSERT_EQ(page.addTuple(createSmallTestTuple(), nullptr), 0);
    EXPECT_EQ(page.getTuple(MAX_SLOTS), nullptr);
    EXPECT_EQ(page.getTuple(MAX_SLOTS + 1), nullptr);
    EXPECT_EQ(page.getTuple(std::numeric_limits<size_t>::max()), nullptr);
}

TEST(PageTest, GetTupleReturnsNullAfterDeletionAndReadsReplacement) {
    Page page;
    ASSERT_EQ(page.addTuple(createSmallTestTuple(), nullptr), 0);
    ASSERT_TRUE(page.deleteTuple(0));
    EXPECT_EQ(page.getTuple(0), nullptr);

    auto replacement = std::make_unique<Tuple>();
    replacement->addField(std::make_unique<Field>(42));
    ASSERT_EQ(page.addTuple(std::move(replacement), nullptr), 0);
    auto retrieved = page.getTuple(0);
    ASSERT_NE(retrieved, nullptr);
    auto field = retrieved->getField(0);
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(*reinterpret_cast<int*>(field->value.get()), 42);
}

TEST(PageTest, GetTupleReturnsIndependentCopiesWithoutChangingPage) {
    Page page;
    auto original = createSmallTestTuple();
    const auto expected = original->serialize();
    ASSERT_EQ(page.addTuple(std::move(original), nullptr), 0);
    const std::string before(page.pageData.get(), PAGE_SIZE);
    const Page &readOnlyPage = page;
    auto first = readOnlyPage.getTuple(0);
    auto second = readOnlyPage.getTuple(0);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first.get(), second.get());
    EXPECT_EQ(first->serialize(), expected);
    first->addField(std::make_unique<Field>(42));
    EXPECT_NE(first->serialize(), expected);
    EXPECT_EQ(second->serialize(), expected);
    EXPECT_EQ(std::string(page.pageData.get(), PAGE_SIZE), before);
}

TEST(PageTest, GetTupleReadsLastValidSlot) {
    Page page;
    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        auto tuple = std::make_unique<Tuple>();
        tuple->addField(std::make_unique<Field>(static_cast<int>(i)));
        ASSERT_EQ(page.addTuple(std::move(tuple), nullptr), i);
    }
    auto retrieved = page.getTuple(MAX_SLOTS - 1);
    ASSERT_NE(retrieved, nullptr);
    auto field = retrieved->getField(0);
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(*reinterpret_cast<int*>(field->value.get()), static_cast<int>(MAX_SLOTS - 1));
}
