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

class SelectOperator : public Operator {
    Key lookupKey;
    std::unique_ptr<Tuple> currentOutput;
    std::shared_ptr<BTree<Key> > index;
    std::string document;
    std::unordered_set<std::string> fieldNames;
    std::shared_ptr<Metadata> metadata;
    bool hasNext = true;
    bool selectAll;
    std::string rootPath;

    FRIEND_TEST(SelectOperatorTest, TestE2ELifeCycle);
    FRIEND_TEST(QueryProcessorTest, ParseSelectQueryWithoutDisplay);
    FRIEND_TEST(PrintOperatorTest, TestGetOutput);

public:
    void open() override;

    bool next() override;

    std::unique_ptr<Tuple> getOutput() override;

    void close() override;

    SelectOperator(Key lookupKey, std::string document,
                   std::unordered_set<std::string> fieldNames, bool selectAll,
                   const std::shared_ptr<Metadata> &metadata,
                   const std::string &rootPath = DEFAULT_ROOT_PATH
    ) : lookupKey(lookupKey), document(std::move(document)), fieldNames(std::move(fieldNames)), selectAll(selectAll),
        metadata(metadata), rootPath(rootPath) {
    }

    bool isIndexValid() const {
        return index != nullptr;
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

    explicit PrintOperator(std::unique_ptr<Operator> input) : input(std::move(input)) {}

    ~PrintOperator() override = default;
};

class InsertOperator : public Operator {
    std::unique_ptr<Tuple> tupleToInsert;
    Key keyToInsert;
    std::string document;
    std::shared_ptr<BTree<Key>> index;
    bool inserted = false;
    std::shared_ptr<Metadata> metadata;

    FRIEND_TEST(QueryProcessorTest, ParseInsertQuery);

public:
    void open() override;

    bool next() override;

    std::unique_ptr<Tuple> getOutput() override;

    void close() override;

    InsertOperator(std::unique_ptr<Tuple> tupleToInsert,
                   Key key, std::string document,
                   const std::shared_ptr<Metadata> &metadata)
        : tupleToInsert(std::move(tupleToInsert)), keyToInsert(key),
          document(std::move(document)), metadata(metadata) {
    }

    [[nodiscard]] bool isIndexValid() const {
        return index != nullptr;
    }

    ~InsertOperator() override = default;
};

class CreateDocumentOperator : public Operator {
    std::string document;
    std::shared_ptr<Metadata> metadata;
    std::string rootPath;

    FRIEND_TEST(QueryProcessorTest, ParseCreateDocument);

public:
    void open() override;

    bool next() override;

    std::unique_ptr<Tuple> getOutput() override;

    void close() override;

    CreateDocumentOperator(std::string document,
                           const std::shared_ptr<Metadata> &metadata,
                           const std::string &rootPath = DEFAULT_ROOT_PATH)
        : document(std::move(document)), metadata(metadata), rootPath(rootPath) {
    }

    ~CreateDocumentOperator() override = default;
};

class DropDocumentOperator : public Operator {
    std::string document;
    std::shared_ptr<Metadata> metadata;
    std::string rootPath;

    FRIEND_TEST(QueryProcessorTest, ParseDropDocument);

public:
    void open() override;

    bool next() override;

    std::unique_ptr<Tuple> getOutput() override;

    void close() override;

    DropDocumentOperator(std::string document,
                         const std::shared_ptr<Metadata> &metadata,
                         const std::string &rootPath = DEFAULT_ROOT_PATH)
        : document(std::move(document)), metadata(metadata), rootPath(rootPath) {
    };

    ~DropDocumentOperator() override = default;
};

#endif //TOMDB_OPERATORS_H
