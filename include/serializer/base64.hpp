#pragma once
#include <vector>
#include <cstdint>
#include "base.hpp"

namespace nano_vectordb
{
/**
 * @brief Base64 binary serializer implementation
 */
struct Base64BinarySerializer : public ISerializer
{
  /**
   * @brief Serialize a vector of Data records to a byte array.
   *        Stub implementation returns empty.
   */
  std::vector<uint8_t> toBytes(const std::vector<Data>&) const override
  {
    return {};
  }

  /**
   * @brief Deserialize a byte array to a vector of Data records.
   *        Stub implementation returns empty.
   */
  std::vector<Data> fromBytes(const std::vector<uint8_t>&) const override
  {
    return {};
  }
};

}  // namespace nano_vectordb
