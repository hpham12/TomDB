//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_FIELD_H
#define TOMDB_FIELD_H
#include <memory>
#include <string>

enum FieldType {
    INTEGER,    // 0
    FLOAT,      // 1
    STRING      // 2
};

class Field {
public:
    FieldType type;
    std::unique_ptr<char[]> value;
    size_t size;

    // constructors
    explicit Field(int i);

    explicit Field(const std::string& s);

    explicit Field(float f);

    std::string serialize() const;
    static std::unique_ptr<Field> deserialize(std::istream &in);
    std::unique_ptr<Field> clone() const;
};

#endif //TOMDB_FIELD_H
