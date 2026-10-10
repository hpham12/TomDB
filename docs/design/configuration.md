# Shared configuration and identifiers

[Design guide](README.md)

Source: [include/commons.h](../../include/commons.h).

## Configuration model

TomDB uses compile-time constants. There is no runtime configuration loader, OS page-size detection, or persisted configuration header. Changing a layout constant requires rebuilding and can make existing files incompatible.

| Constant or type | Current value | Meaning |
| --- | --- | --- |
| `PAGE_SIZE` | 4096 bytes | Unit of page allocation and file I/O |
| `MAX_SLOTS` | 50 | Fixed slot directory entries per page |
| `MAX_CACHED_PAGES` | 15 | Buffer frames, pin counters, and lock-table entries |
| `DATABASE_FILE_NAME` | `tomdb.data` | Defined filename constant; managers accept explicit paths |
| `INDEX_FILE_NAME` | `tomdb.index` | Defined filename constant; no implemented index component |
| `FrameID` | `uint16_t` | Index into the buffer pool |
| `INVALID_FRAME_ID` | 65535 | Unassigned frame marker |
| `INVALID_VALUE` | Maximum `uint64_t`, stored as `size_t` | Uninitialized slot offset/size and failed insertion marker |
| `LockMode` | `SHARED`, `EXCLUSIVE` | Requested frame access mode |

The 4096-byte database page is fixed independently of the host's OS page size. The buffer holds 61,440 bytes of page payload at capacity, plus frame, lock, container, and allocation overhead.

## Page identity

`PageID` combines a string `fileManagerId` with a `uint16_t fileManagerPageId`. Both participate in equality and hashing, so page zero in two registered files has distinct cache identities. File-local page numbers are zero-based and translate to byte offset `fileManagerPageId * PAGE_SIZE`.

`INVALID_PAGE_ID` is the pair `{"", 65535}`. There is no general sentinel validation at registration or pinning boundaries. Callers should avoid using that pair as a real page identity. A 16-bit page number addresses 65,536 pages (256 MiB at the current page size), although extension accepts `size_t` and does not enforce this addressability limit.

## Persistent format assumptions

Fields and tuples serialize native scalar representations. Pages persist the raw in-memory `Slot` array, including ABI-dependent layout and padding. The current byte-expectation tests assume little-endian, four-byte field enums, integers, and floats; serialization performs no byte-order conversion.

The slot directory occupies `sizeof(Slot) * MAX_SLOTS`. For an ABI with a 24-byte `Slot`, that is 1200 bytes, leaving 2896 bytes for tuple payload. This is an ABI-dependent calculation, not an explicitly encoded disk format.

There are no file-format versions, checksums, schema headers, or migration mechanisms. Files should be reused only with compatible layout constants and native representations. See [records](records/fields-and-tuples.md), [pages](records/pages.md), and [file manager](storage/file-manager.md) for the corresponding contracts and limits.
