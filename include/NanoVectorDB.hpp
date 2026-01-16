#pragma once
#include "helper.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <optional>

namespace nano_vectordb
{

/**
 * @brief Data structure representing a vector entry
 *
 */
struct Data
{
  std::string id;
  Vector vector;
};

using ConditionLambda = std::function<bool(const Data&)>;

/**
 * @brief A simple in-memory vector database supporting upsert, get, remove, and query operations.
 *
 */
class NanoVectorDB
{
public:
  struct QueryResult
  {
    Data data;
    Float score;
  };

  /**
   * @brief Construct a new Nano Vector DB object
   *
   * @param embedding_dim Embedding dimension
   * @param metric Similarity metric to use ("cosine" supported)
   * @param storage_file Storage file path
   */
  NanoVectorDB(int embedding_dim, const std::string& metric = "cosine",
               const std::string& storage_file = "nano-vectordb.json")
    : embedding_dim_(embedding_dim), metric_(metric), storage_file_(storage_file)
  {
    // Try to load storage if file exists
    auto loaded = load_storage(storage_file_, embedding_dim_);
    if (loaded)
    {
      // Parse loaded["data"] into data_ and loaded["matrix"] into matrix_
      // Data is expected as array of objects with id and vector (flattened)
      matrix_ = loaded.value()["matrix"].get<Matrix>();
      if (loaded.value().contains("data"))
      {
        for (const auto& d : loaded.value()["data"])
        {
          Data entry;
          entry.id = d["id"];
          // For simplicity, skip vector field (should be reconstructed from matrix)
          data_.push_back(entry);
        }
      }
      if (loaded.value().contains("additional_data"))
      {
        additional_data_ = loaded.value()["additional_data"];
      }
      if (loaded.value().contains("embedding_dim"))
      {
        int loaded_dim = loaded.value()["embedding_dim"];
        if (loaded_dim != embedding_dim_)
        {
          std::cerr << "Embedding dim mismatch, expected: " << embedding_dim_
                    << ", but loaded: " << loaded_dim << std::endl;
          throw std::runtime_error("Embedding dim mismatch");
        }
      }
      pre_process();
      std::cout << "Init NanoVectorDB with " << data_.size() << " data" << std::endl;
    }
    else
    {
      matrix_ = Matrix(0, embedding_dim_);
    }
  }

  /**
   * @brief Pre-process the matrix (e.g., normalize for cosine)
   */
  void pre_process()
  {
    if (metric_ == "cosine")
    {
      if (matrix_.rows() > 0)
        matrix_ = normalize(matrix_);
    }
  }

  /**
   * @brief Upsert data entries into the database.
   *
   * @param datas Vector of Data entries to upsert.
   */
  void upsert(const std::vector<Data>& datas)
  {
    std::unordered_map<std::string, Data> index_datas;
    for (const auto& data : datas)
    {
      std::string id = data.id.empty() ? hash_vector(data.vector) : data.id;
      index_datas[id] = data;
    }
    if (metric_ == "cosine")
    {
      for (auto& [id, d] : index_datas)
      {
        d.vector = normalize(d.vector);
      }
    }
    std::unordered_set<std::string> updated;
    for (size_t i = 0; i < data_.size(); ++i)
    {
      auto it = index_datas.find(data_[i].id);
      if (it != index_datas.end())
      {
        data_[i] = it->second;
        matrix_.row(i) = it->second.vector;
        updated.insert(it->first);
      }
    }
    for (const auto& [id, d] : index_datas)
    {
      if (updated.count(id) == 0)
      {
        data_.push_back(d);
        matrix_.conservativeResize(matrix_.rows() + 1, embedding_dim_);
        matrix_.row(matrix_.rows() - 1) = d.vector;
      }
    }
  }

  /**
   * @brief Retrieve data entries by their IDs.
   *
   * @param ids Vector of IDs to retrieve.
   * @return std::vector<Data> Retrieved data entries.
   */
  std::vector<Data> get(const std::vector<std::string>& ids) const
  {
    std::vector<Data> result;
    for (const auto& d : data_)
    {
      if (std::find(ids.begin(), ids.end(), d.id) != ids.end())
      {
        result.push_back(d);
      }
    }
    return result;
  }

