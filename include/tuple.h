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
    static std::unique_ptr<Tuple> deserialize(std::istream &in);

    /**
     * Serialize tuple with the following format:
     *
     * <code>[tupleSize] [serializedField1] [serializedField2]</code>
     */
    std::string serialize();

    size_t getSize();
    void addField(std::unique_ptr<Field> &field);
    std::unique_ptr<Field> getField(size_t index);
};

#endif //TOMDB_TUPLE_H
