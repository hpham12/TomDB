//
// Created by Hieu Pham on 9/19/26.
//

#ifndef TOMDB_TWO_Q_POLICY_H
#define TOMDB_TWO_Q_POLICY_H
#include <mutex>
#include <unordered_set>
#include <vector>

#include "policy.h"

class TwoQPolicy : public Policy {
    std::vector<PageID> fifo;
    std::vector<PageID> lru;
    std::mutex mutex;

public:
    void evictPage(PageID pageId) noexcept(false) override;

    void accessPage(PageID pageId) noexcept(false) override;

    PageID selectPageToEvict(const unordered_set<PageID> &pageIds) noexcept(false) override;
};

#endif //TOMDB_TWO_Q_POLICY_H
