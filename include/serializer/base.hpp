#pragma once
#include <cstdint>
#include <vector>
#include "../structs.hpp"

namespace nano_vectordb
{

// Uses Data from Data.hpp

/**
 * @brief Strategy interface for serialization
 */
struct ISerializer
{
  /**
   * @brief Virtual destructor for the serializer interface.
   */
  virtual ~ISerializer() = default;

  /**
   * @brief Serialize a vector of Data records to a byte array.
   *
   * @param records Vector of Data records to serialize.
   * @return std::vector<uint8_t> Serialized byte array.
   */
  virtual std::vector<uint8_t> toBytes(const std::vector<Data>& records) const = 0;

  /**
   * @brief Deserialize a byte array to a vector of Data records.
   *
   * @param bytes Byte array to deserialize.
   * @return std::vector<Data> Vector of deserialized Data records.
   */
  virtual std::vector<Data> fromBytes(const std::vector<uint8_t>& bytes) const = 0;
};

}  // namespace nano_vectordb