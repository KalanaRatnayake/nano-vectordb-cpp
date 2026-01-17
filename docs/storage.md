# Storage

This document describes storage backends in nano-VectorDB and how to select or extend them.

## Built-in Storage Backends

- Interface: [include/storage/base.hpp](../include/storage/base.hpp)
- File: plain file I/O using `std::ifstream/std::ofstream`.
	- Header: [include/storage/file.hpp](../include/storage/file.hpp)
- MMap: memory-mapped I/O using `mmap`/`munmap` for fast bulk reads/writes.
	- Header: [include/storage/mmap.hpp](../include/storage/mmap.hpp)


## Selecting Storage via Enums

Use the factory and enum to choose a backend:
- Factory: [include/storage/factory.hpp](include/storage/factory.hpp)
- Enum: `nano_vectordb::storage { File, MMap }`

### Using in NanoVectorDB

- Initialize storage with a path:
	- initialize_storage: `db.initialize_storage(nano_vectordb::storage::File, "nano-vectordb.json")`
- When set, `save()` writes JSON bytes via the storage strategy; constructors read bytes via the storage and, if set, parse them via the serializer strategy.

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

