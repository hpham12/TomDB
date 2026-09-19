//
// Created by Hieu Pham on 9/19/26.
//

#include "buffer/two_q_policy.h"

#include <unordered_set>

void TwoQPolicy::accessPage(PageID pageId) noexcept(false) {
    std::lock_guard lock(this->mutex);

    // Find page in LRU
    bool foundInLRU = false;
    size_t lruIndex = -1;
    int counter = 0;
    for (auto const &pid: this->lru) {
        if (pid == pageId) {
            foundInLRU = true;
            lruIndex = counter;
            break;
        }
        counter++;
    }

    if (foundInLRU) {
        this->lru.erase(lru.begin() + lruIndex);
        this->lru.push_back(pageId);
        return;
    }

    // if not found in LRU, check for presence in FIFO
    bool foundInFIFO = false;
    size_t fifoIndex = -1;
    counter = 0;
    for (auto const &pid: this->fifo) {
        if (pid == pageId) {
            foundInFIFO = true;
            fifoIndex = counter;
            break;
        }
        counter++;
    }

    if (foundInFIFO) {
        // promote to LRU
        this->fifo.erase(fifo.begin() + fifoIndex);
        this->lru.push_back(pageId);
        return;
    }

    // if not in FIFO or LRU
    if (this->lru.size() + this->fifo.size() < MAX_CACHED_PAGES) {
        this->fifo.push_back(pageId);
    }
}

void TwoQPolicy::evictPage(PageID pageId) noexcept(false) {
    std::lock_guard lock(this->mutex);
    bool foundInLRU = false;
    size_t lruIndex = -1;
    int counter = 0;
    for (auto const &pid: this->lru) {
        if (pid == pageId) {
            foundInLRU = true;
            lruIndex = counter;
            break;
        }
        counter++;
    }

    if (foundInLRU) {
        this->lru.erase(lru.begin() + lruIndex);
        return;
    }

    // if not found in LRU, check for presence in FIFO
    bool foundInFIFO = false;
    size_t fifoIndex = -1;
    counter = 0;
    for (auto const &pid: this->fifo) {
        if (pid == pageId) {
            foundInFIFO = true;
            fifoIndex = counter;
            break;
        }
        counter++;
    }

    if (foundInFIFO) {
        this->fifo.erase(fifo.begin() + fifoIndex);
    }
}

PageID TwoQPolicy::selectPageToEvict(const std::unordered_set<PageID> &pinnedPages) noexcept(false) {
    std::lock_guard lock(this->mutex);
    PageID removedPageId = INVALID_PAGE_ID;

    bool foundEvictVictimInFIFO = false;
    size_t fifoIndex = -1;
    int counter = 0;
    for (auto const &pid: this->fifo) {
        if (!pinnedPages.contains(pid)) {
            foundEvictVictimInFIFO = true;
            fifoIndex = counter;
            removedPageId = pid;
            break;
        }
        counter++;
    }

    if (foundEvictVictimInFIFO) {
        this->fifo.erase(fifo.begin() + fifoIndex);
        return removedPageId;
    }

    // If no evictable victim found in FIFO
    bool foundEvictVictimInLRU = false;
    size_t lruIndex = -1;
    counter = 0;
    for (auto const &pid: this->lru) {
        if (!pinnedPages.contains(pid)) {
            foundEvictVictimInLRU = true;
            lruIndex = counter;
            removedPageId = pid;
            break;
        }
        counter++;
    }

    if (foundEvictVictimInLRU) {
        this->lru.erase(lru.begin() + lruIndex);
        return removedPageId;
    }

    return INVALID_PAGE_ID;
}

bool TwoQPolicy::isInFifo(PageID pageId) const {
    for (auto const &pid : fifo) {
        if (pageId == pid) {
            return true;
        }
    }

    return false;
}

bool TwoQPolicy::isInLru(PageID pageId) const {
    for (auto const &pid : lru) {
        if (pageId == pid) {
            return true;
        }
    }

    return false;
}
