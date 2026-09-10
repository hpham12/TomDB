//
// Created by Hieu Pham on 8/31/26.
//

#include "page.h"

#include <cstring>

// construct an empty page
Page::Page() {
    Slot* slots = reinterpret_cast<Slot*>(pageData.get());

    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        slots[i].empty = true;
        slots[i].size = INVALID_VALUE;
        slots[i].offset = INVALID_VALUE;
    }
}

size_t Page::addTuple(std::unique_ptr<Tuple> tuple, char* reason) {
    auto slots = reinterpret_cast<Slot*>(pageData.get());
    // Iterate through slots to find one that can potentially hold the new tuple
    bool foundSlot = false;
    size_t slotIndex = INVALID_VALUE;
    uint32_t tupleSize = tuple->getSize();
    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        auto slot = slots[i];
        if (slot.empty && slot.size >= tupleSize) {
            foundSlot = true;
            slotIndex = i;
            break;
        }
    }

    if (!foundSlot) {
        if (reason != nullptr) {
            char failReason[] = "No empty slot is large enough to hold tuple";
            memcpy(reason, failReason, sizeof(failReason));
        }
        return INVALID_VALUE;
    }

    size_t offset;
    if (slotIndex == 0) {
        offset = metadata_size;
    } else {
        offset = slots[slotIndex - 1].offset + slots[slotIndex - 1].size;
    }

    // next, check whether the slot can actually hold the tuple without overflowing the page
    if (offset + tupleSize > PAGE_SIZE) {
        if (reason != nullptr) {
            char failReason[] = "Tuple cannot fit in page";
            memcpy(reason, failReason, sizeof(failReason));
        }
        return INVALID_VALUE;
    }

    slots[slotIndex].offset = offset;
    slots[slotIndex].size = tupleSize;
    slots[slotIndex].empty = false;

    memcpy(pageData.get() + offset, tuple->serialize().c_str(), tupleSize);

    return slotIndex;
}

bool Page::deleteTuple(size_t index) {
    if (index >= MAX_SLOTS) {
        return false;
    }

    auto slots = reinterpret_cast<Slot*>(pageData.get());

    if (slots[index].empty) {
        return false;
    }

    slots[index].empty = true;

    return true;
}
