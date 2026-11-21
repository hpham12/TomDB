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
    uint16_t size;
    std::string name;

    // constructors
    explicit Field(int i, std::string name);

    explicit Field(const std::string& s, std::string name);

    explicit Field(float f, std::string name);

    /**
     * Serialize field (byte representation, little endian), with the following format:
     *
     * <code>[fieldType][fieldSize][fieldData][fieldNameSize][fieldName]</code>
     */
    [[nodiscard]] std::string serialize() const;

    static std::unique_ptr<Field> deserialize(std::istream &in);

    [[nodiscard]] std::unique_ptr<Field> clone() const;

    /**
     * Total size (bytes) of the serialized field
     */
    [[nodiscard]] uint32_t getSize() const;
};

#endif //TOMDB_FIELD_H
