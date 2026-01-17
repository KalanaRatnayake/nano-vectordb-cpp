# NanoVectorDB

Represents the database instance for fixed-dimension vectors.

Constructor:
- NanoVectorDB(dim)
  - dim: size of each vector (e.g., 1024)

Primary methods:
- upsert(std::vector<Data> batch)
  - Inserts or updates records by id.

- query(const Eigen::VectorXf& query, int top_k = 10, optional threshold, optional filter)
  - Returns top-k nearest neighbors using the selected metric.

- get(const std::vector<std::string>& ids)
  - Retrieves records by id.

- remove(const std::vector<std::string>& ids)
  - Removes records by id and compacts the matrix.

- save()
  - Persists the index/data to the configured storage file.

- get_additional_data() / store_additional_data(json)
  - Reads/writes extra metadata stored alongside vectors.

- size() const
  - Number of stored records.
  
- clear()
  - Removes all records.

## Selecting Strategies via Enums

You can choose serializer, storage, and metric strategies via enums and factory helpers.

- Serializer: [include/serializer/factory.hpp](../include/serializer/factory.hpp)
  - nano_vectordb::serializer { JSON, Base64Binary }
  - nano_vectordb::serializer::make(serializer) returns a strategy instance.

- Storage: [include/storage/factory.hpp](../include/storage/factory.hpp)
  - nano_vectordb::storage { File, MMap }
  - nano_vectordb::storage::make(storage) returns a strategy instance.

- Metric: [include/metric/factory.hpp](../include/metric/factory.hpp)
  - nano_vectordb::metric { L2, Cosine }
  - nano_vectordb::metric::make(metric) returns a strategy instance.

### Using in NanoVectorDB

- Metric:
  - initialize_metric: db.initialize_metric(nano_vectordb::metric::Cosine)
- Serializer:
  - initialize_serializer: db.initialize_serializer(nano_vectordb::serializer::JSON)
- Storage with path:
  - initialize_storage: db.initialize_storage(nano_vectordb::storage::File, "nano-vectordb.json")

When set, save() writes via the storage strategy. The constructor auto-loads from the default storage file (or your configured path) and uses the serializer strategy if provided.

Back to: [README](../readme.md)