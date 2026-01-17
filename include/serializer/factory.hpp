#pragma once
#include <memory>
#include "json.hpp"
#include "base64.hpp"

namespace nano_vectordb
{
/**
 * @brief Enum-based selection helpers
 *
 * @param JSON JSON serializer implementation
 * @param Base64Binary Base64 binary serializer implementation
 */
enum class serializer
{
  JSON,
  Base64Binary
};

/**
 * @brief Factory function to create serializer strategy instances based on the specified type.
 *
 * @param t Serializer type.
 * @return std::shared_ptr<::nano_vectordb::ISerializer> Instance of the corresponding serializer strategy.
 */
inline std::shared_ptr<::nano_vectordb::ISerializer> make(serializer t)
{
  switch (t)
  {
    case serializer::JSON:
      return std::make_shared<struct ::nano_vectordb::JSONSerializer>();
    case serializer::Base64Binary:
      return std::make_shared<struct ::nano_vectordb::Base64BinarySerializer>();
  }
  return nullptr;
}
}  // namespace nano_vectordb