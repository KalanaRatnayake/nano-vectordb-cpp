#pragma once
#include "NanoVectorDB.hpp"
#include <string>
#include <unordered_map>
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
    storage_.erase(tenant_id);
    cache_queue_.erase(std::remove(cache_queue_.begin(), cache_queue_.end(), tenant_id), cache_queue_.end());
    std::filesystem::remove(storage_dir_ + "/" + jsonfile_from_id(tenant_id));
  }

  /**
   * @brief Get a tenant object
   *
   * @param tenant_id Tenant identifier
   * @return std::shared_ptr<NanoVectorDB>
   */
  std::shared_ptr<NanoVectorDB> get_tenant(const std::string& tenant_id)
  {
    if (storage_.count(tenant_id))
      return storage_[tenant_id];
    throw std::runtime_error("Tenant not found");
  }

  /**
   * @brief Save all cached tenant databases to disk.
   */
  void save()
  {
    if (!std::filesystem::exists(storage_dir_))
      std::filesystem::create_directories(storage_dir_);
    for (const auto& [tenant_id, db] : storage_)
    {
      db->save();
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
    if (storage_.size() >= (size_t)max_capacity_)
    {
      storage_.erase(cache_queue_.front());
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
};

}  // namespace nano_vectordb
