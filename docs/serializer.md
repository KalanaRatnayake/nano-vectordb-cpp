# Serializer

This document describes how serialization works in nano-VectorDB and how to select a serializer strategy.

## JSON Serializer

- Format: JSON object containing:
  - `embedding_dim`: integer dimension of vectors
  - `matrix`: base64-encoded binary of all vectors (row-major float array)
  - `data`: array of records with at least `id`
- Implementation: header-only in [include/serializer/json.hpp](include/serializer/json.hpp)
- Helpers: uses [include/helper.hpp](include/helper.hpp) for array_to_buffer_string() and buffer_string_to_array()

### Encoding
- Builds an Eigen::MatrixXf from std::vector<Data>.
- Encodes the matrix to base64 via array_to_buffer_string().
- Emits JSON with embedding_dim, matrix, and data ids.

### Decoding
- Parses JSON bytes.
- Decodes matrix via buffer_string_to_array().
- Reconstructs std::vector<Data> by pairing data[i].id with matrix.row(i).

## Built-in Serializer Backends

- Interface: [include/serializer/base.hpp](../include/serializer/base.hpp)
- json: [include/serializer/json.hpp](../include/serializer/file.hpp)
- Base64: [include/serializer/base64.hpp](../include/serializer/mmap.hpp)

## Adding New Serializers

1. Create a header in `include/serializer/your_serializer.hpp`:
  - Derive from `nano_vectordb::ISerializer` and implement `toBytes(records)` and `fromBytes(bytes)`.
  - You can reuse helpers in [include/helper.hpp](include/helper.hpp) to encode/decode matrices.
2. Update the enum and factory:
  - Add a new value to `nano_vectordb::serializer` in [include/serializer/factory.hpp](include/serializer/factory.hpp).
  - Add a `make()` case that returns `std::make_shared<YourSerializer>()`.
3. Use it:
  - `db.initialize_serializer(nano_vectordb::serializer::YourSerializer)`.
