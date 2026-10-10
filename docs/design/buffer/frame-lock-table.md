# Frame lock table

[Design guide](../README.md)

Sources: [frame_lock_table.h](../../../include/buffer/frame_lock_table.h) and [frame_lock_table.cpp](../../../src/buffer/frame_lock_table.cpp).

## Responsibility and state

`FrameLockTable` provides shared-reader/exclusive-writer coordination for each buffer frame. It is a page-access mechanism, not transaction-level concurrency control. The table contains `MAX_CACHED_PAGES` separately allocated `OwnershipInfo` objects, each with a 16-bit shared count, an exclusive flag, a mutex, and a condition variable.

Each frame has independent state. The per-frame mutex protects ownership bookkeeping; it is released after acquisition and is not held throughout the caller's page access. Logical ownership is represented by the count and exclusive flag until the caller unlocks.

## Acquisition and release

| Method | Rule |
| --- | --- |
| `lockShare(frameId, timeoutMillis)` | Wait while exclusive ownership exists, then increment shared count |
| `lockExclusive(frameId, timeoutMillis)` | Wait while exclusive ownership or any shared count exists, then set exclusive |
| `unlockShare(frameId)` | Reject exclusive state; decrement count and notify all when it reaches zero |
| `unlockExclusive(frameId)` | Reject nonexclusive state; clear exclusive and notify all |

All operations validate `frameId < MAX_CACHED_PAGES`, otherwise throwing `std::out_of_range`. Acquisitions default to a 1000 ms timeout. Each computes one deadline using `steady_clock` and repeatedly waits until that deadline, rechecking the predicate after wakeups. If the deadline expires while access remains blocked, it throws `std::logic_error("Frame lock timed out")`. This loop handles spurious wakeups and competing waiters.

The ownership rule is multiple readers or one writer for a frame. Notification wakes waiters to compete for the state mutex; it does not transfer ownership directly.

## Integration and limits

The [buffer manager](buffer-manager.md) reserves a pin before waiting on a cached frame. The pin prevents normal victim selection during the wait, while the lock table controls access to the contents. Neither mechanism substitutes for the other.

There is no thread identity tracking, recursive exclusive locking, upgrade/downgrade API, writer queue, fairness guarantee, or deadlock detection. New readers may enter while a writer waits, so writers can time out under sustained read traffic. Shared counts can overflow; unlocking a shared frame with zero readers underflows because there is no zero-count check. Correctly paired operations are required.

These locks do not coordinate file registration, all public flush paths, or transactions across pages. Cross-page lock ordering remains a caller concern.

## Existing validation

[Lock-table tests](../../../test/buffer/frame_lock_table_test.cpp) cover shared/exclusive state, incompatible unlocks, invalid IDs, timeout behavior, readers waiting for an exclusive owner, and competing writers after the final reader releases. They do not establish fairness or validate ownership by calling thread.
