#pragma once
#include "helper.hpp"
#include "structs.hpp"
#include "metric/base.hpp"
#include "metric/factory.hpp"
#include "storage/base.hpp"
#include "storage/factory.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <memory>
#include <functional>
#include <optional>
#include <algorithm>
#include <numeric>

namespace nano_vectordb
{

/**
 * @brief A simple in-memory vector database supporting upsert, get, remove, and query operations.
 *
 */
class NanoVectorDB
{
public:
  /**
   * @brief Construct a new Nano Eigen::VectorXf DB object
   *
   * @param embedding_dim Embedding dimension
   * @param metric Similarity metric to use ("cosine" supported)
   * @param storage_file Storage file path
   */
  NanoVectorDB(int embedding_dim, const std::string& metric = "cosine",
               const std::string& storage_file = "nano-vectordb.json")
    : embedding_dim_(embedding_dim), metric_(metric), storage_file_(storage_file)
  {
    NVDB_LOG("[NanoVectorDB::NanoVectorDB] embedding_dim=" << embedding_dim_ << ", metric=" << metric_
                                                           << ", storage_file=" << storage_file_);
    // Use storage + serializer strategies if provided, fallback to default file loading
    std::optional<nlohmann::json> loaded;
    std::vector<Data> loaded_records;
    if (storage_strategy_)
    {
      try
      {
        // Prefer row-wise storage if available
        if (auto rs = std::dynamic_pointer_cast<IStorageRecords>(storage_strategy_))
        {
          auto lr = rs->read_records(storage_file_);
          if (!lr.records.empty() || lr.embedding_dim > 0)
          {
            loaded_records = std::move(lr.records);
            loaded = nlohmann::json{ {"embedding_dim", lr.embedding_dim}, {"matrix", ""}, {"data", nlohmann::json::array()} };
            additional_data_ = lr.additional;
          }
        }
        else
        {
          auto bytes = storage_strategy_->read(storage_file_);
          if (!bytes.empty())
          {
            nlohmann::json j = nlohmann::json::parse(bytes.begin(), bytes.end());
            loaded = j;
          }
        }
      }
      catch (...)
      {
        loaded = load_storage(storage_file_, embedding_dim_);
      }
    }
    else
    {
      loaded = load_storage(storage_file_, embedding_dim_);
    }
    if (loaded)
    {
      const auto& val = loaded.value();
      if (!val.contains("matrix"))
      {
        throw std::runtime_error("Storage file missing 'matrix' field");
      }
      if (!loaded_records.empty())
      {
        // Rebuild from serializer-decoded records
        data_ = loaded_records;
        matrix_.resize((int)data_.size(), embedding_dim_);
        for (size_t i = 0; i < data_.size(); ++i)
        {
          if (data_[i].vector.size() != embedding_dim_)
          {
            throw std::runtime_error("Loaded record dim mismatch");
          }
          matrix_.row((int)i) =
              Eigen::Map<const Eigen::RowVectorXf>(data_[i].vector.data(), data_[i].vector.size());
        }
      }
      else
      {
        std::string matrix_b64 = val["matrix"];
        matrix_ = buffer_string_to_array(matrix_b64, embedding_dim_);
        if (!val.contains("data"))
        {
          throw std::runtime_error("Storage file missing 'data' field");
        }
        for (const auto& d : val["data"])
        {
          if (!d.contains("id"))
          {
            throw std::runtime_error("Data entry missing 'id' field");
          }
          Data entry;
          entry.id = d["id"];
          // Vector will align with matrix rows by index
          entry.vector = matrix_.row((int)data_.size());
          data_.push_back(entry);
        }
      }
      if (val.contains("additional_data"))
      {
        additional_data_ = val["additional_data"];
      }
      if (val.contains("embedding_dim"))
      {
        int loaded_dim = val["embedding_dim"];
        if (loaded_dim != embedding_dim_)
        {
          throw std::runtime_error("Embedding dim mismatch: expected " + std::to_string(embedding_dim_) +
                                   ", got " + std::to_string(loaded_dim));
        }
      }
      if (matrix_.rows() != static_cast<int>(data_.size()))
      {
        throw std::runtime_error("Eigen::MatrixXf row count does not match data size");
      }
      pre_process();
    }
    else
    {
      matrix_ = Eigen::MatrixXf(0, embedding_dim_);
    }
  }

