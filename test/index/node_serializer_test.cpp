#include <gtest/gtest.h>
#include <algorithm>
#include <vector>

#include "index/node_serializer.h"

using Codec = NodeSerializer<int32_t>;

TEST(NodeSerializerTest, LeafRoundTripPreservesRecordsAndSignedKeys) {
    LeafNode<int32_t> node;
    const Record low{.pageId={"data", 65536}, .slotIndex=3};
    const Record high{.pageId={"other", 9}, .slotIndex=4};
    ASSERT_TRUE(node.add(INT32_MIN, low));
    ASSERT_TRUE(node.add(INT32_MAX, high));
    const auto bytes = Codec::serialize(node);
    auto restored = Codec::deserializeLeaf(bytes);
    EXPECT_EQ(restored.getCount(), 2);
    EXPECT_EQ(restored.getRecord(INT32_MIN), low);
    EXPECT_EQ(restored.getRecord(INT32_MAX), high);
    EXPECT_EQ(restored.getId(), INVALID_PAGE_ID);
    EXPECT_EQ(restored.getParentId(), INVALID_PAGE_ID);
    EXPECT_EQ(Codec::serialize(restored), bytes);
    EXPECT_TRUE(restored.add(0, low)); // Restored object is usable, not a cast buffer.
}

TEST(NodeSerializerTest, InnerRoundTripPreservesChildrenAndKeys) {
    InnerNode<int32_t> node;
    ASSERT_TRUE(node.addChild(PageID{"left", 100}, 0));
    ASSERT_EQ(node.addKey(-10), 0);
    ASSERT_TRUE(node.addChild(PageID{"middle", 200}, 1));
    node.setCount(1);
    ASSERT_EQ(node.addKey(20), 1);
    ASSERT_TRUE(node.addChild(PageID{"right", 300}, 2));
    node.setCount(2);
    const auto bytes = Codec::serialize(node);
    auto restored = Codec::deserializeInner(bytes);
    EXPECT_FALSE(restored.isLeaf());
    EXPECT_EQ(restored.getLevel(), 1);
    EXPECT_EQ(restored.getCount(), 2);
    EXPECT_EQ(restored.getKeyIndex(-10), 1);
    EXPECT_EQ(restored.getKeyIndex(20), 2);
    EXPECT_EQ(Codec::serialize(restored), bytes);
}

TEST(NodeSerializerTest, EmptyAndFullNodesRoundTrip) {
    LeafNode<int32_t> leaf;
    InnerNode<int32_t> inner;
    EXPECT_EQ(Codec::serialize(Codec::deserializeLeaf(Codec::serialize(leaf))), Codec::serialize(leaf));
    EXPECT_EQ(Codec::serialize(Codec::deserializeInner(Codec::serialize(inner))), Codec::serialize(inner));
    for (uint16_t i = 0; i < leaf.getKCapacity(); ++i) {
        ASSERT_TRUE(leaf.add(i, Record{.pageId={"data", i}, .slotIndex=i}));
    }
    for (uint16_t i = 0; i < inner.getKCapacity(); ++i) {
        ASSERT_NE(inner.addKey(i), INVALID_INDEX);
        ASSERT_TRUE(inner.addChild(PageID{"index", i}, i + 1));
        inner.setCount(i + 1);
    }
    EXPECT_EQ(Codec::serialize(Codec::deserializeLeaf(Codec::serialize(leaf))), Codec::serialize(leaf));
    EXPECT_EQ(Codec::serialize(Codec::deserializeInner(Codec::serialize(inner))), Codec::serialize(inner));
}

TEST(NodeSerializerTest, ReadsGoldenLittleEndianLeafWithMetadataAndNextLink) {
    // Independently construct a page: version, kind, key width/sign, level, count.
    std::vector<char> golden{'T', 'N', 'D', '1', 1, 0, 4, 1, 0, 0, 1, 0};
    const auto integer = [&](uint64_t value, size_t width) {
        for (size_t i = 0; i < width; ++i) {
            golden.push_back(static_cast<char>(value & 255));
            value >>= 8;
        }
    };
    const auto pageId = [&](const std::string& name, uint64_t number) {
        integer(name.size(), 1);
        golden.insert(golden.end(), name.begin(), name.end());
        integer(number, 8);
    };
    pageId("index", 42); // Self.
    pageId("index", 10); // Parent.
    pageId("index", 43); // Next leaf.
    integer(UINT32_MAX, 4); // Key -1.
    pageId("data", 65536);
    integer(7, 8);
    auto restored = Codec::deserializeLeaf(golden);
    EXPECT_EQ(restored.getId(), (PageID{"index", 42}));
    EXPECT_EQ(restored.getParentId(), (PageID{"index", 10}));
    EXPECT_EQ(restored.getRecord(-1), (Record{.pageId={"data", 65536}, .slotIndex=7}));
    const auto encoded = Codec::serialize(restored);
    EXPECT_TRUE(std::equal(golden.begin(), golden.end(), encoded.begin()));
    EXPECT_TRUE(std::all_of(encoded.begin() + golden.size(), encoded.end(), [](char c) { return c == 0; }));
    for (size_t size = 0; size < golden.size(); ++size) {
        EXPECT_THROW(Codec::deserializeLeaf(std::span<const char>(golden.data(), size)), std::invalid_argument);
    }
}

