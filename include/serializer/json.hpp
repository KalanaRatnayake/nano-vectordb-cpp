#pragma once
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <Eigen/Dense>
#include "base.hpp"
#include "../helper.hpp"

namespace nano_vectordb
{

/**
 * @brief JSON serializer implementation
 */
struct JSONSerializer : public ISerializer
{
  std::vector<uint8_t> toBytes(const std::vector<Data>& records) const override
  {
    nlohmann::json j;
    int embedding_dim = 0;
    if (!records.empty())
      embedding_dim = static_cast<int>(records.front().vector.size());
    Eigen::MatrixXf mat(static_cast<int>(records.size()), embedding_dim);
    std::vector<nlohmann::json> data_json;
    for (size_t i = 0; i < records.size(); ++i)
    {
      const auto& r = records[i];
      if (embedding_dim == 0)
      {
        embedding_dim = static_cast<int>(r.vector.size());
        mat.resize(static_cast<int>(records.size()), embedding_dim);
      }
      if (r.vector.size() != embedding_dim)
        throw std::runtime_error("JSONSerializer::toBytes: record dim mismatch");
      mat.row(static_cast<int>(i)) = Eigen::Map<const Eigen::RowVectorXf>(r.vector.data(), r.vector.size());
      nlohmann::json entry;
      entry["id"] = r.id;
      data_json.push_back(entry);
    }
    j["embedding_dim"] = embedding_dim;
    j["matrix"] = array_to_buffer_string(mat);
    j["data"] = data_json;
    auto dumped = j.dump();
    return std::vector<uint8_t>(dumped.begin(), dumped.end());
  }

  std::vector<Data> fromBytes(const std::vector<uint8_t>& bytes) const override
  {
    if (bytes.empty()) return {};
    nlohmann::json j = nlohmann::json::parse(bytes.begin(), bytes.end());
    if (!j.contains("embedding_dim") || !j.contains("matrix") || !j.contains("data"))
      throw std::runtime_error("JSONSerializer::fromBytes: missing fields");
    int embedding_dim = j["embedding_dim"].get<int>();
    std::string matrix_b64 = j["matrix"].get<std::string>();
    auto mat = buffer_string_to_array(matrix_b64, embedding_dim);
    const auto& data_arr = j["data"];
    if ((int)data_arr.size() != mat.rows())
      throw std::runtime_error("JSONSerializer::fromBytes: data/matrix row mismatch");
    std::vector<Data> out;
    out.reserve(data_arr.size());
    for (size_t i = 0; i < data_arr.size(); ++i)
    {
      const auto& d = data_arr[i];
      if (!d.contains("id"))
        throw std::runtime_error("JSONSerializer::fromBytes: missing id");
      Data r;
      r.id = d["id"].get<std::string>();
      r.vector = mat.row(static_cast<int>(i));
      out.push_back(std::move(r));
    }
    return out;
  }
};

}  // namespace nano_vectordb