  /**
   * @brief Construct a new Nano Eigen::VectorXf DB object with strategies
   *
   * @param embedding_dim Embedding dimension
   * @param metric Similarity metric to use ("cosine" supported)
   * @param storage_file Storage file path
   * @param metric_strategy Metric strategy
   * @param storage_strategy Storage strategy
   */
  NanoVectorDB(int embedding_dim, const std::string& metric, const std::string& storage_file,
               std::shared_ptr<IMetric> metric_strategy,
               std::shared_ptr<IStorage> storage_strategy)
    : embedding_dim_(embedding_dim)
    , metric_(metric)
    , storage_file_(storage_file)
    , metric_strategy_(std::move(metric_strategy))
    , storage_strategy_(std::move(storage_strategy))
  {
    NVDB_LOG("[NanoVectorDB::NanoVectorDB] embedding_dim=" << embedding_dim_ << ", metric=" << metric_
                                                           << ", storage_file=" << storage_file_);
    // Use storage + serializer strategies if provided, fallback to default file loading
    std::optional<nlohmann::json> loaded;
    std::vector<Data> loaded_records;
    if (storage_strategy_)
    {
      try
      {
        if (auto rs = std::dynamic_pointer_cast<IStorageRecords>(storage_strategy_))
        {
          auto lr = rs->read_records(storage_file_);
          if (!lr.records.empty() || lr.embedding_dim > 0)
          {
            loaded_records = std::move(lr.records);
            loaded = nlohmann::json{ {"embedding_dim", lr.embedding_dim}, {"matrix", ""}, {"data", nlohmann::json::array()} };
            additional_data_ = lr.additional;
          }
        }
        else
        {
          auto bytes = storage_strategy_->read(storage_file_);
          if (!bytes.empty())
          {
            nlohmann::json j = nlohmann::json::parse(bytes.begin(), bytes.end());
            loaded = j;
          }
        }
      }
      catch (...)
      {
        loaded = load_storage(storage_file_, embedding_dim_);
      }
    }
    else
    {
      loaded = load_storage(storage_file_, embedding_dim_);
    }
    if (loaded)
    {
      const auto& val = loaded.value();
      if (!val.contains("matrix"))
      {
        throw std::runtime_error("Storage file missing 'matrix' field");
      }
      if (!loaded_records.empty())
      {
        // Rebuild from serializer-decoded records
        data_ = loaded_records;
        matrix_.resize((int)data_.size(), embedding_dim_);
        for (size_t i = 0; i < data_.size(); ++i)
        {
          if (data_[i].vector.size() != embedding_dim_)
          {
            throw std::runtime_error("Loaded record dim mismatch");
          }
          matrix_.row((int)i) =
              Eigen::Map<const Eigen::RowVectorXf>(data_[i].vector.data(), data_[i].vector.size());
        }
      }
      else
      {
        std::string matrix_b64 = val["matrix"];
        matrix_ = buffer_string_to_array(matrix_b64, embedding_dim_);
        if (!val.contains("data"))
        {
          throw std::runtime_error("Storage file missing 'data' field");
        }
        for (const auto& d : val["data"])
        {
          if (!d.contains("id"))
          {
            throw std::runtime_error("Data entry missing 'id' field");
          }
          Data entry;
          entry.id = d["id"];
          entry.vector = matrix_.row((int)data_.size());
          data_.push_back(entry);
        }
      }
      if (val.contains("additional_data"))
      {
        additional_data_ = val["additional_data"];
      }
      if (val.contains("embedding_dim"))
      {
        int loaded_dim = val["embedding_dim"];
        if (loaded_dim != embedding_dim_)
        {
          throw std::runtime_error("Embedding dim mismatch: expected " + std::to_string(embedding_dim_) +
                                   ", got " + std::to_string(loaded_dim));
        }
      }
      if (matrix_.rows() != static_cast<int>(data_.size()))
      {
        throw std::runtime_error("Eigen::MatrixXf row count does not match data size");
      }
      pre_process();
    }
    else
    {
      matrix_ = Eigen::MatrixXf(0, embedding_dim_);
    }
  }

