//
// Created by Hieu Pham on 8/30/26.
//

#ifndef TOMDB_STORAGE_MANAGER_H
#define TOMDB_STORAGE_MANAGER_H

#include "page.h"

class StorageManager {
public:
    StorageManager();

    Page getPage(uint16_t pageId);
};

#endif //TOMDB_STORAGE_MANAGER_H
