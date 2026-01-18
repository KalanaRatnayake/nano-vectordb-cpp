#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "../structs.hpp"

namespace nano_vectordb
{

/**
 * @brief Strategy interface for storage backends
 */
struct IStorage
{
  /**
   * @brief Virtual destructor for the storage interface.
   */
  virtual ~IStorage() = default;

  /**
   * @brief Write a byte array to the specified storage path.
   *
   * @param path Storage path.
   * @param bytes Byte array to write.
   */
  virtual void write(const std::string& path, const std::vector<uint8_t>& bytes) const = 0;

  /**
   * @brief Read a byte array from the specified storage path.
   *
   * @param path Storage path.
   * @return std::vector<uint8_t> Byte array read from storage.
   */
  virtual std::vector<uint8_t> read(const std::string& path) const = 0;
};

/**
 * @brief Structure to hold loaded records and metadata.
 */
struct StorageLoad
{
  std::vector<Data> records;
  nlohmann::json additional = nlohmann::json::object();
  int embedding_dim = 0;
};

/**
 * @brief Optional interface for row-wise storage backends.
 *
 * Backends implementing this can persist records in native formats (e.g., SQLite tables)
 * without requiring a serializer. If not present, NanoVectorDB falls back to byte-oriented
 * storage via `IStorage` and (optionally) a serializer.
 */
struct IStorageRecords : public IStorage
{
  virtual ~IStorageRecords() = default;

  /**
   * @brief Write all records and metadata to the storage path.
   */
  virtual void write_records(const std::string& path, const std::vector<Data>& records, int embedding_dim,
                             const nlohmann::json& additional) const = 0;

  /**
   * @brief Read all records and metadata from the storage path.
   */
  virtual StorageLoad read_records(const std::string& path) const = 0;
};

}  // namespace nano_vectordb
