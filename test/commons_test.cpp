#include <gtest/gtest.h>

#include "commons.h"

TEST(RecordTest, EqualWhenPageAndSlotMatch) {
    const Record record{.pageId={"data", 7}, .slotIndex=3};
    const Record same{.pageId={"data", 7}, .slotIndex=3};

    EXPECT_EQ(record, same);
    EXPECT_EQ(same, record);
    EXPECT_EQ(record, record);
}

TEST(RecordTest, NotEqualWhenSlotDiffers) {
    const Record record{.pageId={"data", 7}, .slotIndex=3};
    const Record other{.pageId={"data", 7}, .slotIndex=4};

    EXPECT_FALSE(record == other);
    EXPECT_FALSE(other == record);
}

TEST(RecordTest, NotEqualWhenPageNumberDiffers) {
    const Record record{.pageId={"data", 7}, .slotIndex=3};
    const Record other{.pageId={"data", 8}, .slotIndex=3};

    EXPECT_FALSE(record == other);
    EXPECT_FALSE(other == record);
}

TEST(RecordTest, NotEqualWhenFileManagerDiffers) {
    const Record record{.pageId={"data", 7}, .slotIndex=3};
    const Record other{.pageId={"index", 7}, .slotIndex=3};

    EXPECT_FALSE(record == other);
    EXPECT_FALSE(other == record);
}

TEST(RecordTest, DefaultRecordEqualsInvalidRecord) {
    const Record record;

    EXPECT_EQ(record, INVALID_RECORD);
    EXPECT_EQ(INVALID_RECORD, record);
    const Record valid{.pageId={"data", 0}, .slotIndex=0};
    EXPECT_FALSE(record == valid);
}
