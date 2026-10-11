//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_FILE_MANAGER_H
#define TOMDB_FILE_MANAGER_H

#include <fstream>
#include <filesystem>

#include "../records/page.h"

class FileManager {
    size_t numPages = 0;
    std::fstream filestream;
    std::mutex mutex;

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
    std::unique_ptr<Page> load(size_t pageId);

    /**
     * Flush the input <code>page</code>, given the <code>pageId</code>
     */
    bool flush(size_t pageId, const Page &page);

    /**
     * Extend file by one page
     *
     */
    void extend();

    /**
     * Extend file page up to <code>maxPageId</code>
     */
    void extend(size_t maxPageId);

    /**
     * Get the current numPages
     */
    size_t getNumPages();

    /**
     * Get an available pageId
     */
    size_t getAvailablePageId();
};

#endif //TOMDB_FILE_MANAGER_H
