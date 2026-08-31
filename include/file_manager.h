//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_FILE_MANAGER_H
#define TOMDB_FILE_MANAGER_H

#include <fstream>

#include "page.h"

class FileManager {

private:
    size_t numPages = 0;
    std::fstream file;
    std::string filePath;

public:
    FileManager(const std::string &filePath);

    ~FileManager() {
        if (file.is_open()) {
            file.close();
        }
    }

    Page load(uint16_t pageId);

    bool flush(uint16_t pageId, const Page &page);
};

#endif //TOMDB_FILE_MANAGER_H
