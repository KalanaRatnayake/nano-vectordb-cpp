# Storage

This document describes storage backends in nano-VectorDB and how to select or extend them.

## Built-in Storage Backends

- Interface: [include/storage/base.hpp](../include/storage/base.hpp)
- File: plain file I/O using std::ifstream/std::ofstream.
	- Header: [include/storage/file.hpp](../include/storage/file.hpp)
- SQLite: persists records row-wise in SQLite tables (`vectors`, `meta`).
	- Header: [include/storage/sqlite.hpp](../include/storage/sqlite.hpp)

### SQLite Schema
- `meta(key TEXT PRIMARY KEY, value TEXT NOT NULL)`
	- Stores metadata as simple key/value pairs.
	- Keys used by NanoVectorDB:
		- `embedding_dim`: the embedding dimension as a stringified integer
		- `additional_data`: a JSON string representing any user-provided extra data
- `vectors(id TEXT PRIMARY KEY, dim INTEGER NOT NULL, vec BLOB NOT NULL)`
	- `id`: unique identifier of the vector (either provided or derived)
	- `dim`: integer dimension used for validation
	- `vec`: raw bytes of the `float` array for the vector (Eigen::VectorXf)

On save, NanoVectorDB writes all current records to `vectors` and upserts metadata into `meta`. On load, it reconstructs `Data` entries from `vectors` and reads `embedding_dim` and `additional_data` from `meta`.


## Selecting Storage via Enums

Use the factory and enum to choose a backend:
- Factory: [include/storage/factory.hpp](include/storage/factory.hpp)
- Enum: `nano_vectordb::storage { File, SQLite }`

### Using in NanoVectorDB

- Initialize storage with a path:
	- File: `db.initialize_storage(nano_vectordb::storage::File, "nano-vectordb.json")`
	- SQLite: `db.initialize_storage(nano_vectordb::storage::SQLite, "nano-vectordb.sqlite")`
- Or pass a strategy directly via constructor:
	- `auto s = nano_vectordb::make(nano_vectordb::storage::SQLite);`
	- `NanoVectorDB db(dim, "cosine", "nano-vectordb.sqlite", nullptr, s);`
- `save()` writes using row-wise records when the strategy supports `IStorageRecords` (SQLite). Otherwise, it writes a single JSON file (File).

### Using in MultiTenant

- Set default storage for new tenants:
	- `multi.set_default_storage(nano_vectordb::storage::File)`
- Tenant file paths follow `storage_dir + "/" + nanovdb_<tenant_id>.json`.

## Adding New Storage Backends

1. Create a header in `include/storage/your_backend.hpp`:
	 - Derive from `nano_vectordb::IStorage` and implement `write(path, bytes)` and `read(path)`.
2. Update the enum and factory:
	 - Add a new value to `nano_vectordb::storage` in [include/storage/factory.hpp](include/storage/factory.hpp).
	 - Add a `make()` case that returns `std::make_shared<YourBackend>()`.
3. Use it:
	 - `db.initialize_storage(nano_vectordb::storage::YourBackend, "/path/to/file")`.

### Persistence Model
NanoVectorDB either:
- Uses `IStorageRecords` backends (SQLite) to read/write records directly.
- Or writes/reads a single JSON file via File storage with internal JSON formatting.

