#include "field.h"

#include <cstring>
#include <sstream>
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

/**
 * Serialize field, with the format:
 *
 * <code>[fieldType] [fieldSize] [fieldData]</code>
 */
std::string Field::serialize() const {
    std::stringstream stream;

    stream << type << ' ';
    stream << size << ' ';

    stream.write(value.get(), static_cast<std::streamsize>(size));

    return stream.str();
}

std::unique_ptr<Field> Field::deserialize(std::istream &in) {
    int type;
    in >> type;

    auto fieldType = static_cast<FieldType>(type);

    size_t size;
    in >> size;

    auto value = std::make_unique<char[]>(size);

    in.seekg(1, std::ios::cur); // skip the white space

    in.read(value.get(), static_cast<std::streamsize>(size));

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
