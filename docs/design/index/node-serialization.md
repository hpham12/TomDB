# Index node serialization

[Design guide](../README.md)

`NodeSerializer<K>` in [node_serializer.h](../../../include/index/node_serializer.h)
encodes leaf and inner nodes without copying C++ object layouts. The initial
format supports integral keys up to 64 bits, excluding `bool`. Prefer explicit
key types such as `int32_t` or `uint64_t`. Strings and floating-point keys require
additional codecs and are rejected at compile time.

[`BTree`](../../../include/index/b_tree.tpp) uses this format to create and reopen
indexes, split nodes during insertion, and traverse the tree for point lookups.
Leaf entries point to tuples in a separate data file.

## Usage

```cpp
#include "index/node_serializer.h"
#include <algorithm>
#include <span>

using Codec = NodeSerializer<int32_t>;
auto bytes = Codec::serialize(leafNode); // std::array<char, PAGE_SIZE>
auto guard = bufferManager.pinPage(indexPageId, EXCLUSIVE);
std::copy(bytes.begin(), bytes.end(), guard->pageData.get());
guard.markDirty();

// While holding a guard for reading:
auto restored = Codec::deserializeLeaf(
    std::span<const char>(guard->pageData.get(), PAGE_SIZE));
```

Use `serialize(innerNode)` and `deserializeInner(bytes)` for inner nodes.
Deserialization returns an independent object whose strings own their storage;
it does not retain references into the page buffer. It can also read a shorter
span containing the entire encoded payload. Full page images have zero-filled
unused bytes; the reader ignores trailing bytes.

## Version 1 format

All integers use little-endian byte order. Signed keys use a two's-complement
wire encoding; object padding and virtual-table pointers are never stored.

| Field | Encoding |
| --- | --- |
| Magic | Four ASCII bytes, `TND1` |
| Version | `uint8`, currently 1 |
| Kind | `uint8`, 0 for leaf, 1 for inner |
| Key width | `uint8`, bytes in the key encoding |
| Key signedness | `uint8`, 0 for unsigned, 1 for signed |
| Level | `uint16`, zero for leaves, positive for inner nodes |
| Entry count | `uint16` |
| Self page ID | Encoded page ID |
| Parent page ID | Encoded page ID |

A page ID is a `uint8` name length (maximum 8 bytes), exactly that many name bytes (no trailing
terminator), and a `uint64` file-local page number. `UINT64_MAX` denotes the
maximum-`size_t` page-number sentinel. Other values must fit the reader's
`size_t`. Names are serialized as bytes, including embedded nulls if present.

A leaf adds the next-leaf page ID, then `count` entries, each containing a key,
record page ID, and `uint64` slot index. An inner node adds `count` keys followed
by `count + 1` child page IDs, including one child for an empty inner node.

## Validation

Serialization rejects invalid levels/counts with `std::invalid_argument` and
oversized payloads or names longer than 8 bytes with `std::length_error`. Deserialization rejects
wrong magic/version/kind/key encoding, invalid levels/counts, truncated data,
unsorted keys, oversized input, and integers that cannot fit the runtime type.
Duplicate keys are permitted. This validates the encoding, not B-tree structural
relationships or whether referenced files/pages exist. There is no checksum.

## Capacity

[node_format.h](../../../include/index/node_format.h) defines the shared wire-size
constants. A page ID uses at most `1 + 8 + 8 = 17` bytes. Both node types reserve
`12 + 3 * 17 = 63` bytes: self and parent IDs plus a leaf link or the extra inner
child. Leaf entries use `sizeof(K) + 17 + 8` bytes; inner entries use
`sizeof(K) + 17` bytes. Thus:

```text
leaf capacity  = (PAGE_SIZE - 63) / (key bytes + 25)
inner capacity = (PAGE_SIZE - 63) / (key bytes + 17)
```

| Key type | Leaf entries | Inner keys | Inner children |
| --- | ---: | ---: | ---: |
| `int32_t` | 139 | 192 | 193 |
| `uint64_t` (document indexes) | 122 | 161 | 162 |

Capacity reserves space for maximum-length names. Shorter names leave unused
space. The reader requires this version's encoding and matching key width and
signedness. Rebuild indexes that use older layouts or longer file-manager IDs.

[Serializer tests](../../../test/index/node_serializer_test.cpp) cover format
round trips, capacity, and input validation. [B-tree tests](../../../test/index/b_tree_test.cpp)
exercise page splits, lookups, and reopening persisted indexes.
