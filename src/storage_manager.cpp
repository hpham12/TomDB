//
// Created by Hieu Pham on 9/12/26.
//

#include "storage_manager.h"

#include <iostream>

StorageManager::StorageManager() {}

bool StorageManager::registerFileManager(std::string id, std::string filePath) {
     if (fileManagers.contains(id)) {
          return false;
     }

     fileManagers[id] = std::make_unique<FileManager>(filePath);
     return true;
}

std::unique_ptr<Page> StorageManager::getPage(PageID pageId) {
     if (!fileManagers.contains(pageId.fileManagerId)) {
          std::cerr << "ERROR: fileManager not registered" << std::endl;
          return nullptr;
     }
     auto &fileManager = fileManagers[pageId.fileManagerId];
     return fileManager->load(pageId.fileManagerPageId);
}

bool StorageManager::flushPage(PageID pageId, Page &page) {
     auto fileManagerId = pageId.fileManagerId;
     auto fileManagerPageId = pageId.fileManagerPageId;

     if (!fileManagers.contains(fileManagerId)) {
          std::cerr << "ERROR: fileManager not registered" << std::endl;
          return false;
     }

     auto &fileManager = fileManagers[fileManagerId];
     return fileManager->flush(fileManagerPageId, page);
}


void StorageManager::extend(const std::string &fileManagerId) {
     if (!fileManagers.contains(fileManagerId)) {
          std::cerr << "ERROR: fileManager not registered" << std::endl;
     }

     return fileManagers[fileManagerId]->extend();
}

/**
 * Extend file page up to <code>maxPageId</code>
 */
void StorageManager::extend(const std::string &fileManagerId, size_t maxPageId) {
     if (!fileManagers.contains(fileManagerId)) {
          std::cerr << "ERROR: fileManager not registered" << std::endl;
     }

     return fileManagers[fileManagerId]->extend(maxPageId);
}

/**
 * Get the current number of pages of a file, given the <code>fileManagerId</code>
 */
size_t StorageManager::getNumPages(const std::string &fileManagerId) {
     if (!fileManagers.contains(fileManagerId)) {
          std::cerr << "ERROR: fileManager not registered" << std::endl;
     }

     return fileManagers[fileManagerId]->getNumPages();
}