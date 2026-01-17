#pragma once
#include "NanoVectorDB.hpp"
#include "metric/base.hpp"
#include "metric/factory.hpp"
#include "serializer/base.hpp"
#include "serializer/factory.hpp"
#include "storage/base.hpp"
#include "storage/factory.hpp"
#include <string>
#include <unordered_map>
#include <random>
#include <vector>
#include <memory>
#include <filesystem>

namespace nano_vectordb
{
/**
 * @brief Multi-tenant NanoVectorDB manager
 */
class MultiTenantNanoVDB
{
public:
  /**
   * @brief Construct a new Multi Tenant Nano VDB object
   *
   * @param embedding_dim Dimension of the embedding vectors.
   * @param metric Similarity metric to use ("cosine" supported).
   * @param max_capacity Maximum number of tenants to cache in memory.
   * @param storage_dir Directory for persistent storage of tenant databases.
   */
  MultiTenantNanoVDB(int embedding_dim, const std::string& metric = "cosine", int max_capacity = 1000,
                     const std::string& storage_dir = "./nano_multi_tenant_storage")
    : embedding_dim_(embedding_dim), metric_(metric), max_capacity_(max_capacity), storage_dir_(storage_dir)
  {
    if (embedding_dim_ <= 0)
    {
      throw std::runtime_error("Embedding dimension must be positive");
    }
    if (max_capacity_ <= 0)
    {
      throw std::runtime_error("Max capacity must be positive");
    }
    if (storage_dir_.empty())
    {
      throw std::runtime_error("Storage directory must not be empty");
    }
  }

  /**
   * @brief Configure default strategies used for new tenants
   * @param metric Default metric strategy. See enum in metric/base.hpp
   */
  void set_default_metric(::nano_vectordb::metric type)
  {
    default_metric_ = ::nano_vectordb::make(type);
  }

  /**
   * @brief Configure default strategies used for new tenants
   * @param serializer Default serializer strategy. See enum in serializer/base.hpp
   */
  void set_default_serializer(::nano_vectordb::serializer type)
  {
    default_serializer_ = ::nano_vectordb::make(type);
  }

  /**
   * @brief Configure default strategies used for new tenants
   * @param storage Default storage strategy. See enum in storage/base.hpp
   */
  void set_default_storage(::nano_vectordb::storage type)
  {
    default_storage_ = ::nano_vectordb::make(type);
  }

  /**
   * @brief Generate the JSON file name from tenant ID
   *
   * @param tenant_id Tenant identifier
   * @return std::string Corresponding JSON file name
   */
  static std::string jsonfile_from_id(const std::string& tenant_id)
  {
    return "nanovdb_" + tenant_id + ".json";
  }

  /**
   * @brief Check if a tenant exists
   *
   * @param tenant_id Tenant identifier
   * @return true If tenant exists
   * @return false If tenant does not exist
   */
  bool contain_tenant(const std::string& tenant_id) const
  {
    return storage_.count(tenant_id) ||
           std::filesystem::exists(storage_dir_ + "/" + jsonfile_from_id(tenant_id));
  }

  /**
   * @brief Create a tenant object
   *
   * @return std::string Tenant identifier
   */
  std::string create_tenant()
  {
    std::string tenant_id = generate_uuid();
    auto db = std::make_shared<NanoVectorDB>(embedding_dim_, metric_,
                                             storage_dir_ + "/" + jsonfile_from_id(tenant_id));
    // Apply default strategies if provided
    if (default_metric_)
      db->initialize_metric(default_metric_);
    if (default_serializer_)
      db->initialize_serializer(default_serializer_);
    if (default_storage_)
      db->initialize_storage(default_storage_);
    if (!db)
    {
      throw std::runtime_error("Failed to create NanoVectorDB for tenant");
    }
    load_tenant_in_cache(tenant_id, db);
    return tenant_id;
  }

