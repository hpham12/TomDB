//
// Created by Hieu Pham on 9/19/26.
//

#include "buffer/two_queue_policy.h"

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
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(i)};
        twoQueuePolicy.accessPage(pageId);
        twoQueuePolicy.accessPage(pageId);
        ASSERT_FALSE(twoQueuePolicy.isInFifo(pageId));
        ASSERT_TRUE(twoQueuePolicy.isInLru(pageId));
    }

    // at this point, buffer is full
    PageID pageId{.fileManagerId="fm1", .fileManagerPageId=36218};
    twoQueuePolicy.accessPage(pageId);
    ASSERT_FALSE(twoQueuePolicy.isInFifo(pageId));
    ASSERT_FALSE(twoQueuePolicy.isInLru(pageId));
}

TEST(TwoQPolicyTest, EvictPageInFifo) {
    TwoQPolicy twoQueuePolicy;

    PageID pageId{.fileManagerId="fm", .fileManagerPageId=2};
    twoQueuePolicy.accessPage(pageId);
    ASSERT_TRUE(twoQueuePolicy.isInFifo(pageId));
    ASSERT_FALSE(twoQueuePolicy.isInLru(pageId));

    twoQueuePolicy.evictPage(pageId);

    ASSERT_FALSE(twoQueuePolicy.isInFifo(pageId));
    ASSERT_FALSE(twoQueuePolicy.isInLru(pageId));
}

TEST(TwoQPolicyTest, EvictPageInLru) {
    TwoQPolicy twoQueuePolicy;

    PageID pageId{.fileManagerId="fm", .fileManagerPageId=2};
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

    PageID pageId1{.fileManagerId="fm", .fileManagerPageId=1};
    twoQueuePolicy.accessPage(pageId1);

    PageID pageId2{.fileManagerId="fm1", .fileManagerPageId=1};
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

    PageID pageId1{.fileManagerId="fm", .fileManagerPageId=1};
    twoQueuePolicy.accessPage(pageId1);

    PageID pageId2{.fileManagerId="fm1", .fileManagerPageId=1};
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

    PageID pageId1{.fileManagerId="fm", .fileManagerPageId=1};
    twoQueuePolicy.accessPage(pageId1);
    pinnedPages.insert(pageId1);

    PageID pageId2{.fileManagerId="fm1", .fileManagerPageId=1};
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

    PageID pageId1{.fileManagerId="fm", .fileManagerPageId=1};
    twoQueuePolicy.accessPage(pageId1);
    pinnedPages.insert(pageId1);

    PageID pageId2{.fileManagerId="fm1", .fileManagerPageId=1};
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

TEST(TwoQPolicyTest, ReaccessMovesPageToMostRecentlyUsedPosition) {
    TwoQPolicy twoQueuePolicy;
    std::unordered_set<PageID> pinnedPages;

    PageID pageId1{.fileManagerId="fm", .fileManagerPageId=0};
    PageID pageId2{.fileManagerId="fm", .fileManagerPageId=1};

    for (auto pageId : {pageId1, pageId2}) {
        twoQueuePolicy.accessPage(pageId);
        twoQueuePolicy.accessPage(pageId);
    }

    twoQueuePolicy.accessPage(pageId1);

    ASSERT_EQ(twoQueuePolicy.selectPageToEvict(pinnedPages), pageId2);
    ASSERT_EQ(twoQueuePolicy.selectPageToEvict(pinnedPages), pageId1);
    ASSERT_EQ(twoQueuePolicy.selectPageToEvict(pinnedPages), INVALID_PAGE_ID);
}

TEST(TwoQPolicyTest, SkipsPinnedPagesWithoutRemovingThem) {
    TwoQPolicy twoQueuePolicy;
    std::unordered_set<PageID> pinnedPages;

    PageID pageId1{.fileManagerId="fm", .fileManagerPageId=0};
    PageID pageId2{.fileManagerId="fm", .fileManagerPageId=1};
    PageID pageId3{.fileManagerId="fm", .fileManagerPageId=2};

    for (auto pageId : {pageId1, pageId2, pageId3}) {
        twoQueuePolicy.accessPage(pageId);
    }

    pinnedPages.insert(pageId1);
    pinnedPages.insert(pageId2);

    ASSERT_EQ(twoQueuePolicy.selectPageToEvict(pinnedPages), pageId3);
    ASSERT_TRUE(twoQueuePolicy.isInFifo(pageId1));
    ASSERT_TRUE(twoQueuePolicy.isInFifo(pageId2));

    pinnedPages.clear();
    ASSERT_EQ(twoQueuePolicy.selectPageToEvict(pinnedPages), pageId1);
    ASSERT_EQ(twoQueuePolicy.selectPageToEvict(pinnedPages), pageId2);
}

TEST(TwoQPolicyTest, EvictionMakesRoomInFullCache) {
    TwoQPolicy twoQueuePolicy;
    std::unordered_set<PageID> pinnedPages;

    for (size_t i = 0; i < MAX_CACHED_PAGES; i++) {
        PageID pageId{.fileManagerId="fm", .fileManagerPageId=static_cast<uint16_t>(i)};
        twoQueuePolicy.accessPage(pageId);
    }

    ASSERT_TRUE(twoQueuePolicy.isCacheFull());

    PageID pageId{.fileManagerId="other", .fileManagerPageId=0};
    PageID firstPageId{.fileManagerId="fm", .fileManagerPageId=0};
    ASSERT_EQ(twoQueuePolicy.selectPageToEvict(pinnedPages), firstPageId);
    ASSERT_FALSE(twoQueuePolicy.isCacheFull());

    twoQueuePolicy.accessPage(pageId);

    ASSERT_TRUE(twoQueuePolicy.isCacheFull());
    ASSERT_TRUE(twoQueuePolicy.isInFifo(pageId));
}
