//
// Created by Hieu Pham on 9/26/26.
//

#include "buffer/frame_lock_table.h"

#include <chrono>

FrameLockTable::FrameLockTable() {
    for (auto i = 0; i  < MAX_CACHED_PAGES; i++) {
        ownerShipInfo[i] = std::make_unique<OwnershipInfo>();
    }
}

void FrameLockTable::lockShare(const FrameID frameId, const uint64_t timeoutMillis) const {
    validateFrameId(frameId);

    auto &ownership = ownerShipInfo[frameId];

    std::unique_lock lock(ownership->mutex);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMillis);
    while (ownership->exclusive) {
        const auto status = ownership->stateChangedCV.wait_until(lock, deadline);
        if (status == std::cv_status::timeout && ownership->exclusive) {
            throw std::logic_error("Frame lock timed out");
        }
    }
    ownership->sharedCounts++;
}

void FrameLockTable::lockExclusive(const FrameID frameId, const uint64_t timeoutMillis) const {
    validateFrameId(frameId);

    auto &ownership = ownerShipInfo[frameId];

    std::unique_lock lock(ownership->mutex);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMillis);
    while (ownership->exclusive || ownership->sharedCounts > 0) {
        const auto status = ownership->stateChangedCV.wait_until(lock, deadline);
        if (status == std::cv_status::timeout &&
            (ownership->exclusive || ownership->sharedCounts > 0)) {
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
        ownership->stateChangedCV.notify_all();
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
    ownership->stateChangedCV.notify_all();
}

void FrameLockTable::validateFrameId(const FrameID &frameId) {
    if (frameId >= MAX_CACHED_PAGES) {
        throw std::out_of_range("frameId out of range");
    }
}
