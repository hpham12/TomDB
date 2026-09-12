//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_STORAGEMANAGER_H
#define TOMDB_STORAGEMANAGER_H
#include <string>
#include <unordered_map>

#include "file_manager.h"

struct PageID {
    std::string fileManagerId;
    uint16_t fileManagerPageId;
};

class StorageManager {
private:
    std::unordered_map<std::string, std::unique_ptr<FileManager>> fileManagers;

public:
    StorageManager();

    /**
     * Register a file manager, given its id and filePath
     */
    bool registerFileManager(std::string id, std::string filePath);

    /**
     * Get a page, using <code>PageID</code>
     */
    std::unique_ptr<Page> getPage(PageID pageId);

    /**
     * Flush a page, given its <code>PageID</code> and content
     */
    bool flushPage(PageID pageId, Page &page);

    /**
     * Extend file by one page, given the <code>fileManagerId</code>
     *
     */
    void extend(const std::string &fileManagerId);

    /**
     * Extend file page up to <code>maxPageId</code>
     */
    void extend(const std::string &fileManagerId, size_t maxPageId);

    /**
     * Get the current number of pages of a file, given the <code>fileManagerId</code>
     */
    size_t getNumPages(const std::string &fileManagerId);
};

#endif //TOMDB_STORAGEMANAGER_H
