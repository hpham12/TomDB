//
// Created by Hieu Pham on 11/13/26.
//

#include "query/operators.h"

// ---- InsertOperator ---- //
void InsertOperator::open() {
    // TODO: Create BTree instance
}

bool InsertOperator::next() {
    // TODO: check hasNext, invoke BTree insert, then set hasNext to false

    return false;
}

std::unique_ptr<Tuple> InsertOperator::getOutput() {
    return nullptr;
}

void InsertOperator::close() {}

// ---- SelectOperator ---- //
void SelectOperator::open() {
    // TODO: Create BTree instance
}

bool SelectOperator::next() {
    // TODO: check hasNext, Invoke BTree select, then set hasNext to false

    return false;
}

std::unique_ptr<Tuple> SelectOperator::getOutput() {

    return nullptr;
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

    return false;
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
    // TODO: invoke input.getOutput(), then format for printing only
    return nullptr;
}

void PrintOperator::close() {
    input->close();
}
