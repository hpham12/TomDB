//
// Created by Hieu Pham on 9/19/26.
//

#ifndef TOMDB_POLICY_H
#define TOMDB_POLICY_H
#include <unordered_set>
#include "commons.h"

class Policy {
public:
    virtual ~Policy() = default;
    virtual void evictPage(PageID pageId) noexcept(false) = 0;
    virtual void accessPage(PageID pageId) noexcept(false) = 0;
    virtual PageID selectPageToEvict(const std::unordered_set<PageID> &pageIds) noexcept(false) = 0;
    [[nodiscard]] virtual bool isCacheFull() const noexcept = 0;
};

#endif //TOMDB_POLICY_H
