//
// Created by Hieu Pham on 10/6/26.
//

#include <gtest/gtest.h>

#include "../test_utils.h"
#include "index/b_tree.h"

TEST(BTREETEST, INSERT) {
    // auto dataFilePath = generateRandomFilePath();
    // auto indexFilePath = generateRandomFilePath();
    // BTree<int> btree(indexFilePath, "index", dataFilePath, "data");
    //
    // // setup tuple
    //
    // std::unique_ptr<Tuple> tuple = std::make_unique<Tuple>();
    // tuple->addField(std::make_unique<Field>(1));
    // tuple->addField(std::make_unique<Field>((float) 123.123));
    // tuple->addField(std::make_unique<Field>("Hello World"));
    //
    // btree.insert(1, std::move(tuple));
    //
    // auto resultTuple = btree.get(1);
    // EXPECT_NE(resultTuple, nullptr);
    // auto field1 = resultTuple->getField(0);
    // EXPECT_EQ(field1->type, FieldType::INTEGER);
    // EXPECT_EQ(field1->size, sizeof(int));
    // EXPECT_EQ(*reinterpret_cast<int*>(field1->value.get()), 1);
    //
    // auto field2 = resultTuple->getField(1);
    // EXPECT_EQ(field2->type, FieldType::FLOAT);
    // EXPECT_EQ(field2->size, sizeof(float));
    // EXPECT_FLOAT_EQ(*reinterpret_cast<float*>(field2->value.get()), 123.123);
    //
    // auto field3 = resultTuple->getField(2);
    // EXPECT_EQ(field3->type, FieldType::STRING);
    // EXPECT_EQ(field3->size, 11);
    // EXPECT_STREQ(field3->value.get(), "Hello World");
}