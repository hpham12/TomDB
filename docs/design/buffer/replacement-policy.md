# Two-queue replacement policy

[Design guide](../README.md)

Sources: [policy.h](../../../include/buffer/policy.h), [two_queue_policy.h](../../../include/buffer/two_queue_policy.h), and [two_queue_policy.cpp](../../../src/buffer/two_queue_policy.cpp).

## Purpose and interface

`Policy` separates replacement history from page I/O and frame ownership. It exposes `accessPage`, explicit `evictPage`, `selectPageToEvict`, and `isCacheFull`. BufferManager constructs `TwoQPolicy` as its concrete policy; there is no public policy injection API.

`TwoQPolicy` uses two vectors of page IDs: FIFO for pages accessed once, and LRU for pages accessed repeatedly. Both vectors run from oldest at the front to newest at the back. Their combined capacity is `MAX_CACHED_PAGES`.

The design aims to preserve repeatedly accessed pages during sequential scans: a newly encountered scan page enters FIFO and is preferred for eviction before a page in LRU. This is a two-queue implementation without a ghost/history queue or separately configured queue capacities.

## Access transitions

| Existing location | Action on `accessPage(id)` |
| --- | --- |
| LRU | Remove and append to LRU's most-recent end |
| FIFO | Remove from FIFO and append to LRU |
| Neither, capacity available | Append to FIFO |
| Neither, cache full | Leave both queues unchanged |

An access does not evict another page. The manager must make room before recording a new resident page. History tracks pin attempts, including a cached attempt whose frame lock later times out; it is not a count of completed reads.

## Victim selection

`selectPageToEvict(pinnedPages)` scans FIFO from oldest to newest and selects the first ID absent from the pinned set. If none qualifies, it scans LRU in the same order. It removes the selected ID from its vector before returning it. With empty queues or only pinned candidates it returns `INVALID_PAGE_ID` and removes nothing.

Selection is therefore destructive, not a peek. The manager must coordinate removal from residency metadata and any dirty-page writeback; the policy has no rollback facility. Explicit `evictPage(id)` removes a named page from either queue, with no effect if absent and no pinned-set check.

For example, accessing A, B, then A leaves FIFO `[B]` and LRU `[A]`. With neither pinned, selection removes B. If B is pinned, selection can remove A while retaining B.

## Complexity and synchronization

Vector searches and erases are linear in the number of tracked pages. This is a simple representation for the current 15-page cache, but does not provide constant-time replacement operations at larger capacities.

Access, explicit removal, and selection take an internal mutex. `isCacheFull` and testing helpers `isInFifo`/`isInLru` do not lock; standalone callers must serialize these reads against changes. BufferManager calls its policy operations under the metadata lock on the normal pin/eviction paths.

The policy knows no page contents, dirty state, frame IDs, or I/O outcomes. Pinned protection comes solely from the set passed at selection time; callers are responsible for its accuracy and synchronization.

## Existing validation

[Policy tests](../../../test/buffer/two_queue_policy_test.cpp) cover admission, promotion, LRU refresh, full-cache behavior, explicit removal, FIFO-first selection, LRU fallback, pinned-page exclusion, empty/all-pinned results, and capacity becoming available after eviction.
