#pragma once
#include <Eigen/Dense>

namespace nano_vectordb
{

/**
 * @brief Strategy interface for similarity metrics
 */
struct IMetric
{
  /**
   * @brief Virtual destructor for the metric interface.
   */
  virtual ~IMetric() = default;

  /**
   * @brief Compute the distance between two vectors.
   *
   * @param a First vector.
   * @param b Second vector.
   *
   * @return float Distance between the two vectors.
   */
  virtual float distance(const Eigen::VectorXf& a, const Eigen::VectorXf& b) const = 0;
};

}  // namespace nano_vectordb
