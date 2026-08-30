# Configurations

## Page
- In TomDB, a Page is the most basic unit of I/O between different layers of the architecture.

### Page size
- In order to play nicely with the OS page, TomDB page size should be the same as OS page size.
- Because the default Unix page size is 4kB, TomDB will have the same default page size.
- Page size can be tuned to match the OS page size.

### Page structure
- In order to efficiently support tuples of different lengths, TomDB uses slotted page structure

### Page usage
- A page is used to represent a node in the B-Tree index structure.
- With internal node, the page contains keys and pointers to child nodes
- With leaf node, the page contains keys and pointers to the actual tuples.

## Tuple
- A tuple corresponds to a row in a DB table.
- A tuple contains one of more Fields
- Tuple needs to support serialization/deserialization

## Field
- A field corresponds to a column in a DB table row
- Supported field types in TomDB are: Float, Integer, String
- Field needs to support serialization/deserialization