  /**
   * @brief Remove data entries by their IDs.
   */
  void remove(const std::vector<std::string>& ids)
  {
    std::unordered_set<std::string> id_set(ids.begin(), ids.end());
    std::vector<Data> new_data;
    std::vector<int> keep_indices;
    for (size_t i = 0; i < data_.size(); ++i)
    {
      if (id_set.count(data_[i].id) == 0)
      {
        new_data.push_back(data_[i]);
        keep_indices.push_back(i);
      }
    }
    data_ = std::move(new_data);
    Matrix new_matrix(keep_indices.size(), embedding_dim_);
    for (size_t i = 0; i < keep_indices.size(); ++i)
    {
      new_matrix.row(i) = matrix_.row(keep_indices[i]);
    }
    matrix_ = std::move(new_matrix);
  }

  /**
   * @brief Perform a similarity query.
   *
   * @param query Input query vector.
   * @param top_k Number of top results to return.
   * @param better_than_threshold Optional threshold to filter results.
   * @param filter Optional filter function to apply on data entries.
   * @return std::vector<QueryResult>
   */
  std::vector<QueryResult> query(const Vector& query, int top_k = 10,
                                 std::optional<Float> better_than_threshold = std::nullopt,
                                 ConditionLambda filter = nullptr) const
  {
    if (metric_ == "cosine")
    {
      return cosine_query(query, top_k, better_than_threshold, filter);
    }
    return {};
  }

  /**
   * @brief Get the number of data entries in the database.
   *
   * @return int Number of data entries.
   */
  int size() const
  {
    return data_.size();
  }

  /**
   * @brief Save the database to a JSON file.
   *
   */
  void save() const
  {
    nlohmann::json storage;
    storage["embedding_dim"] = embedding_dim_;
    storage["matrix"] = array_to_buffer_string(matrix_);
    // Serialize data_
    std::vector<nlohmann::json> data_json;
    for (const auto& d : data_)
    {
      nlohmann::json entry;
      entry["id"] = d.id;
      // Optionally, serialize vector as well
      data_json.push_back(entry);
    }
    storage["data"] = data_json;
    if (!additional_data_.is_null())
    {
      storage["additional_data"] = additional_data_;
    }
    std::ofstream f(storage_file_);
    f << storage.dump();
  }

  /**
   * @brief Get additional stored data.
   *
   * @return nlohmann::json Additional data.
   */
  nlohmann::json get_additional_data() const
  {
    return additional_data_;
  }

  /**
   * @brief Store additional data.
   *
   * @param data Additional data to store.
   */
  void store_additional_data(const nlohmann::json& data)
  {
    additional_data_ = data;
  }

private:
  /**
   * @brief Perform a cosine similarity query.
   *
   * @param query Input query vector.
   * @param top_k Number of top results to return.
   * @param better_than_threshold Optional threshold to filter results.
   * @param filter Optional filter function to apply on data entries.
   * @return std::vector<QueryResult> Query results.
   */
  std::vector<QueryResult> cosine_query(const Vector& query, int top_k,
                                        std::optional<Float> better_than_threshold,
                                        ConditionLambda filter) const
  {
    Vector q = normalize(query);
    std::vector<int> indices;
    if (filter)
    {
      for (size_t i = 0; i < data_.size(); ++i)
      {
        if (filter(data_[i]))
          indices.push_back(i);
      }
    }
    else
    {
      indices.resize(data_.size());
      std::iota(indices.begin(), indices.end(), 0);
    }
    std::vector<std::pair<int, Float>> scored;
    for (int idx : indices)
    {
      Float score = matrix_.row(idx).dot(q);
      scored.emplace_back(idx, score);
    }
    std::sort(scored.begin(), scored.end(), [](auto& a, auto& b) { return a.second > b.second; });
    std::vector<QueryResult> results;
    for (int i = 0; i < std::min(top_k, (int)scored.size()); ++i)
    {
      if (better_than_threshold && scored[i].second < *better_than_threshold)
        break;
      results.push_back({ data_[scored[i].first], scored[i].second });
    }
    return results;
  }

  int embedding_dim_;
  std::string metric_;
  std::string storage_file_;
  std::vector<Data> data_;
  Matrix matrix_;
  nlohmann::json additional_data_ = nlohmann::json::object();
};

}  // namespace nano_vectordb