  /**
   * @brief Delete a tenant object
   *
   * @param tenant_id Tenant identifier
   */
  void delete_tenant(const std::string& tenant_id)
  {
    if (!contain_tenant(tenant_id))
    {
      throw std::runtime_error("Tenant does not exist: " + tenant_id);
    }
    storage_.erase(tenant_id);
    cache_queue_.erase(std::remove(cache_queue_.begin(), cache_queue_.end(), tenant_id), cache_queue_.end());
    std::error_code ec;
    std::filesystem::remove(storage_dir_ + "/" + jsonfile_from_id(tenant_id), ec);
    if (ec)
    {
      throw std::runtime_error("Failed to remove tenant file: " + ec.message());
    }
  }

  /**
   * @brief Get a tenant object
   *
   * @param tenant_id Tenant identifier
   * @return std::shared_ptr<NanoVectorDB>
   */
  std::shared_ptr<NanoVectorDB> get_tenant(const std::string& tenant_id)
  {
    // If present in cache, return it
    if (storage_.count(tenant_id))
    {
      if (!storage_[tenant_id])
      {
        throw std::runtime_error("Tenant DB pointer is null: " + tenant_id);
      }
      return storage_[tenant_id];
    }
    // Lazy-load from disk if file exists
    const std::string path = storage_dir_ + "/" + jsonfile_from_id(tenant_id);
    if (std::filesystem::exists(path))
    {
      auto db = std::make_shared<NanoVectorDB>(embedding_dim_, metric_, path);
      load_tenant_in_cache(tenant_id, db);
      return db;
    }
    throw std::runtime_error("Tenant not found: " + tenant_id);
  }

  /**
   * @brief Save all cached tenant databases to disk.
   */
  void save()
  {
    std::error_code ec;
    if (!std::filesystem::exists(storage_dir_))
    {
      std::filesystem::create_directories(storage_dir_, ec);
      if (ec)
      {
        throw std::runtime_error("Failed to create storage directory: " + ec.message());
      }
    }
    for (const auto& [tenant_id, db] : storage_)
    {
      if (!db)
      {
        throw std::runtime_error("Tenant DB pointer is null during save: " + tenant_id);
      }
      try
      {
        db->save();
      }
      catch (const std::exception& e)
      {
        throw std::runtime_error("Failed to save tenant '" + tenant_id + "': " + e.what());
      }
    }
  }

private:
  /**
   * @brief Load a tenant into cache
   *
   * @param tenant_id Tenant identifier
   * @param db Pointer to NanoVectorDB instance
   */
  void load_tenant_in_cache(const std::string& tenant_id, std::shared_ptr<NanoVectorDB> db)
  {
    if (!db)
    {
      throw std::runtime_error("Cannot cache null NanoVectorDB for tenant: " + tenant_id);
    }
    if (storage_.size() >= (size_t)max_capacity_)
    {
      // Evict least-recently-added tenant and persist it to disk
      const std::string evict_id = cache_queue_.front();
      auto it = storage_.find(evict_id);
      if (it != storage_.end() && it->second)
      {
        std::error_code ec;
        if (!std::filesystem::exists(storage_dir_))
        {
          std::filesystem::create_directories(storage_dir_, ec);
          if (ec)
          {
            throw std::runtime_error("Failed to create storage directory during eviction: " + ec.message());
          }
        }
        try
        {
          it->second->save();
        }
        catch (const std::exception& e)
        {
          throw std::runtime_error("Failed to save evicted tenant '" + evict_id + "': " + e.what());
        }
      }
      storage_.erase(evict_id);
      cache_queue_.erase(cache_queue_.begin());
    }
    storage_[tenant_id] = db;
    cache_queue_.push_back(tenant_id);
  }

  /**
   * @brief Generate a UUID string
   *
   * @return std::string UUID string
   */
  static std::string generate_uuid()
  {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    std::ostringstream oss;
    oss << std::hex << dis(gen);
    return oss.str();
  }

  int embedding_dim_;
  std::string metric_;
  int max_capacity_;
  std::string storage_dir_;
  std::unordered_map<std::string, std::shared_ptr<NanoVectorDB>> storage_;
  std::vector<std::string> cache_queue_;

  // Default strategies
  std::shared_ptr<IMetric> default_metric_{};
  std::shared_ptr<ISerializer> default_serializer_{};
  std::shared_ptr<IStorage> default_storage_{};
};

}  // namespace nano_vectordb
