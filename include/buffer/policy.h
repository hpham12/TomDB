//
// Created by Hieu Pham on 9/19/26.
//

#ifndef TOMDB_POLICY_H
#define TOMDB_POLICY_H
#include <set>

#include "constants.h"

class Policy {
public:
    virtual ~Policy() = default;
    virtual void evictPage(PageID pageId) noexcept(false);
    virtual void accessPage(PageID pageId) noexcept(false);
    virtual PageID selectPageToEvict(std::set<PageID> pageIds) noexcept(false);
};

#endif //TOMDB_POLICY_H
