//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_FIELD_H
#define TOMDB_FIELD_H
#include <string>

enum FieldType { INTEGER, FLOAT, STRING };

class Field {
public:
    FieldType type;
    std::unique_ptr<char[]> value;
    size_t size;

    // constructors
    Field(int i);
    Field(std::string s);
    Field(float f);

    static std::string serialize(Field &field);
    static Field deserialize(std::ifstream &in);
};

#endif //TOMDB_FIELD_H
