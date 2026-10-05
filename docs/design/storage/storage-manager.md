# Storage manager

[Design guide](../README.md)

Sources: [storage_manager.h](../../../include/storage/storage_manager.h) and [storage_manager.cpp](../../../src/storage/storage_manager.cpp).

## Responsibility and state

`StorageManager` routes page operations to registered files. It owns an `unordered_map<string, unique_ptr<FileManager>>`; construction starts with an empty registry. A string ID is an application-selected name, not a filesystem path or an automatically assigned identifier.

The split between a registry and single-file managers lets callers address pages with a composite `PageID` while keeping file offsets and stream operations inside [FileManager](file-manager.md).

## Registration and routing

`registerFileManager(id, path)` constructs a file manager and returns true for a new ID. An existing ID returns false without replacing its manager. File creation and reopening follow the file-manager constructor's behavior.

| Method | Delegated operation |
| --- | --- |
| `getPage(PageID)` | `load(fileManagerPageId)` on the selected manager |
| `flushPage(PageID, Page&)` | `flush(fileManagerPageId, page)` |
| `extend(id)` | Add one page |
| `extend(id, maxPageId)` | Extend through an inclusive page ID |
| `getNumPages(id)` | Read that manager's page count |
| `getFileManager(id)` | Return a reference to its owning `unique_ptr` |

Every lookup method throws `FileManagerNotRegisteredException` for an unknown ID. Otherwise load bounds exceptions and flush results propagate from the file manager. Returned pages have independent ownership; the storage manager does not cache them.

## Ownership and limits

Registry ownership closes file streams when managers are destroyed. Registration is process-local: there is no catalog file, automatic discovery, unregister operation, or persisted ID-to-path mapping. Callers must register files again after restart.

IDs are checked for uniqueness, but paths are not canonicalized or deduplicated. Two IDs can open the same physical file with separate streams and page counts. `getFileManager` exposes the owning pointer by reference, so callers can replace or null it and invalidate assumptions made by routing methods.

The registry has no locks. Registration and access require external coordination when used across threads. It adds no transactions, durability guarantees, or I/O error handling beyond the file manager.

## Existing validation

[Storage-manager tests](../../../test/storage/storage_manager_test.cpp) cover multiple registrations, duplicate IDs, page access and flushing, both extension methods, page counts, and missing-manager exceptions. The persistence semantics are exercised further by the [file-manager tests](../../../test/storage/file_manager_test.cpp).
