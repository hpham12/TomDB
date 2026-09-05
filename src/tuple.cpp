//
// Created by Hieu Pham on 9/3/26.
//

#include "tuple.h"
#include <sstream>

void Tuple::addField(std::unique_ptr<Field> field) {
    fields.push_back(field->clone());
}

size_t Tuple::getSize() {
    size_t totalSize = 0;
    for (std::unique_ptr<Field> &field : fields) {
        totalSize += field->size;
    }

    return totalSize;
}

std::string Tuple::serialize() {
    size_t totalSize = getSize();

    std::stringstream stream;
    stream << totalSize << ' ';

    for (const auto & field : fields) {
        stream << field->serialize();
        stream << ' ';
    }

    return stream.str();
}

std::unique_ptr<Tuple> Tuple::deserialize(std::istream &in) {
    size_t tupleSize;
    in >> tupleSize;
    std::unique_ptr<Tuple> tuple = std::make_unique<Tuple>();

    size_t totalSize = 0;

    while (totalSize < tupleSize) {
        auto field = Field::deserialize(in);
        totalSize += field->size;
        tuple->addField(field->clone());
    }

    return tuple;
}

std::unique_ptr<Field> Tuple::getField(size_t index) {
    if (index >= fields.size()) {
        throw std::out_of_range("Out of bound!");
    }

    return fields[index]->clone();
}
