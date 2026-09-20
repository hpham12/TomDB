//
// Created by Hieu Pham on 9/20/26.
//

#ifndef TOMDB_BUFFER_FRAME_H
#define TOMDB_BUFFER_FRAME_H
#include <memory>

#include "commons.h"
#include "records/page.h"

class BufferFrame {
    std::unique_ptr<Page> page = nullptr;
    PageID pageId = INVALID_PAGE_ID;
    FrameID frameId = INVALID_VALUE;
    bool isDirty = false;

    friend class BufferManager;

public:
    BufferFrame() = default;
    void markDirty();
    void reset();
};

#endif //TOMDB_BUFFER_FRAME_H
