#pragma once
#include "base.hpp"

namespace nano_vectordb
{

/**
 * @brief L2 (squared Euclidean) distance metric implementation
 */
struct L2Metric : public IMetric
{
	/**
	 * @brief Compute the L2 (squared Euclidean) distance between two vectors.
	 *
	 * @param a First vector.
	 * @param b Second vector.
	 * @return float L2 distance.
	 */
  float distance(const Eigen::VectorXf& a, const Eigen::VectorXf& b) const override
  {
    return (a - b).squaredNorm();
  }
};

}  // namespace nano_vectordb
