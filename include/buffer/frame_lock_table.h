//
// Created by Hieu Pham on 9/26/26.
//

#ifndef TOMDB_FRAME_LOCK_TABLE_H
#define TOMDB_FRAME_LOCK_TABLE_H
#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>

#include "commons.h"

class OwnershipInfo {
    friend class FrameLockTable;

    uint16_t sharedCounts = 0;
    bool exclusive = false;
    mutable std::mutex mutex;
    std::condition_variable exclusiveCV;
    std::condition_variable shareCountCv;
};

class FrameLockTable {
    std::array<std::unique_ptr<OwnershipInfo>, MAX_CACHED_PAGES> ownerShipInfo;

    static void validateFrameId(const FrameID &frameId);

public:
    void lockShare(FrameID frameId, uint64_t timeoutMillis = 1000) const;

    void unlockShare(FrameID frameId) const;

    void lockExclusive(FrameID frameId, uint64_t timeoutMillis = 1000) const;

    void unlockExclusive(FrameID frameId) const;
};


#endif //TOMDB_FRAME_LOCK_TABLE_H
