# File manager

[Design guide](../README.md)

Sources: [file_manager.h](../../../include/storage/file_manager.h) and [file_manager.cpp](../../../src/storage/file_manager.cpp).

## Responsibility

Each `FileManager` owns one `std::fstream` and its in-memory `numPages`. It maps zero-based page numbers to fixed-size file regions. [StorageManager](storage-manager.md) owns the registry of multiple file managers; tuple deletion belongs to [Page](../records/pages.md).

A file is a contiguous sequence of raw page images, without a file header:

```text
[page 0: 4096 bytes][page 1: 4096 bytes] ...
byte offset = pageId * PAGE_SIZE
```

## Construction and lifetime

Construction creates missing parent directories and creates the file if absent. It opens the file for input and output, seeks to its end, and computes `numPages = fileSize / PAGE_SIZE`. If that count is zero, it writes one initialized empty page. Existing valid files retain their contents and page count. Destruction closes the stream.

Failure to open, filesystem exceptions during setup, or failure to determine file size prints an error and exits the process with status 1. The constructor does not report those failures through exceptions to its caller.

## Operations

| Operation | Behavior |
| --- | --- |
| `load(uint16_t pageId)` | Checks bounds, seeks, reads one page into a newly owned `Page`, and returns it |
| `flush(uint16_t pageId, const Page&)` | Checks bounds, overwrites one page, and flushes the stream |
| `extend()` | Writes one initialized page at the current end and increments the count |
| `extend(size_t maxPageId)` | Writes initialized pages through the requested inclusive page ID |
| `getNumPages()` | Returns the in-memory count |

An out-of-range load throws `std::out_of_range`; an out-of-range flush prints an error and returns false. A valid-range flush returns true without checking the subsequent stream state. `extend(10)` on a one-page file adds ten pages and results in eleven pages. If the requested ID already exists, extension logs a message and leaves the count unchanged.

Loading does not extend a file, and flushing cannot append. Automatic extension when pinning a new page is a [buffer-manager](../buffer/buffer-manager.md) behavior.

## Persistence and limits

Page reads return independent allocations. Modifying a loaded page does not update the file until a flush. Stream flushing transfers buffered output toward the OS but does not call `fsync` or provide a crash-durability guarantee. There is no WAL, checksum, atomic page-write protocol, free-page list, page deletion, or file shrink operation.

Reads, writes, and extensions do not check for short I/O or stream failures. Extension updates `numPages` even if a write fails. File lengths not divisible by `PAGE_SIZE` are not rejected; integer division ignores a trailing partial page, and extension can overwrite that tail. The count is not refreshed for external file changes.

The stream is opened without `std::ios::binary`; the current Unix environment does not apply text-mode translation, but this is another portability constraint. The manager has no synchronization around its shared seek positions or stream state. Callers must serialize operations on the same manager.

## Existing validation

[File-manager tests](../../../test/storage/file_manager_test.cpp) cover new and existing files, loading and overwriting pages, bounds failures, both extension methods, initialized appended pages, and reopening a file while preserving an earlier written page. They do not simulate disk failures, torn writes, or malformed file lengths.
