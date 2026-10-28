#ifndef TOMDB_NODE_SERIALIZER_H
#define TOMDB_NODE_SERIALIZER_H

#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "node.h"

// Version 1: integral keys only. All integers are little-endian; names are
// uint8-length-prefixed bytes (at most 8 bytes per name). Only populated entries are stored.
template<class K>
class NodeSerializer {
public:
    static constexpr size_t MAX_FILE_MANAGER_NAME_BYTES = NodeFormat::MAX_FILE_MANAGER_NAME_BYTES;

private:
    static_assert(std::is_integral_v<K> && !std::is_same_v<K, bool> && sizeof(K) <= 8,
                  "NodeSerializer requires an integral key of at most 64 bits (excluding bool)");
    using UnsignedKey = std::make_unsigned_t<K>;

    struct Writer {
        std::span<char> bytes;
        size_t offset = 0;

        void integer(uint64_t value, size_t width) {
            if (width > bytes.size() - offset) {
                throw std::length_error("Node exceeds PAGE_SIZE");
            }
            for (size_t i = 0; i < width; ++i) {
                bytes[offset++] = static_cast<char>(value & 0xff);
                value >>= 8;
            }
        }

        void pageId(const PageID &id) {
            if (id.fileManagerId.size() > MAX_FILE_MANAGER_NAME_BYTES) {
                throw std::length_error("File-manager name exceeds 8 bytes");
            }
            integer(id.fileManagerId.size(), NodeFormat::NAME_LENGTH_BYTES);
            for (unsigned char c: id.fileManagerId) {
                integer(c, 1);
            }
            // Encode the sentinel independently of the host's size_t width.
            integer(id.fileManagerPageId == std::numeric_limits<size_t>::max()
                        ? UINT64_MAX
                        : id.fileManagerPageId, NodeFormat::PAGE_NUMBER_BYTES);
        }
    };

    struct Reader {
        std::span<const char> bytes;
        size_t offset = 0;

        uint64_t integer(size_t width) {
            if (width > bytes.size() - offset) {
                throw std::invalid_argument("Truncated node");
            }
            uint64_t value = 0;
            for (size_t i = 0; i < width; ++i) {
                value |= uint64_t(static_cast<unsigned char>(bytes[offset++])) << (8 * i);
            }
            return value;
        }

        size_t sizeValue(bool allowSentinel = false) {
            const auto value = integer(8);
            if (allowSentinel && value == UINT64_MAX) {
                return std::numeric_limits<size_t>::max();
            }
            if (value > std::numeric_limits<size_t>::max()) {
                throw std::invalid_argument("Integer exceeds size_t range");
            }
            return value;
        }

        PageID pageId() {
            const auto length = integer(NodeFormat::NAME_LENGTH_BYTES);
            if (length > MAX_FILE_MANAGER_NAME_BYTES) {
                throw std::invalid_argument("File-manager name exceeds 8 bytes");
            }
            if (length > bytes.size() - offset) {
                throw std::invalid_argument("Truncated file-manager name");
            }
            std::string name(bytes.data() + offset, length);
            offset += length;
            return PageID{.fileManagerId = std::move(name), .fileManagerPageId = sizeValue(true)};
        }

        K key() {
            const auto value = static_cast<UnsignedKey>(integer(sizeof(K)));
            if constexpr (std::is_signed_v<K>) {
                if (value > static_cast<UnsignedKey>(std::numeric_limits<K>::max())) {
                    return static_cast<K>(-1 - static_cast<K>(static_cast<UnsignedKey>(~value)));
                }
            }
            return static_cast<K>(value);
        }
    };

