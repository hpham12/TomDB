# Technical debt

## High priority

1. **Audit generated code.** Review and clean up the node serializer, node deserializer, and JSON parser. [Node codec](../include/index/node_serializer.h) · [JSON parser](../src/query/insert_json.cpp)
2. **Add B-tree range queries.** Implement lookups over a range of keys. [B-tree implementation](../include/index/b_tree.tpp)
3. **Add B-tree updates.** Implement updates to existing records through the index. [B-tree implementation](../include/index/b_tree.tpp)
4. **Add B-tree deletion.** Implement deletion of individual records through the index. [B-tree implementation](../include/index/b_tree.tpp)
5. **Support look up by specific field**
6. **Fix B-tree page pinning.** Review the pinning logic so writes honor exclusive access. [B-tree implementation](../include/index/b_tree.tpp)
7. **Support index creation**. Allow create index for non-id field, and choose the index based on query.
8. **Find an existing page with room for a tuple.** Add a file-manager mechanism to locate a page where the tuple will fit. Inserts currently try the active data page before extending the file. This mechanism would let them reuse space in older pages. [File manager](design/storage/file-manager.md)
9. **Refactor field and tuple serialization.** Define byte order for both encoding and decoding so the format works across little-endian and big-endian systems. [Fields and tuples](design/records/fields-and-tuples.md)
10. **Audit memory ownership.**
11. **Evaluate direct I/O.** Investigate bypassing the kernel's page cache to avoid caching the same pages in both the OS and the buffer pool. Measure the performance impact and account for platform support and I/O alignment requirements. [Buffer manager](design/buffer/buffer-manager.md)
12. **Efficient space utilization.** How to save space by not persisting file name for each node?