TEST(NodeSerializerTest, RejectsMalformedHeadersAndWrongKeyType) {
    const auto valid = Codec::serialize(LeafNode<int32_t>{});
    for (size_t offset : {0u, 4u, 5u, 6u, 7u, 8u, 10u}) {
        auto bytes = valid;
        bytes[offset] = static_cast<char>(255);
        EXPECT_THROW(Codec::deserializeLeaf(bytes), std::invalid_argument) << offset;
    }
    EXPECT_THROW(Codec::deserializeInner(valid), std::invalid_argument);
    EXPECT_THROW(NodeSerializer<uint32_t>::deserializeLeaf(valid), std::invalid_argument);
    EXPECT_THROW(NodeSerializer<int64_t>::deserializeLeaf(valid), std::invalid_argument);
    std::vector<char> oversized(PAGE_SIZE + 1);
    EXPECT_THROW(Codec::deserializeLeaf(oversized), std::invalid_argument);
    auto bytes = valid;
    bytes[12] = static_cast<char>(255); // Impossible name length.
    EXPECT_THROW(Codec::deserializeLeaf(bytes), std::invalid_argument);
}

TEST(NodeSerializerTest, RejectsOversizedPayloadAndInvalidCount) {
    LeafNode<int32_t> leaf;
    ASSERT_TRUE(leaf.add(1, Record{.pageId={std::string(PAGE_SIZE, 'x'), 1}, .slotIndex=0}));
    EXPECT_THROW(Codec::serialize(leaf), std::length_error);
    InnerNode<int32_t> inner;
    inner.setCount(inner.getKCapacity() + 1);
    EXPECT_THROW(Codec::serialize(inner), std::invalid_argument);
}

TEST(NodeSerializerTest, SupportsUnsigned64BitAndByteKeys) {
    LeafNode<uint64_t> wide;
    const Record record{.pageId={"data", 1}, .slotIndex=0};
    ASSERT_TRUE(wide.add(UINT64_MAX, record));
    auto restored = NodeSerializer<uint64_t>::deserializeLeaf(NodeSerializer<uint64_t>::serialize(wide));
    EXPECT_EQ(restored.getRecord(UINT64_MAX), record);
    LeafNode<int8_t> narrow;
    ASSERT_TRUE(narrow.add(INT8_MIN, record));
    auto small = NodeSerializer<int8_t>::deserializeLeaf(NodeSerializer<int8_t>::serialize(narrow));
    EXPECT_EQ(small.getRecord(INT8_MIN), record);
}


TEST(NodeSerializerTest, AcceptsEightByteNamesAndRejectsLongerNames) {
    LeafNode<int32_t> leaf;
    const Record record{.pageId={"12345678", 1}, .slotIndex=0};
    ASSERT_TRUE(leaf.add(1, record));
    auto restored = Codec::deserializeLeaf(Codec::serialize(leaf));
    EXPECT_EQ(restored.getRecord(1), record);
    LeafNode<int32_t> tooLong;
    ASSERT_TRUE(tooLong.add(1, Record{.pageId={"123456789", 1}, .slotIndex=0}));
    EXPECT_THROW(Codec::serialize(tooLong), std::length_error);
    InnerNode<int32_t> inner;
    ASSERT_TRUE(inner.addChild(PageID{"123456789", 1}, 0));
    EXPECT_THROW(Codec::serialize(inner), std::length_error);
    auto malformed = Codec::serialize(LeafNode<int32_t>{});
    malformed[12] = 9;
    EXPECT_THROW(Codec::deserializeLeaf(malformed), std::invalid_argument);
}

TEST(NodeSerializerTest, RejectsTruncatedAndInvalidInnerNodes) {
    InnerNode<int32_t> node;
    ASSERT_TRUE(node.addChild(PageID{"left", 1}, 0));
    ASSERT_EQ(node.addKey(10), 0);
    ASSERT_TRUE(node.addChild(PageID{"right", 2}, 1));
    node.setCount(1);
    const auto bytes = Codec::serialize(node);
    // 12-byte prefix, two empty-name page IDs, one key, two named children.
    constexpr size_t payloadSize = 12 + 9 + 9 + 4 + 13 + 14;
    for (size_t size = 0; size < payloadSize; ++size) {
        EXPECT_THROW(Codec::deserializeInner(std::span<const char>(bytes.data(), size)), std::invalid_argument);
    }
    auto invalid = bytes;
    invalid[8] = 0; // An inner node cannot have level zero.
    EXPECT_THROW(Codec::deserializeInner(invalid), std::invalid_argument);
    invalid = bytes;
    invalid[10] = invalid[11] = static_cast<char>(255);
    EXPECT_THROW(Codec::deserializeInner(invalid), std::invalid_argument);
}

