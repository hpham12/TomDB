//
// Created by Hieu Pham on 11/13/26.
//

#ifndef TOMDB_QUERY_PROCESSOR_H
#define TOMDB_QUERY_PROCESSOR_H
#include <memory>

#include "metadata.h"
#include "operators.h"

class QueryProcessor {
public:
    static std::unique_ptr<Operator> parse(std::string query, Metadata metadata);
    static void processQuery(std::string query);
    static void execute(std::unique_ptr<Operator> &op);
};

#endif //TOMDB_QUERY_PROCESSOR_H
