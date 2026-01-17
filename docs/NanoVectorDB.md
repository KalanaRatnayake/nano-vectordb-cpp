# NanoVectorDB

Represents the database instance for fixed-dimension vectors.

Constructor:
- NanoVectorDB(dim)
  - dim: size of each vector (e.g., 1024)

Primary methods:
- upsert(std::vector<Data> batch)
  - Inserts or updates records by id.

- query(const std::vector<float>& vector, int k)
  - Returns top-k nearest neighbors for the given query vector.

- save(const std::string& path = default)
  - Persists the index/data to disk.

- load(const std::string& path)
  - Restores a previously saved database.

- size() const
  - Number of stored records.
  
- clear()
  - Removes all records.

Notes:
- All vectors must match the constructor dimension.
- Distance metric is fixed per build/implementation (commonly L2 or cosine).
- Thread-safety depends on usage; perform external synchronization if mutating concurrently.