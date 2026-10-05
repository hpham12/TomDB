# Buffer frames and page guards

[Design guide](../README.md)

Sources: [buffer_frame.h](../../../include/buffer/buffer_frame.h), [buffer_frame.cpp](../../../src/buffer/buffer_frame.cpp), [page_guard.h](../../../include/buffer/page_guard.h), and [page_guard.cpp](../../../src/buffer/page_guard.cpp).

## Buffer frame

A `BufferFrame` owns an optional `Page` and stores its page ID, frame ID, dirty flag, and atomic exclusive-mode flag. BufferManager owns frames for its entire lifetime; replacing a page resets and reuses the frame object.

A fresh or reset frame has a null page, invalid identifiers, a false dirty flag, and a false exclusive flag. `markDirty()` sets the dirty flag; `isPageDirty()` reads it. `reset()` discards the page and restores the initial state. Reset does not flush data or release locks; the manager is responsible for those operations before reuse.

The page pointer is public. The remaining state is managed by friends `BufferManager` and `PageGuard`. A frame does not contain its own pin count or mutex; those are held by the manager and lock table.

## Guard ownership

`PageGuard` is a scoped access token returned by `BufferManager::pinPage`. It stores non-owning pointers to the manager and frame and a local dirty flag. It is not copyable, so ordinary copying cannot cause duplicate unpins. The manager must outlive its guards, and page/frame pointers obtained from a guard must not be used after release.

Dereference and arrow operators expose the cached page. Const access returns a const page. `getPageId`, `getFrameId`, and `getFrame` expose identity and the underlying frame; boolean conversion checks only whether the frame pointer is non-null.

## Lifetime transitions

| Event | Behavior |
| --- | --- |
| Construct from manager/frame | Store pointers with local dirty flag initially false |
| `markDirty()` | Set the local flag; defer marking the frame |
| Destroy active guard | Mark the frame if needed, then unpin through the manager |
| Move construct | Transfer pointers and dirty state; clear the source |
| Move assign | Release the destination's old pin, then transfer and clear the source |
| Self move assign | Leave the guard unchanged |

A moved-from guard has null pointers, false dirty state, invalid identifiers, and false boolean conversion. Its arrow operator returns null; dereferencing it is invalid. There is no default constructor, although the public pointer constructor permits an empty guard such as `PageGuard(nullptr, nullptr)`.

The pointer constructor does not acquire a lock or increment a pin count. Normal callers should obtain active guards through `pinPage` rather than construct tokens for arbitrary frames.

## Cleanup and limits

Destructor and move-assignment cleanup catch `std::logic_error` from unpinning and print it to standard error. Cleanup failure is not returned to the caller; move assignment still transfers the incoming token. If the manager restores a reservation after failed unlocking, that reservation can remain without a usable guard.

Both shared and exclusive guards expose the same mutable interface. Lock-mode discipline and explicit dirty marking are caller responsibilities. The guard is not a transaction, does not roll back changes on exceptions, and does not write pages to disk when destroyed. Raw `getFrame()` access can bypass its dirty-tracking convention.

## Existing validation

[Frame tests](../../../test/buffer/buffer_frame_test.cpp) cover dirty marking and reset. [Guard tests](../../../test/buffer/page_guard.cpp) cover accessors, const access, empty and moved-from state, move construction and assignment, self-move, destruction with dirty propagation, and cleanup failure during assignment. [Buffer-manager tests](../../../test/buffer/buffer_manager_test.cpp) exercise guards with real pinning and release.
