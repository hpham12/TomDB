//
// Created by Hieu Pham on 9/19/26.
//

#include "buffer/two_q_policy.h"

#include <gtest/gtest.h>

TEST(TwoQPolicyTest, AccessPage) {
    TwoQPolicy twoQueuePolicy;

    PageID pageId1{"fm1", 1};

    twoQueuePolicy.accessPage(pageId1);
    ASSERT_TRUE(twoQueuePolicy.isInFifo(pageId1));
    ASSERT_FALSE(twoQueuePolicy.isInLru(pageId1));

    twoQueuePolicy.accessPage(pageId1);
    ASSERT_FALSE(twoQueuePolicy.isInFifo(pageId1));
    ASSERT_TRUE(twoQueuePolicy.isInLru(pageId1));
}

TEST(TwoQPolicyTest, AccessPageWhenBufferIsFull) {
    TwoQPolicy twoQueuePolicy;

    for (size_t i = 0; i < MAX_CACHED_PAGES; i++) {
        PageID pageId{"fm", static_cast<uint16_t>(i)};
        twoQueuePolicy.accessPage(pageId);
        twoQueuePolicy.accessPage(pageId);
        ASSERT_FALSE(twoQueuePolicy.isInFifo(pageId));
        ASSERT_TRUE(twoQueuePolicy.isInLru(pageId));
    }

    // at this point, buffer is full
    PageID pageId{"fm1", 36218};
    twoQueuePolicy.accessPage(pageId);
    ASSERT_FALSE(twoQueuePolicy.isInFifo(pageId));
    ASSERT_FALSE(twoQueuePolicy.isInLru(pageId));
}

TEST(TwoQPolicyTest, EvictPageInFifo) {
    TwoQPolicy twoQueuePolicy;

    PageID pageId{"fm", 2};
    twoQueuePolicy.accessPage(pageId);
    ASSERT_TRUE(twoQueuePolicy.isInFifo(pageId));
    ASSERT_FALSE(twoQueuePolicy.isInLru(pageId));

    twoQueuePolicy.evictPage(pageId);

    ASSERT_FALSE(twoQueuePolicy.isInFifo(pageId));
    ASSERT_FALSE(twoQueuePolicy.isInLru(pageId));
}

TEST(TwoQPolicyTest, EvictPageInLru) {
    TwoQPolicy twoQueuePolicy;

    PageID pageId{"fm", 2};
    twoQueuePolicy.accessPage(pageId);
    twoQueuePolicy.accessPage(pageId);
    ASSERT_FALSE(twoQueuePolicy.isInFifo(pageId));
    ASSERT_TRUE(twoQueuePolicy.isInLru(pageId));

    twoQueuePolicy.evictPage(pageId);

    ASSERT_FALSE(twoQueuePolicy.isInFifo(pageId));
    ASSERT_FALSE(twoQueuePolicy.isInLru(pageId));
}

TEST(TwoQPolicyTest, SelectPageInFifoToEvict) {
    TwoQPolicy twoQueuePolicy;
    std::unordered_set<PageID> pinnedPages;

    PageID pageId1{"fm", 1};
    twoQueuePolicy.accessPage(pageId1);

    PageID pageId2{"fm1", 1};
    twoQueuePolicy.accessPage(pageId2);
    twoQueuePolicy.accessPage(pageId2);

    ASSERT_TRUE(twoQueuePolicy.isInFifo(pageId1));
    ASSERT_TRUE(twoQueuePolicy.isInLru(pageId2));

    auto pageSelectedForEviction = twoQueuePolicy.selectPageToEvict(pinnedPages);

    ASSERT_EQ(pageSelectedForEviction, pageId1);
}

TEST(TwoQPolicyTest, SelectPageInLruToEvict) {
    TwoQPolicy twoQueuePolicy;
    std::unordered_set<PageID> pinnedPages;

    PageID pageId1{"fm", 1};
    twoQueuePolicy.accessPage(pageId1);

    PageID pageId2{"fm1", 1};
    twoQueuePolicy.accessPage(pageId2);
    twoQueuePolicy.accessPage(pageId2);

    twoQueuePolicy.accessPage(pageId1);

    ASSERT_TRUE(twoQueuePolicy.isInLru(pageId1));
    ASSERT_TRUE(twoQueuePolicy.isInLru(pageId2));

    auto pageSelectedForEviction = twoQueuePolicy.selectPageToEvict(pinnedPages);

    ASSERT_EQ(pageSelectedForEviction, pageId2);
}

TEST(TwoQPolicyTest, SelectPageToEvictWhenPinnedPagesNotEmpty) {
    TwoQPolicy twoQueuePolicy;
    std::unordered_set<PageID> pinnedPages;

    PageID pageId1{"fm", 1};
    twoQueuePolicy.accessPage(pageId1);
    pinnedPages.insert(pageId1);

    PageID pageId2{"fm1", 1};
    twoQueuePolicy.accessPage(pageId2);
    twoQueuePolicy.accessPage(pageId2);

    ASSERT_TRUE(twoQueuePolicy.isInFifo(pageId1));
    ASSERT_TRUE(twoQueuePolicy.isInLru(pageId2));

    auto pageSelectedForEviction = twoQueuePolicy.selectPageToEvict(pinnedPages);

    ASSERT_EQ(pageSelectedForEviction, pageId2);
}

TEST(TwoQPolicyTest, SelectPageToEvictWhenAllPagesarePinned) {
    TwoQPolicy twoQueuePolicy;
    std::unordered_set<PageID> pinnedPages;

    PageID pageId1{"fm", 1};
    twoQueuePolicy.accessPage(pageId1);
    pinnedPages.insert(pageId1);

    PageID pageId2{"fm1", 1};
    twoQueuePolicy.accessPage(pageId2);
    twoQueuePolicy.accessPage(pageId2);
    pinnedPages.insert(pageId2);

    ASSERT_TRUE(twoQueuePolicy.isInFifo(pageId1));
    ASSERT_TRUE(twoQueuePolicy.isInLru(pageId2));

    auto pageSelectedForEviction = twoQueuePolicy.selectPageToEvict(pinnedPages);

    ASSERT_EQ(pageSelectedForEviction, INVALID_PAGE_ID);
}

TEST(TwoQPolicyTest, SelectPageToEvictWhenQueuesAreEmpty) {
    TwoQPolicy twoQueuePolicy;
    std::unordered_set<PageID> pinnedPages;

    auto pageSelectedForEviction = twoQueuePolicy.selectPageToEvict(pinnedPages);

    ASSERT_EQ(pageSelectedForEviction, INVALID_PAGE_ID);
}
