//
// Created by Hieu Pham on 9/20/26.
//

#ifndef TOMDB_BUFFER_FRAME_H
#define TOMDB_BUFFER_FRAME_H
#include <memory>

#include "commons.h"
#include "records/page.h"

class BufferFrame {
    PageID pageId = INVALID_PAGE_ID;
    FrameID frameId = INVALID_FRAME_ID;
    bool isDirty = false;

    friend class BufferManager;

public:
    std::unique_ptr<Page> page = nullptr;
    BufferFrame() = default;
    void markDirty();
    void reset();
    bool isPageDirty() const;
};

#endif //TOMDB_BUFFER_FRAME_H