  /**
   * @brief Pre-process the matrix (e.g., normalize for cosine)
   */
  void pre_process()
  {
    NVDB_LOG("[NanoVectorDB::pre_process] matrix shape: (" << matrix_.rows() << ", " << matrix_.cols()
                                                           << ")");
    if (metric_ == "cosine")
    {
      if (matrix_.rows() > 0)
        matrix_ = normalize_rows(matrix_);
    }
  }

  /**
   * @brief Upsert data entries into the database.
   *
   * @param datas Eigen::VectorXf of Data entries to upsert.
   */
  void upsert(const std::vector<Data>& datas)
  {
    NVDB_LOG("[NanoVectorDB::upsert] datas.size()=" << datas.size());
    std::unordered_map<std::string, Data> index_datas;
    for (const auto& data : datas)
    {
      if (data.vector.size() != embedding_dim_)
      {
        throw std::runtime_error("Eigen::VectorXf dimension mismatch in upsert: expected " +
                                 std::to_string(embedding_dim_) + ", got " +
                                 std::to_string(data.vector.size()));
      }
      // Always use hash of vector as ID if id is empty, to match Python behavior
      std::string id = data.id.empty() ? hash_vector(data.vector) : data.id;
      Data entry = data;
      entry.id = id;
      index_datas[id] = entry;
    }
    if (metric_ == "cosine")
    {
      for (auto& [id, d] : index_datas)
      {
        d.vector = normalize(d.vector);
      }
    }
    std::unordered_set<std::string> updated;
    NVDB_LOG("[NanoVectorDB::upsert] data_.size() before update=" << data_.size());
    int updated_count = 0;
    int inserted_count = 0;
    for (size_t i = 0; i < data_.size(); ++i)
    {
      auto it = index_datas.find(data_[i].id);
      if (it != index_datas.end())
      {
        data_[i] = it->second;
        const auto& vec = it->second.vector;
        if (vec.size() != embedding_dim_)
        {
          throw std::runtime_error("[upsert] Eigen::VectorXf size mismatch before assignment");
        }
        matrix_.row(i) = Eigen::Map<const Eigen::RowVectorXf>(vec.data(), vec.size());
        updated.insert(it->first);
        ++updated_count;
      }
    }
    for (const auto& [id, d] : index_datas)
    {
      if (updated.count(id) == 0)
      {
        if (d.vector.size() != embedding_dim_)
        {
          throw std::runtime_error("[upsert] Eigen::VectorXf size mismatch before assignment (new row)");
        }
        data_.push_back(d);
        matrix_.conservativeResize(matrix_.rows() + 1, embedding_dim_);
        matrix_.row(matrix_.rows() - 1) =
            Eigen::Map<const Eigen::RowVectorXf>(d.vector.data(), d.vector.size());
        ++inserted_count;
      }
    }
    NVDB_LOG("[NanoVectorDB::upsert] summary: updated=" << updated_count << ", inserted=" << inserted_count);
  }

