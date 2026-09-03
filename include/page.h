//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_PAGE_H
#define TOMDB_PAGE_H

#include "constants.h"
#include "tuple.h"

class Slot {
public:
    bool empty;     // Does slot point to actual data?
    size_t offset;  // Offset of the slot within the page
    size_t size;    // Size of the slot

    Slot() : empty(true), offset(INVALID_VALUE), size(INVALID_VALUE) {}
};

class Page {
public:
    std::unique_ptr<char[]> pageData = std::make_unique<char[]>(PAGE_SIZE);
    size_t metadata_size = sizeof(Slot);

    Page();
    bool addTuple(std::unique_ptr<Tuple> tuple);
    bool deleteTuple(size_t index);
};

#endif //TOMDB_PAGE_H
