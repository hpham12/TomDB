//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_PAGE_H
#define TOMDB_PAGE_H

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

    Page();

    bool addTuple(std::unique_ptr<Tuple> tuple);
};

#endif //TOMDB_PAGE_H
