//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_STORAGEMANAGER_H
#define TOMDB_STORAGEMANAGER_H
#include <string>
#include <unordered_map>

#include "file_manager.h"

class StorageManager {
private:
    std::unordered_map<std::string, std::unique_ptr<FileManager>> fileManagers;

public:
    StorageManager();

    bool registerFileManager(std::string id, std::unique_ptr<FileManager> fileManager);

    Page getPage(std::string fileManagerId, uint16_t pageId);

    bool flushPage(std::string fileManagerId, uint16_t pageId);
};

#endif //TOMDB_STORAGEMANAGER_H
