//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_PAGE_H
#define TOMDB_PAGE_H

#include "constants.h"
#include "tuple.h"

class Slot {
public:
    bool empty;
    std::unique_ptr<Tuple> tuple;
    size_t size;

    Slot();
};

class Page {
public:
    std::vector<std::unique_ptr<Slot>> slots;
    std::unique_ptr<char[]> pageData = std::make_unique<char[]>(PAGE_SIZE);

    Page();

    bool addTuple(std::unique_ptr<Tuple> tuple);

    bool deleteTuple(size_t index);
};

#endif //TOMDB_PAGE_H