TEST(NodeSerializerTest, RejectsUnsortedEncodedKeys) {
    LeafNode<int32_t> node;
    const Record record{.pageId={"data", 1}, .slotIndex=0};
    ASSERT_TRUE(node.add(10, record));
    ASSERT_TRUE(node.add(20, record));
    auto bytes = Codec::serialize(node);
    constexpr size_t secondKeyOffset = 12 + 3 * 9 + 4 + 13 + 8;
    std::fill_n(bytes.begin() + secondKeyOffset, 4, 0); // Second key is now 0 < 10.
    EXPECT_THROW(Codec::deserializeLeaf(bytes), std::invalid_argument);
}

namespace {
Codec::PageBytes emptyNodeWithMaximumLengthNames(bool leaf) {
    Codec::PageBytes bytes{};
    const std::array<char, 12> prefix{'T', 'N', 'D', '1', 1,
        static_cast<char>(leaf ? 0 : 1), 4, 1, static_cast<char>(leaf ? 0 : 1), 0, 0, 0};
    std::copy(prefix.begin(), prefix.end(), bytes.begin());
    size_t offset = prefix.size();
    for (uint8_t number = 1; number <= 3; ++number) {
        bytes[offset++] = 8;
        for (char c : std::string_view("12345678")) {
            bytes[offset++] = c;
        }
        bytes[offset] = static_cast<char>(number);
        offset += 8;
    }
    return bytes;
}
}

TEST(NodeSerializerTest, FullLeafWithMaximumLengthNamesFitsSerializedBudget) {
    // Independent wire-size arithmetic: 63 bytes metadata, 29 bytes per int32 entry.
    constexpr size_t capacity = (PAGE_SIZE - 63) / 29;
    ASSERT_EQ(LeafNode<int32_t>::getKCapacity(), capacity);
    EXPECT_LE(63 + capacity * 29, PAGE_SIZE);
    EXPECT_GT(63 + (capacity + 1) * 29, PAGE_SIZE);
    auto node = Codec::deserializeLeaf(emptyNodeWithMaximumLengthNames(true));
    for (size_t i = 0; i < capacity; ++i) {
        ASSERT_TRUE(node.add(static_cast<int32_t>(i), Record{.pageId={"12345678", i}, .slotIndex=i}));
    }
    const auto bytes = Codec::serialize(node);
    auto restored = Codec::deserializeLeaf(bytes);
    EXPECT_EQ(restored.getCount(), capacity);
    EXPECT_EQ(Codec::serialize(restored), bytes);
    EXPECT_FALSE(node.add(999, Record{}));
}

TEST(NodeSerializerTest, FullInnerWithExtraChildAndMaximumLengthNamesFitsSerializedBudget) {
    // Metadata includes the extra child: 63 bytes fixed, 21 bytes per int32 entry.
    constexpr size_t capacity = (PAGE_SIZE - 63) / 21;
    ASSERT_EQ(InnerNode<int32_t>::getKCapacity(), capacity);
    EXPECT_LE(63 + capacity * 21, PAGE_SIZE);
    EXPECT_GT(63 + (capacity + 1) * 21, PAGE_SIZE);
    auto node = Codec::deserializeInner(emptyNodeWithMaximumLengthNames(false));
    for (uint16_t i = 0; i < capacity; ++i) {
        ASSERT_EQ(node.addKey(i), i);
        ASSERT_TRUE(node.addChild(PageID{"12345678", i}, i + 1));
        node.setCount(i + 1);
    }
    const auto bytes = Codec::serialize(node);
    auto restored = Codec::deserializeInner(bytes);
    EXPECT_EQ(restored.getCount(), capacity);
    EXPECT_EQ(Codec::serialize(restored), bytes);
    EXPECT_EQ(node.addKey(999), INVALID_INDEX);
}

TEST(NodeSerializerTest, CapacityUsesEncodedKeyWidth) {
    EXPECT_EQ(LeafNode<int8_t>::getKCapacity(), (PAGE_SIZE - 63) / 26);
    EXPECT_EQ(InnerNode<int8_t>::getKCapacity(), (PAGE_SIZE - 63) / 18);
    EXPECT_EQ(LeafNode<uint64_t>::getKCapacity(), (PAGE_SIZE - 63) / 33);
    EXPECT_EQ(InnerNode<uint64_t>::getKCapacity(), (PAGE_SIZE - 63) / 25);
}
