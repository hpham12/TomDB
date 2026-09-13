//
// Created by Hieu Pham on 9/12/26.
//

#include "storage_manager.h"

#include <iostream>

StorageManager::StorageManager() = default;

bool StorageManager::registerFileManager(const std::string& fileManagerId, const std::string& filePath) {
     if (fileManagers.contains(fileManagerId)) {
          return false;
     }

     fileManagers[fileManagerId] = std::make_unique<FileManager>(filePath);
     return true;
}

std::unique_ptr<Page> StorageManager::getPage(const PageID& pageId) {
     if (!fileManagers.contains(pageId.fileManagerId)) {
          throw std::logic_error("FileManager not registered");
     }
     auto &fileManager = fileManagers[pageId.fileManagerId];
     return fileManager->load(pageId.fileManagerPageId);
}

bool StorageManager::flushPage(PageID pageId, Page &page) {
     auto fileManagerId = pageId.fileManagerId;
     auto fileManagerPageId = pageId.fileManagerPageId;

     if (!fileManagers.contains(fileManagerId)) {
          throw std::logic_error("FileManager not registered");
     }

     auto &fileManager = fileManagers[fileManagerId];
     return fileManager->flush(fileManagerPageId, page);
}


void StorageManager::extend(const std::string &fileManagerId) {
     if (!fileManagers.contains(fileManagerId)) {
          throw std::logic_error("FileManager not registered");
     }

     fileManagers[fileManagerId]->extend();
}

void StorageManager::extend(const std::string &fileManagerId, size_t maxPageId) {
     if (!fileManagers.contains(fileManagerId)) {
          throw std::logic_error("FileManager not registered");
     }

     fileManagers[fileManagerId]->extend(maxPageId);
}

size_t StorageManager::getNumPages(const std::string &fileManagerId) {
     if (!fileManagers.contains(fileManagerId)) {
          throw std::logic_error("FileManager not registered");
     }

     return fileManagers[fileManagerId]->getNumPages();
}

std::unique_ptr<FileManager> &StorageManager::getFileManager(const std::string &fileManagerId) {
     if (!fileManagers.contains(fileManagerId)) {
          throw std::logic_error("FileManager not registered");
     }

     return fileManagers[fileManagerId];
}
