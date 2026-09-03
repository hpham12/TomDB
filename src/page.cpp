//
// Created by Hieu Pham on 8/31/26.
//

#include "page.h"

// construct an empty page
Page::Page() {
    Slot* slots = reinterpret_cast<Slot*>(pageData.get());

    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        slots[i].empty = true;
        slots[i].size = INVALID_VALUE;
        slots[i].offset = INVALID_VALUE;
    }
}

bool Page::addTuple(std::unique_ptr<Tuple> tuple) {
    // TODO: Implement
    return false;
}

bool Page::deleteTuple(size_t index) {
    // TODO: Implement
    return false;
}
