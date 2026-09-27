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

TEST(FrameLockTableTest, UnlockExclusiveWakesUpWaitingThread) {
    FrameLockTable frameLockTable;

    std::thread t([&]() {
        frameLockTable.lockExclusive(1, 500);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        frameLockTable.unlockExclusive(1);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_NO_THROW(frameLockTable.lockExclusive(1, 500));
    t.join();
}

TEST(FrameLockTableTest, UnlockShareWakesUpWaitingThread) {
    FrameLockTable frameLockTable;
    bool exceptionOcurred = false;

    auto lockShare = [&]() {
        try {
            frameLockTable.lockShare(1, 500);
            frameLockTable.unlockShare(1);
        } catch (std::exception &e) {
            exceptionOcurred = true;
        }
    };

    std::thread t1(lockShare);

    std::thread t2(lockShare);

    std::thread t3(lockShare);

    std::thread t4([&]() {
        try {
            frameLockTable.lockExclusive(1, 500);
            frameLockTable.unlockExclusive(1);
        } catch (std::exception &e) {
            exceptionOcurred = true;
        }
    });

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    ASSERT_FALSE(exceptionOcurred);
}

TEST(FrameLockTableTest, LockShareWaitsForExclusive) {
    FrameLockTable frameLockTable;
    bool exceptionOccurred = false;

    std::thread t1([&]() {
        try {
            frameLockTable.lockShare(1, 500);
            frameLockTable.unlockShare(1);
        } catch (std::logic_error &e) {
            exceptionOccurred = true;
        }
    });


    std::thread t4([&]() {
        try {
            frameLockTable.lockExclusive(1, 500);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            frameLockTable.unlockExclusive(1);
        } catch (std::logic_error &e) {
            exceptionOccurred = true;
        }
    });

    t1.join();
    t4.join();

    ASSERT_FALSE(exceptionOccurred);
}
