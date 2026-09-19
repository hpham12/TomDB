//
// Created by Hieu Pham on 9/19/26.
//

#ifndef TOMDB_TWO_Q_POLICY_H
#define TOMDB_TWO_Q_POLICY_H
#include <mutex>
#include <unordered_set>
#include <vector>

#include "policy.h"

/**
 * Two-queue policy is used to avoid cache thrashing during sequential scanning.
 * The idea with this policy is to maintain a FIFO queue and an LRU queue.
 *
 * On initial access, page enters fifo queue, then on second access, page got moved to LRU queue.
 * When it comes to eviction, we prioritize eviction from FIFO queue.
 */
class TwoQPolicy : public Policy {
    std::vector<PageID> fifo;
    std::vector<PageID> lru;
    std::mutex mutex;

public:
    void evictPage(PageID pageId) noexcept(false) override;

    void accessPage(PageID pageId) noexcept(false) override;

    PageID selectPageToEvict(const unordered_set<PageID> &pageIds) noexcept(false) override;

    // method for testing purposes only
    bool isInFifo(PageID) const;
    bool isInLru(PageID) const;
};

#endif //TOMDB_TWO_Q_POLICY_H
