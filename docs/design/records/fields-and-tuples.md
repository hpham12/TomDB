# Fields and tuples

[Design guide](../README.md)

Sources: [field.h](../../../include/records/field.h), [field.cpp](../../../src/records/field.cpp), [tuple.h](../../../include/records/tuple.h), and [tuple.cpp](../../../src/records/tuple.cpp).

## Responsibility and ownership

`Field` represents a named integer, float, or string. It owns its payload through `unique_ptr<char[]>` and exposes its name, type, payload, and length publicly. `Tuple` owns an ordered vector of fields and represents one serialized record without a schema dependency.

`Tuple::addField(unique_ptr<Field>)` consumes the supplied pointer and stores a clone. `getField(index)` returns a field clone and throws `std::out_of_range` for an invalid index. `getFieldCount()` returns the number of fields, and `Tuple::clone()` copies the entire tuple.

## Field format

```text
[native FieldType][uint16_t payload length][payload bytes][uint8_t name length][name bytes]
```

Type values are `INTEGER = 0`, `FLOAT = 1`, and `STRING = 2`. Integer and float constructors copy `sizeof(int)` and `sizeof(float)` bytes respectively. Serialized names and string values omit NUL terminators. `getSize()` includes the length prefixes, payload, and field name.

Under the tested ABI, a numeric field takes `11 + name length` bytes. A string field takes `7 + name length + value length` bytes.

Deserialization reads the type, length, and payload, then reconstructs a field using the corresponding constructor. Any type other than integer or float currently falls through to string construction.

## Tuple format

```text
[uint32_t total serialized size][field 0][field 1] ... [field n]
```

The total size includes the four-byte prefix. There is no field count: deserialization reads fields until accumulated serialized sizes reach the prefix value. For valid input this consumes exactly one tuple, allowing adjacent tuples in a stream. An empty tuple occupies four bytes.

The tests use an integer and a float named `name` and a string field with both name and value `Hello World`. This tuple takes `4 + 15 + 15 + 29 = 63` bytes under the tested ABI. A [page](pages.md) stores these bytes and tracks their location in a slot.

## Tradeoffs and limits

Self-describing fields allow decoding without a schema. Each record stores its own field names, types, and lengths. Cloning isolates ownership and adds allocations and copies.

The format is native-endian and ABI-dependent, despite the field header's little-endian comment. Decoders do not validate stream success, numeric payload sizes, unknown type tags, or exact tuple-size boundaries. They assume well-formed input produced by compatible code.

The low-level field API narrows string length to `uint16_t` and serialized name length to `uint8_t` without checking for overflow. The JSON insert parser validates those lengths before creating fields. Cloning a string field truncates its value at the first embedded NUL byte.

## Existing validation

[Field tests](../../../test/records/field_test.cpp) cover construction, expected serialized bytes, decoding, cloning, and size calculation for all three types. [Tuple tests](../../../test/records/tuple_test.cpp) cover serialization, field access, bounds errors, consecutive tuple decoding, empty tuples, and independent returned copies. These tests do not establish malformed-input handling or cross-platform format compatibility.
