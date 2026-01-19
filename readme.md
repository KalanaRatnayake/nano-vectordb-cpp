# Nano-VectorDB-cpp

Nano-VectorDB-CPP is a minimal C++ vector database ported from [gusye1234/nano-vectordb](https://github.com/gusye1234/nano-vectordb) focusing on:

- In-memory storage of fixed-length float vectors
- Simple upsert/query APIs
- Optional persistence (save/load)
- Simplified Stratergies for Stoarage, Serialization and Metrics

Core concepts:
- Data: the record containing an id, vector, and optional metadata.
- NanoVectorDB: manages storage, indexing/scan, and persistence.
- Similarity: queries return nearest neighbors by a distance metric (e.g., cosine or L2).

Typical flow:
1. Construct NanoVectorDB with a fixed dimension.
2. Upsert a batch of Data records.
3. Query with a vector and k to get top matches.
4. Save to disk; later load to restore state.

> **Note:** This main branch now contains only the C++ implementation. The upstream with Python-CPP mix is tracked on a separate branch `reference-python`.

## Requirements

- C++17 or later
- [Eigen](https://eigen.tuxfamily.org/) (vector math)
- [OpenSSL](https://www.openssl.org/) (base64 encoding)
- [nlohmann/json](https://github.com/nlohmann/json) (JSON serialization)

### Build & Test (CMake)

```bash
sudo apt-get update
sudo apt-get install -y g++ cmake libeigen3-dev libssl-dev nlohmann-json3-dev
```

### Build using CMake

```bash
mkdir build
cd build
cmake ..
cd ..
cmake --build build
```

### Run the test executable

```bash
./build/test_db
```

## Usage

```cpp
#include "NanoVectorDB.hpp"
#include <vector>
using namespace nano_vectordb;

NanoVectorDB vdb(1024);
std::vector<Data> data;
// Fill data with your vectors
vdb.upsert(data);
auto results = vdb.query(data[0].vector, 10);
vdb.save();
```

See src/test_db.cpp for full API usage and tests.

## Documentation

Class explanations are in the docs folder:
- [Database class, API and enum selection](./docs/NanoVectorDB.md)
- [Data record structure and serialization](./docs/Data.md)
- [Metrics: L2 and Cosine](./docs/metric.md)
- [Serializers: JSON and Base64](./docs/serializer.md)
- [Storage backends: File and MMap](./docs/storage.md)