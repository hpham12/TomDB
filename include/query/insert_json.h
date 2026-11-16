#ifndef TOMDB_INSERT_JSON_H
#define TOMDB_INSERT_JSON_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "records/tuple.h"

struct ParsedInsertJson {
    std::uint64_t key;
    std::unique_ptr<Tuple> tuple;
};

// Parses a JSON object whose values can be represented by Tuple's scalar fields.
std::optional<ParsedInsertJson> parseInsertJson(std::string_view json);

#endif // TOMDB_INSERT_JSON_H
