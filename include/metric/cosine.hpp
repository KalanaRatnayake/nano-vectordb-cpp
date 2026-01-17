#pragma once
#include "base.hpp"

namespace nano_vectordb
{

// Cosine distance (1 - cosine similarity)
/**
 * @brief Cosine distance metric implementation
 */
struct CosineMetric : public IMetric
{
  /**
   * @brief Compute the Cosine distance between two vectors.
   *
   * @param a First vector.
   * @param b Second vector.
   *
   * @return float Cosine distance.
   */
  float distance(const Eigen::VectorXf& a, const Eigen::VectorXf& b) const override
  {
    const float denom = a.norm() * b.norm();
    if (denom == 0.0f)
      return 1.0f;
    float sim = a.dot(b) / denom;
    return 1.0f - sim;
  }
};

}  // namespace nano_vectordb
