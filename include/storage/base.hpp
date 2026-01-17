#pragma once
#include <string>
#include <vector>
#include <cstdint>

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

}  // namespace nano_vectordb
