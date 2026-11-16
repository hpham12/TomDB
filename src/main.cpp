//
// Created by Hieu Pham on 9/12/26.
//

#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include "metadata.h"
#include "query/query_processor.h"

int main() {
    auto metadata = std::make_shared<Metadata>();

    std::string query;
    while (std::getline(std::cin, query)) {
        try {
            QueryProcessor::processQuery(query, metadata, true);
        } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
        }
    }
}
