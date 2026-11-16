//
// Created by Hieu Pham on 11/13/26.
//

#ifndef TOMDB_OPERATORS_H
#define TOMDB_OPERATORS_H
#include <utility>
#include <vector>

#include "metadata.h"
#include "index/b_tree.h"
#include "records/tuple.h"

class Operator {
public:
    virtual ~Operator() = default;

    /** Initialize the operator */
    virtual void open() = 0;

    /** Progress to next tuple */
    virtual bool next() = 0;

    /** Get output */
    virtual std::unique_ptr<Tuple> getOutput() = 0;

    /** Clean up resources */
    virtual void close() = 0;
};

using Key = uint64_t;

class SelectOperator : public Operator {
    Key lookupKey;
    std::unique_ptr<Tuple> currentOutput;
    std::unique_ptr<BTree<Key> > index;
    std::string document;
    std::vector<std::string> fieldNames;
    bool hasNext = true;

public:
    void open() override;

    bool next() override;

    std::unique_ptr<Tuple> getOutput() override;

    void close() override;

    SelectOperator(Key lookupKey, std::string document, std::vector<std::string> fieldNames = {})
        : lookupKey(lookupKey), document(std::move(document)), fieldNames(std::move(fieldNames)) {
    }

    ~SelectOperator() override = default;
};

class PrintOperator : public Operator {
    std::unique_ptr<Operator> input;

public:
    void open() override;

    bool next() override;

    std::unique_ptr<Tuple> getOutput() override;

    void close() override;

    PrintOperator(std::unique_ptr<Operator> input) : input(std::move(input)) {}

    ~PrintOperator() override = default;
};

class InsertOperator : public Operator {
    std::unique_ptr<Tuple> tupleToInsert;
    Key keyToInsert;
    std::string document;
    std::unique_ptr<BTree<Key> > index;
    bool inserted = false;

public:
    void open() override;

    bool next() override;

    std::unique_ptr<Tuple> getOutput() override;

    void close() override;

    InsertOperator(std::unique_ptr<Tuple> tupleToInsert, Key key, std::string document)
        : tupleToInsert(std::move(tupleToInsert)), keyToInsert(key), document(std::move(document)) {
    }

    ~InsertOperator() override = default;
};

class CreateDocumentOperator : public Operator {
    std::string document;
    std::shared_ptr<Metadata> metadata;

public:
    void open() override;

    bool next() override;

    std::unique_ptr<Tuple> getOutput() override;

    void close() override;

    CreateDocumentOperator(std::string document, const std::shared_ptr<Metadata> &metadata) : document(std::move(document)), metadata(metadata) {}

    ~CreateDocumentOperator() override = default;
};

class DropDocumentOperator : public Operator {
    std::string document;
    bool documentDropped = false;
    std::shared_ptr<Metadata> metadata;

public:
    void open() override;

    bool next() override;

    std::unique_ptr<Tuple> getOutput() override;

    void close() override;

    DropDocumentOperator(std::string document, const std::shared_ptr<Metadata> &metadata) : document(std::move(document)),
        metadata(metadata) {
    };

    ~DropDocumentOperator() override = default;
};

#endif //TOMDB_OPERATORS_H
