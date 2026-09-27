//
// Created by Hieu Pham on 9/26/26.
//

#include "buffer/frame_lock_table.h"

FrameLockTable::FrameLockTable() {
    for (auto i = 0; i  < MAX_CACHED_PAGES; i++) {
        ownerShipInfo[i] = std::make_unique<OwnershipInfo>();
    }
}

void FrameLockTable::lockShare(const FrameID frameId, const uint64_t timeoutMillis) const {
    validateFrameId(frameId);

    auto &ownership = ownerShipInfo[frameId];

    std::unique_lock lock(ownership->mutex);
    while (ownership->exclusive) {
        auto status = ownership->exclusiveCV.wait_for(lock, std::chrono::milliseconds(timeoutMillis));
        if (status == std::cv_status::timeout) {
            // TODO: Create exception type for this
            throw std::logic_error("Frame lock timed out");
        }
    }
    ownership->sharedCounts++;
}

void FrameLockTable::lockExclusive(const FrameID frameId, const uint64_t timeoutMillis) const {
    validateFrameId(frameId);

    auto &ownership = ownerShipInfo[frameId];

    std::unique_lock lock(ownership->mutex);

    while (ownership->exclusive) {
        auto status = ownership->exclusiveCV.wait_for(lock, std::chrono::milliseconds(timeoutMillis/2));

        if (status == std::cv_status::timeout) {
            // TODO: Create exception type for this
            throw std::logic_error("Frame lock timed out");
        }
    }

    while (ownership->sharedCounts > 0) {
        auto status = ownership->shareCountCv.wait_for(lock, std::chrono::milliseconds(timeoutMillis/2));
        if (status == std::cv_status::timeout) {
            // TODO: Create exception type for this
            throw std::logic_error("Frame lock timed out");
        }
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
