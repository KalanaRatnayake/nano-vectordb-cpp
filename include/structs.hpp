#pragma once
#include <string>
#include <Eigen/Dense>

namespace nano_vectordb
{

/**
 * @brief Data record structure
 */
struct Data
{
  std::string id;
  Eigen::VectorXf vector;
};

/**
 * @brief Query result structure
 *
 */
struct QueryResult
{
  Data data;
  float score;
};

}  // namespace nano_vectordb
