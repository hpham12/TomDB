//
// Created by Hieu Pham on 9/26/26.
//

#include "buffer/frame_lock_table.h"

#include <gtest/gtest.h>
#include <barrier>
#include <thread>
#include <vector>

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

TEST(FrameLockTableTest, LockShareWaitsForExclusive) {
    FrameLockTable frameLockTable;
    frameLockTable.lockExclusive(1);

    int readerCount = 8;
    std::barrier start(readerCount + 1);
    std::atomic<int> activeReaders{0};
    std::atomic<int> completed{0};
    std::atomic<bool> failed{false};
    std::vector<std::thread> writers;

    for (int i = 0; i < readerCount; ++i) {
        writers.emplace_back([&] {
            start.arrive_and_wait();
            try {
                frameLockTable.lockShare(1, 2000);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                --activeReaders;
                frameLockTable.unlockShare(1);
                ++completed;
            } catch (const std::exception &) {
                failed = true;
            }
        });
    }

    start.arrive_and_wait();

    // Give the readers opportunity to wait behind the initial reader.
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    frameLockTable.unlockExclusive(1);
    for (auto &reader : writers) {
        reader.join();
    }

    EXPECT_FALSE(failed);
    EXPECT_EQ(completed, readerCount);
}


TEST(FrameLockTableTest, WaitingWritersRemainExclusiveAfterLastReaderReleases) {
    FrameLockTable frameLockTable;
    frameLockTable.lockShare(1);

    int writerCount = 8;
    std::barrier start(writerCount + 1);
    std::atomic<int> activeWriters{0};
    std::atomic<int> completed{0};
    std::atomic<bool> overlap{false};
    std::atomic<bool> failed{false};
    std::vector<std::thread> writers;

    for (int i = 0; i < writerCount; ++i) {
        writers.emplace_back([&] {
            start.arrive_and_wait();
            try {
                frameLockTable.lockExclusive(1, 2000);
                if (activeWriters.fetch_add(1) != 0) {
                    overlap = true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                --activeWriters;
                frameLockTable.unlockExclusive(1);
                ++completed;
            } catch (const std::exception &) {
                failed = true;
            }
        });
    }

    start.arrive_and_wait();

    // Give the writers opportunity to wait behind the initial reader.
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    frameLockTable.unlockShare(1);
    for (auto &writer : writers) {
        writer.join();
    }

    EXPECT_FALSE(overlap);
    EXPECT_FALSE(failed);
    EXPECT_EQ(completed, writerCount);
}
