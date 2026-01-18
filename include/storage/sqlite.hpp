#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include "base.hpp"
namespace nano_vectordb
{

/**
 * @brief SQLite storage backend (row-wise).
 *
 * Schema:
 *  - meta(key TEXT PRIMARY KEY, value TEXT)
 *  - vectors(id TEXT PRIMARY KEY, dim INTEGER NOT NULL, vec BLOB NOT NULL)
 * Stores Eigen::VectorXf as raw float BLOBs.
 */
struct SQLiteStorage : public IStorageRecords
{
  void write_records(const std::string& path, const std::vector<Data>& records, int embedding_dim,
                     const nlohmann::json& additional) const override
  {
    sqlite3* db = nullptr;
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK)
    {
      std::string msg = "SQLiteStorage: open failed: ";
      msg += sqlite3_errmsg(db);
      if (db)
        sqlite3_close(db);
      throw std::runtime_error(msg);
    }
    char* err = nullptr;
    const char* create_meta = "CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value TEXT NOT NULL)";
    const char* create_vecs = "CREATE TABLE IF NOT EXISTS vectors ("
                              " id TEXT PRIMARY KEY,"
                              " dim INTEGER NOT NULL,"
                              " vec BLOB NOT NULL"
                              ")";
    if (sqlite3_exec(db, create_meta, nullptr, nullptr, &err) != SQLITE_OK)
    {
      std::string msg = std::string("SQLiteStorage: create meta failed: ") + (err ? err : "");
      sqlite3_free(err);
      sqlite3_close(db);
      throw std::runtime_error(msg);
    }
    if (sqlite3_exec(db, create_vecs, nullptr, nullptr, &err) != SQLITE_OK)
    {
      std::string msg = std::string("SQLiteStorage: create vectors failed: ") + (err ? err : "");
      sqlite3_free(err);
      sqlite3_close(db);
      throw std::runtime_error(msg);
    }
    if (sqlite3_exec(db, "BEGIN", nullptr, nullptr, &err) != SQLITE_OK)
    {
      std::string msg = std::string("SQLiteStorage: begin failed: ") + (err ? err : "");
      sqlite3_free(err);
      sqlite3_close(db);
      throw std::runtime_error(msg);
    }

    // Clear and insert records
    if (sqlite3_exec(db, "DELETE FROM vectors", nullptr, nullptr, &err) != SQLITE_OK)
    {
      sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
      std::string msg = std::string("SQLiteStorage: clear vectors failed: ") + (err ? err : "");
      sqlite3_free(err);
      sqlite3_close(db);
      throw std::runtime_error(msg);
    }

