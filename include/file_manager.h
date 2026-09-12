//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_FILE_MANAGER_H
#define TOMDB_FILE_MANAGER_H

#include <fstream>
#include <filesystem>

#include "page.h"

class FileManager {

private:
    size_t numPages = 0;
    std::fstream filestream;
    std::string filePath;

public:
    FileManager(const std::filesystem::path &filePath);

    ~FileManager() {
        if (filestream.is_open()) {
            filestream.close();
        }
    }

    /**
     * load the page, given <code>pageId</code>
     */
    std::unique_ptr<Page> load(uint16_t pageId);

    /**
     * Flush the input <code>page</code>, given the <code>pageId</code>
     */
    bool flush(uint16_t pageId, const Page &page);

    /**
     * Extend file by one page
     *
     */
    void extend();

    /**
     * Extend file page up to <code>maxPageId</code>
     */
    void extend(size_t maxPageId);

    size_t getNumPages() {
        return numPages;
    }
};

#endif //TOMDB_FILE_MANAGER_H
