//
// Created by Hieu Pham on 11/13/26.
//

#ifndef TOMDB_QUERY_PROCESSOR_H
#define TOMDB_QUERY_PROCESSOR_H
#include <memory>
#include <regex>

#include "metadata.h"
#include "operators.h"

inline std::regex SELECT_PATTERN(
    R"(^\s*SELECT\s+([A-Za-z_][A-Za-z0-9_]*(?:\s*,\s*[A-Za-z_][A-Za-z0-9_]*)*)\s+FROM\s+([A-Za-z_][A-Za-z0-9_]*)\s+WHERE\s+ID\s*=\s*([0-9]+)\s*;?\s*$)",
    std::regex::icase);

inline const std::regex INSERT_PATTERN(
    R"(^\s*INSERT\s+INTO\s+([A-Za-z_][A-Za-z0-9_]*)\s+VALUE\s+(\{[\s\S]*\})\s*;?\s*$)",
    std::regex::icase);

inline const std::regex CREATE_PATTERN(
    R"(^\s*CREATE\s+DOCUMENT\s+([A-Za-z_][A-Za-z0-9_]*)\s*;?\s*$)",
    std::regex::icase);

inline const std::regex REMOVE_PATTERN(
    R"(^\s*REMOVE\s+DOCUMENT\s+([A-Za-z_][A-Za-z0-9_]*)\s*;?\s*$)",
    std::regex::icase);

class QueryProcessor {
public:
    static std::unique_ptr<Operator> parse(string query, const shared_ptr<Metadata>& metadata, bool display);
    static void processQuery(string query, const shared_ptr<Metadata>& metadata, bool display);
    static void execute(unique_ptr<Operator> &op);
};

#endif //TOMDB_QUERY_PROCESSOR_H
