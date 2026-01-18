#pragma once
#include <memory>
#include "file.hpp"
#include "sqlite.hpp"

namespace nano_vectordb
{

/**
 * @brief Enum-based selection helpers
 *
 * @param File File-based storage backend
 * @param MMap Memory-mapped file storage backend
 */
enum class storage
{
  File,
  SQLite
};

/**
 * @brief Factory function to create storage strategy instances based on the specified type.
 *
 * @param t Storage type.
 * @return std::shared_ptr<::nano_vectordb::IStorage> Instance of the corresponding storage strategy.
 */
inline std::shared_ptr<::nano_vectordb::IStorage> make(storage t)
{
  switch (t)
  {
    case storage::File:
      return std::make_shared<struct ::nano_vectordb::FileStorage>();
    case storage::SQLite:
      return std::make_shared<struct ::nano_vectordb::SQLiteStorage>();
  }
  return nullptr;
}
}  // namespace nano_vectordb