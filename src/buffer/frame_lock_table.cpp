//
// Created by Hieu Pham on 9/26/26.
//

#include "buffer/frame_lock_table.h"

void FrameLockTable::lockShare(const FrameID frameId, const uint64_t timeoutMillis) const {
    validateFrameId(frameId);

    auto &ownership = ownerShipInfo[frameId];

    std::unique_lock lock(ownership->mutex);
    while (ownership->exclusive) {
        ownership->exclusiveCV.wait_for(lock, std::chrono::milliseconds(timeoutMillis));
    }
    ownership->sharedCounts++;
}

void FrameLockTable::lockExclusive(const FrameID frameId, const uint64_t timeoutMillis) const {
    validateFrameId(frameId);

    auto &ownership = ownerShipInfo[frameId];

    std::unique_lock lock(ownership->mutex);

    while (ownership->exclusive) {
        ownership->exclusiveCV.wait_for(lock, std::chrono::milliseconds(timeoutMillis/2));
    }

    while (ownership->sharedCounts > 0) {
        ownership->shareCountCv.wait_for(lock, std::chrono::milliseconds(timeoutMillis/2));
    }

    ownership->exclusive = true;
}

void FrameLockTable::unlockShare(const FrameID frameId) const {
    validateFrameId(frameId);

    auto &ownership = ownerShipInfo[frameId];

    std::unique_lock lock(ownership->mutex);
    if (ownership->exclusive) {
        throw std::logic_error("frame is exclusive and cannot be unlocked as shared entity");
    }
    ownership->sharedCounts--;
    if (ownership->sharedCounts == 0) {
        ownership->shareCountCv.notify_all();
    }
}

void FrameLockTable::unlockExclusive(const FrameID frameId) const {
    validateFrameId(frameId);

    auto &ownership = ownerShipInfo[frameId];

    std::unique_lock lock(ownership->mutex);
    if (!ownership->exclusive) {
        throw std::logic_error("frame is shared and cannot be unlocked as exclusive entity");
    }
    ownership->exclusive = false;
    ownership->exclusiveCV.notify_all();
}

void FrameLockTable::validateFrameId(const FrameID &frameId) {
    if (frameId >= MAX_CACHED_PAGES) {
        throw std::out_of_range("frameId out of range");
    }
}
