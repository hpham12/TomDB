#include "file_manager.h"
#include "constants.h"
#include "iostream"

//
// Created by Hieu Pham on 8/30/26.
//
FileManager::FileManager(const std::string &filePath) {
    file.open(filePath, std::fstream::in | std::fstream::out | std::fstream::app);

    if (!file.is_open()) {
        std::cerr << "Could not open file " << DATABASE_FILE_NAME << std::endl;
        exit(1);
    }

    file.seekp (0, std::ios::end);
    size_t fileSize = file.tellg();

    this->numPages = fileSize / PAGE_SIZE;
}
