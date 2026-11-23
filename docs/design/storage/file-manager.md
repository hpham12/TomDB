# File manager

[Design guide](../README.md)

Sources: [file_manager.h](../../../include/storage/file_manager.h) and [file_manager.cpp](../../../src/storage/file_manager.cpp).

## Responsibility

Each `FileManager` owns one `std::fstream` and its in-memory `size_t numPages`. File-local page IDs use `size_t` throughout loading, flushing, extension, and allocation. It maps zero-based page numbers to fixed-size file regions. [StorageManager](storage-manager.md) owns the registry of multiple file managers; tuple deletion belongs to [Page](../records/pages.md).

A file is a contiguous sequence of raw page images, without a file header:

```text
[page 0: 4096 bytes][page 1: 4096 bytes] ...
byte offset = pageId * PAGE_SIZE
```

## Construction and lifetime

Construction creates missing parent directories and opens or creates the file. It computes `numPages = fileSize / PAGE_SIZE` without allocating pages. A new file starts at zero pages. Destruction closes the stream.

Failure to open, filesystem exceptions during setup, or failure to determine file size prints an error and exits the process with status 1. The constructor does not report those failures through exceptions to its caller.

## Operations

| Operation | Behavior |
| --- | --- |
| `load(size_t pageId)` | Checks bounds, seeks, reads one page into a newly owned `Page`, and returns it |
| `flush(size_t pageId, const Page&)` | Checks bounds, overwrites one page, and flushes the stream |
| `extend()` | Writes one initialized page at the current end and increments the count |
| `extend(size_t maxPageId)` | Writes initialized pages through the requested inclusive page ID |
| `getAvailablePageId()` | Appends one initialized page and returns its `size_t` page ID |
| `getActivePageId()` | Returns the last allocated page ID or throws `std::out_of_range` for an empty file |
| `getNumPages()` | Returns the in-memory count |
| `refresh()` | Recomputes the count from the current file length |

An out-of-range load throws `std::out_of_range`. An out-of-range flush prints an error and returns false. A valid-range flush returns true without checking the subsequent stream state. `extend(10)` on an empty file creates eleven pages. If the requested ID already exists, extension logs a message and leaves the count unchanged.

Loading does not extend a file, and flushing cannot append. Automatic extension when pinning a new page is a [buffer-manager](../buffer/buffer-manager.md) behavior.

## Persistence and limits

Page reads return independent allocations. Modifying a loaded page does not update the file until a flush. Stream flushing transfers buffered output toward the OS but does not call `fsync` or provide a crash-durability guarantee. There is no WAL, checksum, atomic page-write protocol, free-page list, page deletion, or file shrink operation.

Reads, writes, and extensions do not check for short I/O or stream failures. Extension updates `numPages` even if a write fails. A trailing partial page is excluded from the count and can be overwritten by extension. Use `refresh()` to pick up external file-length changes.

The stream is opened without `std::ios::binary`, which assumes Unix text-mode behavior. A mutex serializes page I/O and page-count access within a manager. Callers must coordinate `refresh()` and access through separate managers for the same file.

## Existing validation

[File-manager tests](../../../test/storage/file_manager_test.cpp) cover empty and existing files, page I/O, bounds failures, extension, active-page lookup, refreshing after external allocation, and reopening a file with its contents preserved.
