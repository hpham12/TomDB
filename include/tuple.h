//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_TUPLE_H
#define TOMDB_TUPLE_H
#include <memory>
#include <vector>

#include "field.h"

class Tuple {
private:
    std::vector<std::unique_ptr<Field>> fields;

public:
    static Tuple deserialize(std::ifstream &in);
    static std::string serialize(Tuple tuple);
    size_t getSize();
};

#endif //TOMDB_TUPLE_H