  /**
   * @brief Retrieve data entries by their IDs.
   *
   * @param ids Eigen::VectorXf of IDs to retrieve.
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
    Eigen::MatrixXf new_matrix(keep_indices.size(), embedding_dim_);
    for (size_t i = 0; i < keep_indices.size(); ++i)
    {
      new_matrix.row(i) = matrix_.row(keep_indices[i]);
    }
    matrix_ = std::move(new_matrix);
    if (matrix_.rows() != static_cast<int>(data_.size()))
    {
      throw std::runtime_error("Eigen::MatrixXf row count does not match data size after remove");
    }
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
  std::vector<QueryResult> query(const Eigen::VectorXf& query, int top_k = 10,
                                 std::optional<float> better_than_threshold = std::nullopt,
                                 std::function<bool(const Data&)> filter = nullptr) const
  {
    if (query.size() != embedding_dim_)
    {
      throw std::runtime_error("Query vector dimension mismatch: expected " + std::to_string(embedding_dim_) +
                               ", got " + std::to_string(query.size()));
    }
    // If a strategy is provided, use it for general distance-based querying
    if (metric_strategy_)
    {
      std::vector<int> indices;
      if (filter)
      {
        for (size_t i = 0; i < data_.size(); ++i)
        {
          if (filter(data_[i]))
            indices.push_back(static_cast<int>(i));
        }
      }
      else
      {
        indices.resize(data_.size());
        std::iota(indices.begin(), indices.end(), 0);
      }
      std::vector<std::pair<int, float>> scored;  // store index and score (higher is better)
      const bool is_cosine = (std::dynamic_pointer_cast<CosineMetric>(metric_strategy_) != nullptr);
      for (int idx : indices)
      {
        const Eigen::VectorXf& v = data_[idx].vector;
        float dist = metric_strategy_->distance(query, v);
        float score = is_cosine ? (1.0f - dist) : (-dist);
        scored.emplace_back(idx, score);
      }
      std::sort(scored.begin(), scored.end(), [](auto& a, auto& b) { return a.second > b.second; });
      std::vector<QueryResult> results;
      for (int i = 0; i < std::min(top_k, (int)scored.size()); ++i)
      {
        float score = scored[i].second;
        if (better_than_threshold && score < *better_than_threshold)
          break;
        results.push_back({ data_[scored[i].first], score });
      }
      return results;
    }
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
    if (storage_strategy_) {
      if (auto rs = std::dynamic_pointer_cast<IStorageRecords>(storage_strategy_))
      {
        rs->write_records(storage_file_, data_, embedding_dim_, additional_data_);
        return;
      }
    }
    nlohmann::json storage;
    std::string dumped;
    storage["embedding_dim"] = embedding_dim_;
    storage["matrix"] = array_to_buffer_string(matrix_);
    std::vector<nlohmann::json> data_json;
    for (const auto& d : data_)
    {
      nlohmann::json entry;
      entry["id"] = d.id;
      data_json.push_back(entry);
    }
    storage["data"] = data_json;
    if (!additional_data_.is_null())
    {
      storage["additional_data"] = additional_data_;
    }
    dumped = storage.dump();
    if (storage_strategy_)
    {
      std::vector<uint8_t> bytes(dumped.begin(), dumped.end());
      storage_strategy_->write(storage_file_, bytes);
    }
    else
    {
      std::ofstream f(storage_file_);
      if (!f.is_open())
      {
        throw std::runtime_error("Failed to open storage file for saving: " + storage_file_);
      }
      f << dumped;
    }
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

  /**
   * @brief Initialize metric strategy
   * @param metric_strategy Metric strategy See enum in metric/base.hpp
   */
  void initialize_metric(::nano_vectordb::metric type)
  {
    metric_strategy_ = ::nano_vectordb::make(type);

    // Keep legacy string in sync for preprocessing
    switch (type)
    {
      case ::nano_vectordb::metric::Cosine:
        metric_ = "cosine";
        break;
      case ::nano_vectordb::metric::L2:
        metric_ = "l2";
        break;
    }
    pre_process();
  }

  // Overload: initialize metric with a strategy instance
  void initialize_metric(const std::shared_ptr<IMetric>& strategy)
  {
    metric_strategy_ = strategy;
    if (std::dynamic_pointer_cast<CosineMetric>(strategy))
      metric_ = "cosine";
    else
      metric_ = "l2";
    pre_process();
  }

  /**
   * @brief Initialize serializer strategy
   * @param serializer_strategy Serializer strategy. See enum in serializer/base.hpp
   */
  // Serializer support removed; JSON persistence handled internally.
  /**
   * @brief Initialize storage strategy
   * @param storage_strategy Storage strategy. See enum in storage/base.hpp
   */
  void initialize_storage(::nano_vectordb::storage type, const std::string& path)
  {
    storage_strategy_ = ::nano_vectordb::make(type);
    storage_file_ = path;
  }

  // Overload: initialize storage with a strategy instance, keep current storage_file_
  void initialize_storage(const std::shared_ptr<IStorage>& strategy)
  {
    storage_strategy_ = strategy;
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
  std::vector<QueryResult> cosine_query(const Eigen::VectorXf& query, int top_k,
                                        std::optional<float> better_than_threshold,
                                        std::function<bool(const Data&)> filter) const
  {
    Eigen::VectorXf q = normalize(query);
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
    std::vector<std::pair<int, float>> scored;
    for (int idx : indices)
    {
      float score = matrix_.row(idx).dot(q);
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
  Eigen::MatrixXf matrix_;
  nlohmann::json additional_data_ = nlohmann::json::object();

  // Strategy pointers (optional)
  std::shared_ptr<IMetric> metric_strategy_{};
  // Serializer removed
  std::shared_ptr<IStorage> storage_strategy_{};
};

}  // namespace nano_vectordb
