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
    size = s.length() + 1;
    value = std::make_unique<char[]>(size);
    s.copy(value.get(), size - 1);
    value.get()[size - 1] = '\0'; // add null terminator
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
    if (type == STRING) {
        stream.write(value.get(), static_cast<std::streamsize>(size) - 1);
    } else {
        stream.write(value.get(), static_cast<std::streamsize>(size));
    }

    return stream.str();
}

Field Field::deserialize(std::istream &in) {
    int type;
    in >> type;

    auto fieldType = static_cast<FieldType>(type);

    size_t size;
    in >> size;

    auto value = std::make_unique<char[]>(size);

    in.read(value.get(), static_cast<std::streamsize>(size));

    if (fieldType == INTEGER) {
        return Field(*reinterpret_cast<int*>(value.get()));
    }

    if (fieldType == FLOAT) {
        return Field(*reinterpret_cast<float*>(value.get()));
    }

    return Field(value.get());
}
