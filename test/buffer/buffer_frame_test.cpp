//
// Created by Hieu Pham on 9/23/26.
//
#include "buffer/buffer_frame.h"

#include <gtest/gtest.h>

TEST(BufferFrameTest, MarkDirty) {
    BufferFrame frame;
    ASSERT_FALSE(frame.isPageDirty());
    frame.markDirty();
    ASSERT_TRUE(frame.isPageDirty());
}

TEST(BufferFrameTest, Reset) {
    BufferFrame frame;
    frame.page = std::make_unique<Page>();
    frame.markDirty();

    ASSERT_TRUE(frame.isPageDirty());

    frame.reset();
    EXPECT_EQ(frame.page, nullptr);
    ASSERT_FALSE(frame.isPageDirty());
}
