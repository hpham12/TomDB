#include "file_manager.h"

#include <filesystem>

#include "constants.h"
#include "iostream"
#include "page.h"

//
// Created by Hieu Pham on 8/30/26.
//
FileManager::FileManager(const std::filesystem::path &filePath) {
    try {
        // 1. Extract the directory portion and create it if missing
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }

        if (!std::filesystem::exists(filePath)) {
            std::ofstream create(filePath);
        }
        filestream.open(filePath, std::fstream::in | std::fstream::out);

        if (!filestream.is_open()) {
            std::cerr << "Could not open file " << filePath << '\n';
            exit(1);
        }
    } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
        exit(1);
    }

    filestream.seekg(0, std::ios::end);
    std::streampos fileSize = filestream.tellg();

    if (filestream.fail() || fileSize == std::streampos(-1)) {
        std::cerr << "Could not determine file size for " << filePath << '\n';
        exit(1);
    }

    numPages = fileSize / PAGE_SIZE;
    if (numPages == 0) {
        extend();
    }
}

std::unique_ptr<Page> FileManager::load(uint16_t pageId) {
    if (pageId >= numPages) {
        throw std::out_of_range("Requested page not exist");
    }

    size_t offset = pageId * PAGE_SIZE;
    filestream.seekg(offset, std::ios::beg);
    auto page = std::make_unique<Page>();
    filestream.read(page->pageData.get(), PAGE_SIZE);

    return page;
}

bool FileManager::flush(uint16_t pageId, const Page &page) {
    if (pageId >= numPages) {
        std::cerr << "Requested page not exist" << '\n';
        return false;
    }

    size_t offset = pageId * PAGE_SIZE;
    filestream.seekp(offset, std::ios::beg);
    filestream.write(page.pageData.get(), PAGE_SIZE);
    filestream.flush();

    return true;
}

void FileManager::extend(size_t maxPageId) {
    if (maxPageId < numPages) {
        std::cerr << "maxPageId is smaller than the current number of pages" << '\n';
        return;
    }

    size_t numPagesToAdd = maxPageId - numPages + 1;

    for (size_t i = 0; i < numPagesToAdd; i++) {
        Page newPage;
        filestream.write(newPage.pageData.get(), PAGE_SIZE);
    }

    size_t pageOffset = numPages * PAGE_SIZE;

    filestream.seekp(pageOffset, std::ios::beg);
    filestream.flush();
    numPages += numPagesToAdd;
}

void FileManager::extend() {
    size_t pageOffset = numPages * PAGE_SIZE;
    Page newPage;
    filestream.seekp(pageOffset, std::ios::beg);
    filestream.write(newPage.pageData.get(), PAGE_SIZE);
    filestream.flush();
    numPages++;
}
