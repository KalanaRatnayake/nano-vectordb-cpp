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
  BIO *bio, *b64;
  int decodeLen = (base64_str.length() * 3) / 4;
  std::vector<char> buffer(decodeLen);
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new_mem_buf(base64_str.data(), base64_str.length());
  bio = BIO_push(b64, bio);
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
  int len = BIO_read(bio, buffer.data(), base64_str.length());
  BIO_free_all(bio);
  int rows = len / (sizeof(Float) * embedding_dim);
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
  if (!fs::exists(file_name))
    return std::nullopt;
  std::ifstream f(file_name);
  if (!f.is_open())
    return std::nullopt;
  nlohmann::json data;
  f >> data;
  if (!data.contains("matrix"))
    return std::nullopt;
  data["matrix"] = buffer_string_to_array(data["matrix"], embedding_dim);
  return data;
}

/**
 * @brief Generate a hash string for a given vector.
 *
 * @param v Input vector.
 * @return std::string Hash string.
 */
inline std::string hash_vector(const Vector& v)
{
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
  return v / v.norm();
}

}  // namespace nano_vectordb
