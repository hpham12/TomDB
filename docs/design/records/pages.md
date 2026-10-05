# Slotted pages

[Design guide](../README.md)

Sources: [page.h](../../../include/records/page.h) and [page.cpp](../../../src/records/page.cpp).

## Responsibility and layout

`Page` owns exactly `PAGE_SIZE` bytes through `unique_ptr<char[]>`. It inserts and logically deletes serialized tuples. File I/O, cache residency, locking, and dirty tracking belong to other components.

```text
byte 0                                              byte 4096
| Slot[0] ... Slot[49] | serialized tuple regions ... |
|<--- metadata_size -->|
```

Each `Slot` contains `empty` (`bool`), `offset` (`size_t`, measured from the start of the page), and `size` (`size_t`). The directory is the native `Slot` representation overlaid on page bytes. `metadata_size` is `sizeof(Slot) * MAX_SLOTS`.

Construction initializes every slot to empty with invalid offset and size. Unused slots have maximum-valued size, which makes them eligible for the insertion scan. A page has no stored page ID, free-space counter, checksum, or tuple count.

## Insertion

`addTuple(unique_ptr<Tuple>, char* reason)` consumes the tuple and:

1. Computes the serialized tuple size.
2. Selects the first empty slot whose recorded size is at least that size.
3. Places slot zero at `metadata_size`; for another slot, computes its offset as the preceding slot's offset plus size.
4. Rejects an insertion extending past `PAGE_SIZE`.
5. Writes offset, actual tuple size, and occupied state, then copies serialized bytes into the page.

Success returns the slot index. Failure returns `INVALID_VALUE` without changing page contents. The two failure messages distinguish no eligible slot from insufficient space at the computed offset. A non-null `reason` must point to a sufficiently large writable buffer; the API takes no buffer capacity and does not clear it on success.

The fixed directory limits a page to 50 tuples even when payload space remains. A tuple must fit entirely within one page.

## Deletion and reuse

`deleteTuple(index)` returns false for an out-of-range or already empty slot. Otherwise it marks the slot empty and preserves its offset, size, and payload bytes. No disk operation occurs; a cached caller must mark its guard dirty to persist the change.

Insertion can reuse a deleted slot when its recorded size is large enough. It recomputes the offset from the preceding slot and replaces the recorded size with the new tuple's size. This is not a general free-space allocator: it does not compact records, merge holes, preserve allocation capacity separately, or search another candidate after a selected candidate fails the page-boundary check. Repeated variable-size reuse should not be assumed to provide general fragmentation or overlap safety.

## Access and tradeoffs

There is no tuple-fetch method. Current callers interpret the directory through `pageData`, select a live slot, and pass its payload bytes to `Tuple::deserialize`. Public raw bytes make persistence simple, but callers must preserve directory and payload consistency themselves. Slot indices may be reused after deletion; there is no generation identifier.

Metadata is stored inside the same byte buffer as tuples, so a full-page write persists both together. It is an ABI-dependent layout, with no corruption validation or transactional atomicity guarantee. See [configuration](../configuration.md) for capacity and compatibility assumptions.

## Existing validation

[Page tests](../../../test/records/page_test.cpp) cover insertion and decoding, payload exhaustion, slot exhaustion, deletion, invalid indices, exact page filling, unchanged bytes after failed insertion, and same-size reuse of a deleted slot without altering its neighbor. Arbitrary variable-size reuse sequences and corrupt slot metadata are not covered.
