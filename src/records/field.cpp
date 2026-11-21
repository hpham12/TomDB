#include "records/field.h"

#include <cstring>
#include <sstream>
#include <iostream>
//
// Created by Hieu Pham on 9/1/26.
//

Field::Field(const int i, std::string name) : name(name){
    type = INTEGER;
    size = sizeof(int);
    value = std::make_unique<char[]>(size);
    memcpy(value.get(), &i, size);
}

Field::Field(const float f, std::string name) : name(name){
    type = FLOAT;
    size = sizeof(float);
    value = std::make_unique<char[]>(size);
    memcpy(value.get(), &f, size);
}

Field::Field(const std::string &s, std::string name) : name(name){
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
    uint8_t nameSize = name.size(); // add null-terminator
    stream.write(reinterpret_cast<const char*>(&nameSize), sizeof(nameSize));
    stream.write(name.data(), nameSize);

    return stream.str();
}

std::unique_ptr<Field> Field::deserialize(std::istream &in) {
    // get field type
    FieldType fieldType;
    in.read(reinterpret_cast<char*>(&fieldType), sizeof(fieldType));

    // get field value
    uint16_t fieldSize;
    in.read(reinterpret_cast<char*>(&fieldSize), sizeof(fieldSize));

    auto value = std::make_unique<char[]>(fieldSize);

    in.read(value.get(), fieldSize);

    // get name
    uint8_t nameSize;
    in.read(reinterpret_cast<char*>(&nameSize), sizeof(nameSize));

    char buffer[nameSize];
    in.read(buffer, nameSize);
    std::string name{buffer, nameSize};

    if (fieldType == INTEGER) {
        return std::make_unique<Field>(*reinterpret_cast<int*>(value.get()), name);
    }

    if (fieldType == FLOAT) {
        return std::make_unique<Field>(*reinterpret_cast<float*>(value.get()), name);
    }

    return std::make_unique<Field>(std::string(value.get(), fieldSize), name);
}

std::unique_ptr<Field> Field::clone() const {
    if (type == STRING) {
        return std::make_unique<Field>(value.get(), name);
    }

    if (type == INTEGER) {
        return std::make_unique<Field>(*reinterpret_cast<int*>(value.get()), name);
    }

    if (type == FLOAT) {
        return std::make_unique<Field>(*reinterpret_cast<float*>(value.get()), name);
    }

    throw std::logic_error("No valid type found");
}

uint32_t Field::getSize() const {
    return size + sizeof(size) + sizeof(type) + sizeof(uint8_t) + name.size();
}
