# Buffer manager

[Design guide](../README.md)

Sources: [buffer_manager.h](../../../include/buffer/buffer_manager.h) and [buffer_manager.cpp](../../../src/buffer/buffer_manager.cpp).

## Responsibility

`BufferManager` caches up to `MAX_CACHED_PAGES` pages, coordinates shared/exclusive access, prevents pinned pages from being selected for replacement, and writes dirty victims through [StorageManager](../storage/storage-manager.md). Callers acquire pages with `pinPage` and receive a move-only [PageGuard](frames-and-guards.md) that releases access at the end of its lifetime.

## State and intended invariants

| State | Role |
| --- | --- |
| `bufferPool` | Owns 15 separately allocated frames; vector indices are frame IDs |
| `pageToFrameMapping` | Maps resident page IDs to frames |
| `availableFrames` | IDs available for loading a page |
| `pinnedPages` | Page IDs excluded from victim selection |
| `pinCounters` | Atomic 16-bit counts of outstanding pin reservations per frame |
| `policy` | Tracks page access and selects unpinned victims |
| `frameLockTable` | Coordinates access to page contents |
| `storageManager` | Owns registered files and handles page I/O |
| `metadataMutex` | Protects pinning, replacement, and residency metadata |

On the successful paths, each resident page has one frame mapping, and each available frame is unmapped. A pin reservation is counted before a cached caller waits for its frame lock, keeping that page out of victim selection while the caller waits. Zero pin reservations make a page eligible for replacement. These are the intended metadata relationships; the release ordering and failure caveats below limit stronger concurrent guarantees.

Construction allocates all frames and makes their IDs available. A frame is chosen from an unordered set, so allocation order is unspecified. File registration is explicit; the manager forwards registration but discards StorageManager's duplicate-ID boolean result.

## Pinning a page

`pinPage(pageId, lockMode)` first takes the metadata mutex exclusively.

On a cache hit it updates replacement history, inserts the page into `pinnedPages`, and increments the pin count. It then releases the metadata mutex before waiting for a shared or exclusive frame lock. Success records the mode on the frame and returns a guard. A lock exception rolls back the pin count and removes the pinned-set entry if the count reaches zero; the access-history update remains.

On a cache miss it:

1. Evicts a victim if the replacement policy reports a full cache.
2. Selects an available frame, or throws `std::logic_error` if none exists.
3. Extends the registered file through the requested page number if necessary, then loads the page.
4. Records the access in the replacement policy and acquires the frame lock.
5. Installs the page and identifiers, initializes the dirty flag, adds the mapping, removes the frame from the available set, and increments the pin count.
6. Returns a guard.

The miss path holds the metadata mutex during extension, I/O, and lock acquisition. Pinning beyond the end allocates every intervening page, including for a shared pin. An unknown file ID raises `FileManagerNotRegisteredException` through the storage manager.

## Release and dirty tracking

Guard destruction or replacement by move assignment propagates its local dirty flag to the frame and calls private `unpinPage`. Unpinning decrements the counter under the metadata mutex and removes the page from `pinnedPages` at zero. It then releases the metadata mutex and unlocks the frame according to the frame's stored mode. An unlock exception restores the reservation and pinned-set entry. Unpinning a page absent from the pinned set throws `std::logic_error`.

Dirty marking is explicit: mutations through a guard do not automatically mark the page. A shared guard still exposes a mutable page, so callers must use exclusive mode for writes. Repinning a resident dirty page preserves its contents and dirty flag.

## Eviction and flushing

The policy selects and removes the first eligible victim from its history. If no victim exists, the manager throws `std::logic_error`; it does not wait for a pin to be released. A selected ID missing from the mapping causes an immediate return.

For a mapped victim, the manager flushes it if dirty, erases the mapping, resets the frame, and returns the frame ID to the available set. Clean victims require no write. [TwoQPolicy](replacement-policy.md) prefers pages seen once over pages accessed repeatedly.

Public `flushPage(pageId)` writes the cached page even if clean. It does not clear the dirty flag, acquire the metadata mutex or a frame lock, or check the storage layer's boolean result. It requires a resident page and externally coordinated access: lookup uses `operator[]`, so passing an uncached ID can insert an incorrect mapping instead of reporting a safe error.

There is no flush-all operation or destructor writeback. Dirty pages still resident at manager destruction are not automatically persisted. Stream flushes provide no crash-recovery guarantee.

## Concurrency and failure boundaries

`isPinned` uses a shared metadata lock. Registration, page-count reads, and public flushing do not take that lock; they must not race with conflicting registry or I/O operations. The underlying file streams are not independently synchronized.

Unpinning makes a page evictable before releasing its frame lock. Another thread can therefore select and reset that frame during the gap; the implementation does not make eligibility and lock release atomic. Lock mode is recorded on the frame rather than on each guard, making this ordering particularly important. The documentation does not claim that the current pin/lock protocol closes every concurrent eviction race.

The miss path updates policy history before lock acquisition, without undoing that update if locking fails. A prior eviction or file extension is also not rolled back. Victim selection removes policy history before writeback, and a false storage flush result is ignored. Thus failure paths do not provide transactional rollback of cache state or guaranteed retention after failed I/O. Pin counters have no overflow checks.

## Usage

```cpp
BufferManager manager;
manager.registerFileManager("data", "tomdb.data");
const PageID id{"data", 0};
{
    auto guard = manager.pinPage(id, EXCLUSIVE);
    auto tuple = std::make_unique<Tuple>();
    tuple->addField(std::make_unique<Field>(42));
    if (guard->addTuple(std::move(tuple), nullptr) != INVALID_VALUE) {
        guard.markDirty();
    }
    manager.flushPage(id); // Explicit write while this example owns access.
} // Dirty state propagates and the guard releases its pin.
```

This example assumes no competing manager operations. The manager must outlive every guard.

## Existing validation

[Buffer-manager tests](../../../test/buffer/buffer_manager_test.cpp) cover cache hits and misses, automatic extension, shared pins, conflicting-mode timeouts, reservation rollback, guard release and move construction, exhausted capacity, explicit flushing, dirty eviction, frame reuse, and preservation of dirty cached data on repinning. Several failure tests inject internal state; they do not establish complete exception safety or absence of concurrent eviction races.
