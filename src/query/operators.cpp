//
// Created by Hieu Pham on 11/13/26.
//

#include "query/operators.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

// ---- InsertOperator ---- //
void InsertOperator::open() {
    // TODO: Create BTree instance
    std::string indexFileName = document + "_tomdb.idx";
    std::string dataFileName = document + "_tomdb.dat";

    std::string indexFilePath = "/.tomdb/" + indexFileName;
    std::string dataFilePath = "/.tomdb/" + dataFileName;

    index = std::make_unique<BTree<Key>>(indexFilePath, indexFileName, dataFilePath, dataFileName);
}

bool InsertOperator::next() {
    if (inserted) {
        return false;
    }
    index->insert(keyToInsert, std::move(tupleToInsert));
    std::cout<<"inserted"<<std::endl;
    inserted = true;
    return true;
}

std::unique_ptr<Tuple> InsertOperator::getOutput() {
    return nullptr;
}

void InsertOperator::close() {}

// ---- SelectOperator ---- //
void SelectOperator::open() {
    std::string indexFileName = document + "_tomdb.idx";
    std::string dataFileName = document + "_tomdb.dat";

    std::string indexFilePath = "/.tomdb/" + indexFileName;
    std::string dataFilePath = "/.tomdb/" + dataFileName;

    index = std::make_unique<BTree<Key>>(indexFilePath, indexFileName, dataFilePath, dataFileName);
}

bool SelectOperator::next() {
    if (!hasNext) {
        return false;
    }

    std::string indexFileName = document + "_tomdb.idx";
    std::string dataFileName = document + "_tomdb.dat";

    std::string indexFilePath = "/.tomdb/" + indexFileName;
    std::string dataFilePath = "/.tomdb/" + dataFileName;

    index = std::make_unique<BTree<Key>>(indexFilePath, indexFileName, dataFilePath, dataFileName);

    currentOutput = index->get(lookupKey);

    hasNext = false;
    return true;
}

std::unique_ptr<Tuple> SelectOperator::getOutput() {
    return currentOutput->clone();
}

void SelectOperator::close() {

}

// ---- CreateDocumentOperator ---- //
void CreateDocumentOperator::open() {

}

bool CreateDocumentOperator::next() {
    /**
     * TODO: Check documentCreated, whether document exists in metadata. If exists, throw error. If not, create index and data file
     * Index file format: {document}_tomdb.idx
     * Data file format: {document}_tomdb.dat
     *
     * Then, update and flush metadata
     */
    if (metadata->containsDocument(document)) {
        return false;
    }

    // Create index and data files
    std::string indexFileName = document + "_tomdb.idx";
    std::string dataFileName = document + "_tomdb.dat";

    std::string indexFilePath = "/.tomdb/" + indexFileName;
    std::string dataFilePath = "/.tomdb/" + dataFileName;

    std::ofstream createdIndexFile(indexFilePath);
    std::ofstream createdDataFile(dataFilePath);

    metadata->addDocument(document);
    metadata->flush();

    return true;
}

void CreateDocumentOperator::close() {
    // close index and data files
}

std::unique_ptr<Tuple> CreateDocumentOperator::getOutput() {
    return nullptr;
}

// ---- DropDocumentOperator ---- //
void DropDocumentOperator::open() {

}

bool DropDocumentOperator::next() {
    /**
     * TODO: Check documentDropped. Check whether document exists in metadata. If not exists, throw error. If not, remove index and data file
     * Then, update and flush metadata
     *
     * Then, set documentDropped -> true;
     */
    return false;
}

void DropDocumentOperator::close() {
    // close index and data files
}

std::unique_ptr<Tuple> DropDocumentOperator::getOutput() {
    return nullptr;
}

// ---- PrintOperator ---- //
void PrintOperator::open() {
    input->open();
}

bool PrintOperator::next() {
    return input->next();
}

std::unique_ptr<Tuple> PrintOperator::getOutput() {
    auto output = input->getOutput();
    if (!output) {
        return nullptr;
    }

    std::ostringstream formatted;
    formatted << '{';
    for (size_t i = 0; i < output->getFieldCount(); ++i) {
        auto field = output->getField(i);
        if (i != 0) {
            formatted << ", ";
        }
        formatted << field->name << ": ";
        switch (field->type) {
            case INTEGER: {
                int value;
                std::memcpy(&value, field->value.get(), sizeof(value));
                formatted << value;
                break;
            }
            case FLOAT: {
                float value;
                std::memcpy(&value, field->value.get(), sizeof(value));
                formatted << value;
                break;
            }
            case STRING:
                formatted << std::quoted(std::string(field->value.get(), field->size));
                break;
            default:
                throw std::logic_error("Unknown field type");
        }
    }
    formatted << "}\n";
    std::cout << formatted.str();
    return output;
}

void PrintOperator::close() {
    input->close();
}