    const char* ins_sql = "INSERT INTO vectors(id, dim, vec) VALUES (?1, ?2, ?3)";
    sqlite3_stmt* ins = nullptr;
    if (sqlite3_prepare_v2(db, ins_sql, -1, &ins, nullptr) != SQLITE_OK)
    {
      sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
      std::string msg = std::string("SQLiteStorage: prepare insert failed: ") + sqlite3_errmsg(db);
      sqlite3_close(db);
      throw std::runtime_error(msg);
    }
    for (const auto& r : records)
    {
      if (r.vector.size() != embedding_dim)
      {
        sqlite3_finalize(ins);
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        sqlite3_close(db);
        throw std::runtime_error("SQLiteStorage: record dim mismatch");
      }
      std::size_t bytes = static_cast<std::size_t>(r.vector.size()) * sizeof(float);
      if (sqlite3_bind_text(ins, 1, r.id.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
          sqlite3_bind_int(ins, 2, embedding_dim) != SQLITE_OK ||
          sqlite3_bind_blob(ins, 3, r.vector.data(), static_cast<int>(bytes), SQLITE_TRANSIENT) != SQLITE_OK)
      {
        sqlite3_finalize(ins);
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        sqlite3_close(db);
        throw std::runtime_error("SQLiteStorage: bind insert failed");
      }
      if (sqlite3_step(ins) != SQLITE_DONE)
      {
        sqlite3_finalize(ins);
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        sqlite3_close(db);
        throw std::runtime_error("SQLiteStorage: step insert failed");
      }
      sqlite3_reset(ins);
      sqlite3_clear_bindings(ins);
    }
    sqlite3_finalize(ins);

    // Upsert meta
    const char* upsert_meta = "REPLACE INTO meta(key, value) VALUES (?1, ?2)";
    sqlite3_stmt* meta = nullptr;
    if (sqlite3_prepare_v2(db, upsert_meta, -1, &meta, nullptr) != SQLITE_OK)
    {
      sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
      std::string msg = std::string("SQLiteStorage: prepare meta failed: ") + sqlite3_errmsg(db);
      sqlite3_close(db);
      throw std::runtime_error(msg);
    }
    // embedding_dim
    std::string emb_key = "embedding_dim";
    std::string emb_val = std::to_string(embedding_dim);
    if (sqlite3_bind_text(meta, 1, emb_key.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_bind_text(meta, 2, emb_val.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_step(meta) != SQLITE_DONE)
    {
      sqlite3_finalize(meta);
      sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
      sqlite3_close(db);
      throw std::runtime_error("SQLiteStorage: upsert embedding_dim failed");
    }
    sqlite3_reset(meta);
    sqlite3_clear_bindings(meta);
    // additional_data as JSON
    std::string add_key = "additional_data";
    std::string add_val = additional.dump();
    if (sqlite3_bind_text(meta, 1, add_key.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_bind_text(meta, 2, add_val.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_step(meta) != SQLITE_DONE)
    {
      sqlite3_finalize(meta);
      sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
      sqlite3_close(db);
      throw std::runtime_error("SQLiteStorage: upsert additional_data failed");
    }
    sqlite3_finalize(meta);

    if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, &err) != SQLITE_OK)
    {
      std::string msg = std::string("SQLiteStorage: commit failed: ") + (err ? err : "");
      sqlite3_free(err);
      sqlite3_close(db);
      throw std::runtime_error(msg);
    }
    sqlite3_close(db);
  }

  StorageLoad read_records(const std::string& path) const override
  {
    StorageLoad res;
    sqlite3* db = nullptr;
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK)
    {
      std::string msg = "SQLiteStorage: open failed: ";
      msg += sqlite3_errmsg(db);
      if (db)
        sqlite3_close(db);
      throw std::runtime_error(msg);
    }
    char* err = nullptr;
    const char* create_meta = "CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value TEXT NOT NULL)";
    const char* create_vecs = "CREATE TABLE IF NOT EXISTS vectors ("
                              " id TEXT PRIMARY KEY,"
                              " dim INTEGER NOT NULL,"
                              " vec BLOB NOT NULL"
                              ")";
    if (sqlite3_exec(db, create_meta, nullptr, nullptr, &err) != SQLITE_OK)
    {
      std::string msg = std::string("SQLiteStorage: create meta failed: ") + (err ? err : "");
      sqlite3_free(err);
      sqlite3_close(db);
      throw std::runtime_error(msg);
    }
    if (sqlite3_exec(db, create_vecs, nullptr, nullptr, &err) != SQLITE_OK)
    {
      std::string msg = std::string("SQLiteStorage: create vectors failed: ") + (err ? err : "");
      sqlite3_free(err);
      sqlite3_close(db);
      throw std::runtime_error(msg);
    }

    // Read meta
    const char* sel_meta = "SELECT key, value FROM meta WHERE key IN ('embedding_dim','additional_data')";
    sqlite3_stmt* m = nullptr;
    if (sqlite3_prepare_v2(db, sel_meta, -1, &m, nullptr) != SQLITE_OK)
    {
      std::string msg = std::string("SQLiteStorage: prepare meta select failed: ") + sqlite3_errmsg(db);
      sqlite3_close(db);
      throw std::runtime_error(msg);
    }
    while (sqlite3_step(m) == SQLITE_ROW)
    {
      std::string key(reinterpret_cast<const char*>(sqlite3_column_text(m, 0)));
      std::string val(reinterpret_cast<const char*>(sqlite3_column_text(m, 1)));
      if (key == "embedding_dim")
      {
        res.embedding_dim = std::stoi(val);
      }
      else if (key == "additional_data")
      {
        try
        {
          res.additional = nlohmann::json::parse(val);
        }
        catch (...)
        {
          res.additional = nlohmann::json::object();
        }
      }
    }
    sqlite3_finalize(m);

    // Read vectors
    const char* sel_vecs = "SELECT id, dim, vec FROM vectors";
    sqlite3_stmt* v = nullptr;
    if (sqlite3_prepare_v2(db, sel_vecs, -1, &v, nullptr) != SQLITE_OK)
    {
      std::string msg = std::string("SQLiteStorage: prepare vectors select failed: ") + sqlite3_errmsg(db);
      sqlite3_close(db);
      throw std::runtime_error(msg);
    }
    while (sqlite3_step(v) == SQLITE_ROW)
    {
      const char* idtxt = reinterpret_cast<const char*>(sqlite3_column_text(v, 0));
      int dim = sqlite3_column_int(v, 1);
      const void* blob = sqlite3_column_blob(v, 2);
      int size = sqlite3_column_bytes(v, 2);
      if (!blob || size != dim * static_cast<int>(sizeof(float)))
      {
        sqlite3_finalize(v);
        sqlite3_close(db);
        throw std::runtime_error("SQLiteStorage: blob size mismatch");
      }
      Data r;
      r.id = idtxt ? std::string(idtxt) : std::string();
      r.vector.resize(dim);
      std::memcpy(r.vector.data(), blob, static_cast<std::size_t>(size));
      res.records.push_back(std::move(r));
    }
    sqlite3_finalize(v);
    sqlite3_close(db);

    if (res.embedding_dim == 0 && !res.records.empty())
      res.embedding_dim = static_cast<int>(res.records.front().vector.size());
    return res;
  }

  // Byte-oriented API unused for rows backend; provide minimal implementations.
  void write(const std::string& path, const std::vector<uint8_t>&) const override
  {
    throw std::runtime_error("SQLiteStorage: write(bytes) unsupported; use write_records");
  }
  std::vector<uint8_t> read(const std::string& path) const override
  {
    (void)path;
    return {};  // no-op; rows API should be used
  }
};

}  // namespace nano_vectordb
