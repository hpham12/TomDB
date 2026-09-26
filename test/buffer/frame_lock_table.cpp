//
// Created by Hieu Pham on 9/26/26.
//

#include "buffer/frame_lock_table.h"

#include <gtest/gtest.h>

TEST(FrameLockTableTest, LockShare) {
    const FrameLockTable frameLockTable;

    frameLockTable.lockShare(1, 100);

    auto &ownershipInfo1 = frameLockTable.ownerShipInfo[1];

    ASSERT_EQ(ownershipInfo1->sharedCounts, 1);
    ASSERT_FALSE(ownershipInfo1->exclusive);

    frameLockTable.lockShare(1, 100);

    ASSERT_EQ(ownershipInfo1->sharedCounts, 2);
    ASSERT_FALSE(ownershipInfo1->exclusive);

    frameLockTable.lockShare(2, 100);

    auto &ownershipInfo2 = frameLockTable.ownerShipInfo[2];

    ASSERT_EQ(ownershipInfo2->sharedCounts, 1);
    ASSERT_FALSE(ownershipInfo2->exclusive);
}

TEST(FrameLockTableTest, LockShareTimeout) {
    const FrameLockTable frameLockTable;

    frameLockTable.lockExclusive(1, 100);
    ASSERT_ANY_THROW(frameLockTable.lockShare(1, 100));
}


TEST(FrameLockTableTest, LockExclusive) {
    const FrameLockTable frameLockTable;

    frameLockTable.lockExclusive(1, 100);

    auto &ownershipInfo = frameLockTable.ownerShipInfo[1];

    ASSERT_EQ(ownershipInfo->sharedCounts, 0);
    ASSERT_TRUE(ownershipInfo->exclusive);
}

TEST(FrameLockTableTest, LockExclusiveTimeoutWithAnotherExclusiveLockActive) {
    const FrameLockTable frameLockTable;

    frameLockTable.lockExclusive(1, 100);
    ASSERT_ANY_THROW(frameLockTable.lockExclusive(1, 100));
}

TEST(FrameLockTableTest, LockExclusiveTimeoutWithAnotherShareLockActive) {
    const FrameLockTable frameLockTable;

    frameLockTable.lockShare(1, 100);
    ASSERT_ANY_THROW(frameLockTable.lockExclusive(1, 100));
}

TEST(FrameLockTableTest, UnlockShare) {
    const FrameLockTable frameLockTable;

    frameLockTable.lockShare(1, 100);

    auto &ownershipInfo = frameLockTable.ownerShipInfo[1];

    ASSERT_EQ(ownershipInfo->sharedCounts, 1);
    ASSERT_FALSE(ownershipInfo->exclusive);

    frameLockTable.unlockShare(1);

    ASSERT_EQ(ownershipInfo->sharedCounts, 0);
    ASSERT_FALSE(ownershipInfo->exclusive);
}

TEST(FrameLockTableTest, UnlockShareThrowsWhenLockExclusive) {
    const FrameLockTable frameLockTable;

    frameLockTable.lockExclusive(1, 100);

    ASSERT_THROW(frameLockTable.unlockShare(1), std::logic_error);
}

TEST(FrameLockTableTest, UnlockExclusive) {
    const FrameLockTable frameLockTable;

    frameLockTable.lockExclusive(1, 100);
    auto &ownershipInfo = frameLockTable.ownerShipInfo[1];

    ASSERT_EQ(ownershipInfo->sharedCounts, 0);
    ASSERT_TRUE(ownershipInfo->exclusive);

    frameLockTable.unlockExclusive(1);

    ASSERT_EQ(ownershipInfo->sharedCounts, 0);
    ASSERT_FALSE(ownershipInfo->exclusive);
}

TEST(FrameLockTableTest, UnlockExclusiveThrowsWhenLockShare) {
    const FrameLockTable frameLockTable;

    frameLockTable.lockShare(1, 100);

    ASSERT_THROW(frameLockTable.unlockExclusive(1), std::logic_error);
}

TEST(FrameLockTableTest, ValidateFrameId) {
    ASSERT_NO_THROW(FrameLockTable::validateFrameId(MAX_CACHED_PAGES - 1));
    ASSERT_THROW(FrameLockTable::validateFrameId(MAX_CACHED_PAGES), std::out_of_range);
    ASSERT_THROW(FrameLockTable::validateFrameId(MAX_CACHED_PAGES + 1), std::out_of_range);
}
