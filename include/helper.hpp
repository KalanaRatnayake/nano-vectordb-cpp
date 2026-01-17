#pragma once
#include <string>
#include <vector>
#include <optional>
#include <fstream>
#include <filesystem>
#include <Eigen/Dense>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <iostream>

#ifndef NANOVDB_ENABLE_LOG
#define NANOVDB_ENABLE_LOG 0
#endif
#if NANOVDB_ENABLE_LOG
#define NVDB_LOG(msg) do { std::cerr << msg << std::endl; } while(0)
#else
#define NVDB_LOG(msg) do {} while(0)
#endif

namespace nano_vectordb
{

using Float = float;
using Vector = Eigen::VectorXf;
using Matrix = Eigen::MatrixXf;

/**
 * @brief Convert a matrix to a base64-encoded string
 * 
 * @param array The matrix to encode
 * @return std::string The base64-encoded string
 */
inline std::string array_to_buffer_string(const Matrix& array)
{
  // Allow empty matrix: encode as empty string
  if (array.size() == 0 || array.rows() == 0 || array.cols() == 0) {
    return std::string();
  }
  const char* bytes = reinterpret_cast<const char*>(array.data());
  size_t byte_len = sizeof(Float) * array.size();
  BIO *bio, *b64;
  BUF_MEM* bufferPtr;
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new(BIO_s_mem());
  bio = BIO_push(b64, bio);
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
  BIO_write(bio, bytes, byte_len);
  BIO_flush(bio);
  BIO_get_mem_ptr(bio, &bufferPtr);
  std::string result(bufferPtr->data, bufferPtr->length);
  BIO_free_all(bio);
  return result;
}

/**
 * @brief Decode a base64-encoded string to a matrix
 * 
 * @param base64_str The base64-encoded string
 * @param embedding_dim The dimension of the embedding vectors
 * @return Matrix The decoded matrix
 */
inline Matrix buffer_string_to_array(const std::string& base64_str, int embedding_dim)
{
  if (embedding_dim <= 0) {
    throw std::runtime_error("Embedding dimension must be positive in buffer_string_to_array");
  }
  // Allow empty base64 string meaning zero rows
  if (base64_str.empty()) {
    return Matrix(0, embedding_dim);
  }
  BIO *bio, *b64;
  int decodeLen = (base64_str.length() * 3) / 4;
  std::vector<char> buffer(std::max(1, decodeLen));
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new_mem_buf(base64_str.data(), base64_str.length());
  bio = BIO_push(b64, bio);
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
  int len = BIO_read(bio, buffer.data(), base64_str.length());
  BIO_free_all(bio);
  if (len <= 0) {
    return Matrix(0, embedding_dim);
  }
  int row_size_bytes = embedding_dim * sizeof(Float);
  if (len % row_size_bytes != 0) {
    throw std::runtime_error("Invalid decoded length for embedding_dim in buffer_string_to_array: len=" + std::to_string(len) + ", embedding_dim=" + std::to_string(embedding_dim));
  }
  int rows = len / row_size_bytes;
  Matrix mat(rows, embedding_dim);
  std::memcpy(mat.data(), buffer.data(), rows * embedding_dim * sizeof(Float));
  return mat;
}

/**
 * @brief Load storage from a JSON file
 * 
 * @param file_name The name of the JSON file
 * @param embedding_dim The dimension of the embedding vectors
 * @return std::optional<nlohmann::json> The loaded JSON object, or nullopt if loading failed
 */
inline std::optional<nlohmann::json> load_storage(const std::string& file_name, int embedding_dim)
{
  namespace fs = std::filesystem;
  // If the storage file does not exist, return nullopt to allow fresh initialization
  if (!fs::exists(file_name)) {
    return std::nullopt;
  }
  std::ifstream f(file_name);
  if (!f.is_open()) {
    throw std::runtime_error("Failed to open storage file: " + file_name);
  }
  nlohmann::json data;
  try {
    f >> data;
  } catch (const std::exception& e) {
    throw std::runtime_error("Failed to parse JSON from storage file: " + std::string(e.what()));
  }
  if (!data.contains("matrix")) {
    throw std::runtime_error("Storage file missing 'matrix' field: " + file_name);
  }
  return data;
}

/**
 * @brief Normalize each row of a matrix to unit length.
 *
 * @param m Input matrix (rows are vectors).
 * @return Matrix Row-wise normalized matrix.
 */
inline Matrix normalize_rows(const Matrix& m)
{
  if (m.size() == 0 || m.rows() == 0) {
    return m;
  }
  Matrix out = m;
  for (int i = 0; i < out.rows(); ++i) {
    Float n = out.row(i).norm();
    if (n == 0) {
      throw std::runtime_error("Cannot normalize zero-norm row in matrix at row " + std::to_string(i));
    }
    out.row(i) /= n;
  }
  return out;
}

/**
 * @brief Generate a hash string for a given vector.
 *
 * @param v Input vector.
 * @return std::string Hash string.
 */
inline std::string hash_vector(const Vector& v)
{
  if (v.size() == 0) {
    throw std::runtime_error("Cannot hash empty vector");
  }
  std::hash<Float> hasher;
  size_t hash = 0;
  for (int i = 0; i < v.size(); ++i)
  {
    hash ^= hasher(v[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  }
  std::ostringstream oss;
  oss << std::hex << hash;
  return oss.str();
}

/**
 * @brief Normalize a vector to unit length.
 *
 * @param v Input vector.
 * @return Vector Normalized vector.
 */
inline Vector normalize(const Vector& v)
{
  if (v.size() == 0) {
    throw std::runtime_error("Cannot normalize empty vector");
  }
  Float n = v.norm();
  if (n == 0) {
    throw std::runtime_error("Cannot normalize zero-norm vector");
  }
  return v / n;
}

}  // namespace nano_vectordb
