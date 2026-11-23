# Technical debt

## High priority

0. **Audit generated code.** Review and clean up the node serializer, node deserializer, and JSON parser. [Node codec](../include/index/node_serializer.h) · [JSON parser](../src/query/insert_json.cpp)
1. **Find an existing page with room for a tuple.** Add a file-manager mechanism to locate a page where the tuple will fit. Inserts currently try the active data page before extending the file. This mechanism would let them reuse space in older pages. [File manager](design/storage/file-manager.md)
2. **Evaluate direct I/O.** Investigate bypassing the kernel's page cache to avoid caching the same pages in both the OS and the buffer pool. Measure the performance impact and account for platform support and I/O alignment requirements. [Buffer manager](design/buffer/buffer-manager.md)
3. **Fix B-tree page pinning.** Review the pinning logic so writes honor exclusive access. [B-tree implementation](../include/index/b_tree.tpp)
4. **Add B-tree deletion.** Implement deletion of individual records through the index. [B-tree implementation](../include/index/b_tree.tpp)
5. **Add B-tree range queries.** Implement lookups over a range of keys. [B-tree implementation](../include/index/b_tree.tpp)
6. **Add B-tree updates.** Implement updates to existing records through the index. [B-tree implementation](../include/index/b_tree.tpp)
7. **Refactor field and tuple serialization.** Define byte order for both encoding and decoding so the format works across little-endian and big-endian systems. [Fields and tuples](design/records/fields-and-tuples.md)
8. **Audit memory ownership.**
