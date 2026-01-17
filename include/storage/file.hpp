#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include "base.hpp"

namespace nano_vectordb
{

/**
 * @brief File-based storage backend
 */
struct FileStorage : public IStorage
{
  /**
   * @brief Write a byte array to the specified storage path.
   * @param path Storage path.
   * @param bytes Byte array to write.
   */
  void write(const std::string& path, const std::vector<uint8_t>& bytes) const override
  {
    std::ofstream ofs(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!ofs)
      throw std::runtime_error("FileStorage: cannot open for write: " + path);
    ofs.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }

  /**
   * @brief Read a byte array from the specified storage path.
   * @param path Storage path.
   * @return std::vector<uint8_t> Byte array read from storage.
   */
  std::vector<uint8_t> read(const std::string& path) const override
  {
    std::ifstream ifs(path, std::ios::binary | std::ios::in);
    if (!ifs)
      throw std::runtime_error("FileStorage: cannot open for read: " + path);
    ifs.seekg(0, std::ios::end);
    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(static_cast<std::size_t>(std::max<std::streamsize>(0, size)));
    if (size > 0)
    {
      ifs.read(reinterpret_cast<char*>(buf.data()), size);
    }
    return buf;
  }
};

}  // namespace nano_vectordb
