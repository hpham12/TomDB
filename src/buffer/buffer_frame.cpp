//
// Created by Hieu Pham on 9/20/26.
//

#include "buffer/buffer_frame.h"

void BufferFrame::markDirty() {
    isDirty = true;
}

void BufferFrame::reset() {
    page = nullptr;
    pageId = INVALID_PAGE_ID;
    frameId = INVALID_FRAME_ID;
    isDirty = false;
}
