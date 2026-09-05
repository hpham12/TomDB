#include "field.h"

#include <cstring>
#include <sstream>
#include <iostream>
//
// Created by Hieu Pham on 9/1/26.
//

Field::Field(const int i) {
    type = INTEGER;
    size = sizeof(int);
    value = std::make_unique<char[]>(size);
    memcpy(value.get(), &i, size);
}

Field::Field(const float f) {
    type = FLOAT;
    size = sizeof(float);
    value = std::make_unique<char[]>(size);
    memcpy(value.get(), &f, size);
}

Field::Field(const std::string &s) {
    type = STRING;
    size = s.length();
    value = std::make_unique<char[]>(size + 1);
    s.copy(value.get(), size);
    value.get()[size] = '\0'; // add null terminator
}

std::string Field::serialize() const {
    std::stringstream stream;

    stream.write(reinterpret_cast<const char*>(&type), sizeof(type));
    stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
    stream.write(value.get(), size);

    return stream.str();
}

std::unique_ptr<Field> Field::deserialize(std::istream &in) {
    FieldType fieldType;
    in.read(reinterpret_cast<char*>(&fieldType), sizeof(fieldType));

    uint16_t fieldSize;
    in.read(reinterpret_cast<char*>(&fieldSize), sizeof(fieldSize));

    auto value = std::make_unique<char[]>(fieldSize);

    in.read(value.get(), fieldSize);

    if (fieldType == INTEGER) {
        return std::make_unique<Field>(*reinterpret_cast<int*>(value.get()));
    }

    if (fieldType == FLOAT) {
        return std::make_unique<Field>(*reinterpret_cast<float*>(value.get()));
    }

    return std::make_unique<Field>(value.get());
}

std::unique_ptr<Field> Field::clone() const {
    if (type == STRING) {
        std::cout << value.get() << std::endl;
        return std::make_unique<Field>(value.get());
    }

    if (type == INTEGER) {
        return std::make_unique<Field>(*reinterpret_cast<int*>(value.get()));
    }

    if (type == FLOAT) {
        return std::make_unique<Field>(*reinterpret_cast<float*>(value.get()));
    }

    throw std::logic_error("No valid type found");
}
