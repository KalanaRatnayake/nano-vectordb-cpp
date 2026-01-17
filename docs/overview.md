# Overview

nano-VectorDB is a minimal C++ vector database focused on:
- In-memory storage of fixed-length float vectors
- Simple upsert/query APIs
- Optional persistence (save/load)

Core concepts:
- Data: the record containing an id, vector, and optional metadata.
- NanoVectorDB: manages storage, indexing/scan, and persistence.
- Similarity: queries return nearest neighbors by a distance metric (e.g., cosine or L2).

Typical flow:
1. Construct NanoVectorDB with a fixed dimension.
2. Upsert a batch of Data records.
3. Query with a vector and k to get top matches.
4. Save to disk; later load to restore state.

See NanoVectorDB.md and Data.md for details.