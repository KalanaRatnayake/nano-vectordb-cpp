#pragma once
#include <memory>
#include "l2.hpp"
#include "cosine.hpp"

namespace nano_vectordb
{
/**
 * @brief Enum for selecting metric types
 *
 * @param L2 L2 (squared Euclidean) distance metric
 * @param Cosine Cosine distance metric
 */
enum class metric
{
  L2,
  Cosine
};

/**
 * @brief Factory function to create metric strategy instances based on the specified type.
 *
 * @param t Metric type.
 * @return std::shared_ptr<::nano_vectordb::IMetric> Instance of the corresponding metric strategy.
 */
inline std::shared_ptr<::nano_vectordb::IMetric> make(metric t)
{
  switch (t)
  {
    case metric::L2:
      return std::make_shared<::nano_vectordb::L2Metric>();
    case metric::Cosine:
      return std::make_shared<::nano_vectordb::CosineMetric>();
  }
  return nullptr;
}
}  // namespace nano_vectordb