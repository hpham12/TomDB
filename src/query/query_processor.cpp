//
// Created by Hieu Pham on 11/13/26.
//

#include "query/query_processor.h"
#include "query/insert_json.h"

#include <charconv>
#include <regex>
#include <vector>

void QueryProcessor::execute(std::unique_ptr<Operator> &op) {
    op->open();
    while (op->next()) {
        op->getOutput();
    }
    op->close();
}

void QueryProcessor::processQuery(std::string query, const shared_ptr<Metadata>& metadata, bool display) {
    std::unique_ptr<Operator> op = parse(query, metadata, display);
    execute(op);
}

std::unique_ptr<Operator> QueryProcessor::parse(string query, const shared_ptr<Metadata>& metadata, bool display) {
    std::smatch match;
    std::unique_ptr<Operator> op;
    if (std::regex_match(query, match, SELECT_PATTERN)) {
        std::unordered_set<std::string> fieldNames;
        const std::string fieldList = match[1].str();
        const bool selectAll = fieldList == "*";
        static const std::regex fieldPattern(R"([A-Za-z_][A-Za-z0-9_]*)");
        for (std::sregex_iterator it(fieldList.begin(), fieldList.end(), fieldPattern), end; it != end; ++it) {
            fieldNames.emplace(it->str());
        }

        const std::string documentName = match[2].str();
        const std::string keyText = match[3].str();
        Key lookupKeyNumber{};
        const auto result = std::from_chars(keyText.data(), keyText.data() + keyText.size(), lookupKeyNumber);
        if (result.ec != std::errc{} || result.ptr != keyText.data() + keyText.size()) {
            throw std::invalid_argument("Invalid query type");
        }

        auto selectOp = std::make_unique<SelectOperator>(lookupKeyNumber, documentName, std::move(fieldNames), selectAll);
        if (display) {
            op = std::make_unique<PrintOperator>(std::move(selectOp));
        } else {
            op = std::move(selectOp);
        }
    } else if (std::regex_match(query, match, INSERT_PATTERN)) {
        auto parsed = parseInsertJson(match[2].str());
        if (!parsed) {
            throw std::invalid_argument("Invalid query type");
        }
        op = std::make_unique<InsertOperator>(std::move(parsed->tuple), parsed->key, match[1].str(), metadata);
    } else if (std::regex_match(query, match, CREATE_PATTERN)) {
        op = std::make_unique<CreateDocumentOperator>(match[1].str(), metadata);
    } else if (std::regex_match(query, match, REMOVE_PATTERN)) {
        op = std::make_unique<DropDocumentOperator>(match[1].str(), metadata);
    } else {
        throw std::invalid_argument("Invalid query type");
    }

    return op;
}
