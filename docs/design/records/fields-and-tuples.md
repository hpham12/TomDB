# Fields and tuples

[Design guide](../README.md)

Sources: [field.h](../../../include/records/field.h), [field.cpp](../../../src/records/field.cpp), [tuple.h](../../../include/records/tuple.h), and [tuple.cpp](../../../src/records/tuple.cpp).

## Responsibility and ownership

`Field` represents an integer, float, or string. It owns its payload through `unique_ptr<char[]>` and exposes its type, payload, and length publicly. `Tuple` owns an ordered vector of fields and represents one serialized row without a schema dependency.

`Tuple::addField(unique_ptr<Field>)` consumes the supplied pointer but stores a clone. `getField(index)` also returns a clone, so modifying a returned field does not modify the tuple. An invalid index throws `std::out_of_range`. There is no tuple field-update or field-removal API.

## Field format

```text
[native FieldType][uint16_t payload length][payload bytes]
```

Type values are `INTEGER = 0`, `FLOAT = 1`, and `STRING = 2`. Integer and float constructors copy `sizeof(int)` and `sizeof(float)` bytes respectively. String construction stores a trailing NUL in memory, but excludes it from the serialized payload length. `getSize()` returns the header plus payload size as `uint32_t`.

For the ABI assumed by the tests, numeric fields take 10 bytes and a string of length N takes `6 + N` bytes. Serialization writes binary data into a `std::string`; the string is a byte container, not a text encoding.

Deserialization reads the type, length, and payload, then reconstructs a field using the corresponding constructor. Any type other than integer or float currently falls through to string construction.

## Tuple format

```text
[uint32_t total serialized size][field 0][field 1] ... [field n]
```

The total size includes the four-byte prefix. There is no field count: deserialization reads fields until accumulated serialized sizes reach the prefix value. For valid input this consumes exactly one tuple, allowing adjacent tuples in a stream. An empty tuple occupies four bytes.

For example, an integer, a float, and `"Hello World"` take `4 + 10 + 10 + 17 = 41` bytes under the tested ABI. A [page](pages.md) stores these bytes unchanged and tracks their location in a slot.

## Tradeoffs and limits

Self-describing fields allow decoding without a schema, at the cost of repeating a type and length for every value. Cloning isolates tuple ownership but adds allocations and copies, including redundant clones during deserialization.

The format is native-endian and ABI-dependent, despite the field header's little-endian comment. Decoders do not validate stream success, numeric payload sizes, unknown type tags, or exact tuple-size boundaries. They assume well-formed input produced by compatible code.

String length is narrowed to `uint16_t` without overflow checking. String cloning uses a NUL-terminated constructor, so embedded NUL bytes are truncated when a field is cloned or added to a tuple. There is no NULL value, schema validation, overflow storage, or explicit numeric encoding conversion.

## Existing validation

[Field tests](../../../test/records/field_test.cpp) cover construction, expected serialized bytes, decoding, cloning, and size calculation for all three types. [Tuple tests](../../../test/records/tuple_test.cpp) cover serialization, field access, bounds errors, consecutive tuple decoding, empty tuples, and independent returned copies. These tests do not establish malformed-input handling or cross-platform format compatibility.