    static void header(Writer &out, const Node &node, bool leaf, size_t capacity) {
        if (node.count > capacity || (leaf ? node.level != 0 : node.level == 0)) {
            throw std::invalid_argument("Invalid node level or count");
        }
        for (char c: std::string_view("TND1")) {
            out.integer(c, 1);
        }
        out.integer(1, 1); // Format version.
        out.integer(leaf ? 0 : 1, 1);
        out.integer(sizeof(K), 1);
        out.integer(std::is_signed_v<K> ? 1 : 0, 1);
        out.integer(node.level, 2);
        out.integer(node.count, 2);
        out.pageId(node.id);
        out.pageId(node.parentId);
    }

    static void header(Reader &in, Node &node, bool leaf, size_t capacity) {
        for (char c: std::string_view("TND1")) {
            if (in.integer(1) != static_cast<unsigned char>(c)) {
                throw std::invalid_argument("Invalid node magic");
            }
        }
        if (in.integer(1) != 1) {
            throw std::invalid_argument("Unsupported node version");
        }
        if (in.integer(1) != (leaf ? 0u : 1u)) {
            throw std::invalid_argument("Wrong node kind");
        }
        if (in.integer(1) != sizeof(K) || in.integer(1) != (std::is_signed_v<K> ? 1u : 0u)) {
            throw std::invalid_argument("Wrong key encoding");
        }
        node.level = static_cast<uint16_t>(in.integer(2));
        node.count = static_cast<uint16_t>(in.integer(2));
        if (node.count > capacity || (leaf ? node.level != 0 : node.level == 0)) {
            throw std::invalid_argument("Invalid node level or count");
        }
        node.id = in.pageId();
        node.parentId = in.pageId();
    }

public:
    using PageBytes = std::array<char, PAGE_SIZE>;

    static PageBytes serialize(const LeafNode<K> &node) {
        PageBytes bytes{};
        Writer out{bytes};
        header(out, node, true, node.kCapacity);
        out.pageId(node.next);
        for (size_t i = 0; i < node.count; ++i) {
            out.integer(static_cast<UnsignedKey>(node.keys[i]), sizeof(K));
            out.pageId(node.records[i].pageId);
            out.integer(node.records[i].slotIndex, NodeFormat::SLOT_INDEX_BYTES);
        }
        return bytes;
    }

    static PageBytes serialize(const InnerNode<K> &node) {
        PageBytes bytes{};
        Writer out{bytes};
        header(out, node, false, node.kCapacity);
        for (size_t i = 0; i < node.count; ++i) {
            out.integer(static_cast<UnsignedKey>(node.keys[i]), sizeof(K));
        }
        for (size_t i = 0; i <= node.count; ++i) {
            out.pageId(node.children[i]);
        }
        return bytes;
    }

    static LeafNode<K> deserializeLeaf(std::span<const char> bytes) {
        if (bytes.size() > PAGE_SIZE) {
            throw std::invalid_argument("Input exceeds PAGE_SIZE");
        }
        LeafNode<K> node;
        Reader in{bytes};
        header(in, node, true, node.kCapacity);
        node.next = in.pageId();
        for (size_t i = 0; i < node.count; ++i) {
            node.keys[i] = in.key();
            node.records[i].pageId = in.pageId();
            node.records[i].slotIndex = in.sizeValue();
            if (i && node.keys[i] < node.keys[i - 1]) {
                throw std::invalid_argument("Unsorted node keys");
            }
        }
        return node;
    }

    static InnerNode<K> deserializeInner(std::span<const char> bytes) {
        if (bytes.size() > PAGE_SIZE) {
            throw std::invalid_argument("Input exceeds PAGE_SIZE");
        }
        InnerNode<K> node;
        Reader in{bytes};
        header(in, node, false, node.kCapacity);
        for (size_t i = 0; i < node.count; ++i) {
            node.keys[i] = in.key();
            if (i && node.keys[i] < node.keys[i - 1]) {
                throw std::invalid_argument("Unsorted node keys");
            }
        }
        for (size_t i = 0; i <= node.count; ++i) {
            node.children[i] = in.pageId();
        }
        return node;
    }
};

#endif // TOMDB_NODE_SERIALIZER_H
