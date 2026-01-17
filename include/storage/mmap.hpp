#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "base.hpp"

namespace nano_vectordb
{

/**
 * @brief Memory-mapped file storage backend
 */
struct MMapStorage : public IStorage
{
  /**
   * @brief Write a byte array to the specified storage path.
   * @param path Storage path.
   * @param bytes Byte array to write.
   */
  void write(const std::string& path, const std::vector<uint8_t>& bytes) const override
  {
    int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
      throw std::runtime_error("MMapStorage: open failed");
    if (::ftruncate(fd, static_cast<off_t>(bytes.size())) != 0)
    {
      ::close(fd);
      throw std::runtime_error("MMapStorage: ftruncate failed");
    }
    void* addr = ::mmap(nullptr, bytes.size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED)
    {
      ::close(fd);
      throw std::runtime_error("MMapStorage: mmap failed");
    }
    std::memcpy(addr, bytes.data(), bytes.size());
    ::msync(addr, bytes.size(), MS_SYNC);
    ::munmap(addr, bytes.size());
    ::close(fd);
  }

  /**
   * @brief Read a byte array from the specified storage path.
   * @param path Storage path.
   * @return std::vector<uint8_t> Byte array read from storage.
   */
  std::vector<uint8_t> read(const std::string& path) const override
  {
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0)
      throw std::runtime_error("MMapStorage: open failed");
    struct stat st{};
    if (::fstat(fd, &st) != 0)
    {
      ::close(fd);
      throw std::runtime_error("MMapStorage: fstat failed");
    }
    std::size_t size = static_cast<std::size_t>(st.st_size);
    std::vector<uint8_t> buf(size);
    if (size == 0)
    {
      ::close(fd);
      return buf;
    }
    void* addr = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED)
    {
      ::close(fd);
      throw std::runtime_error("MMapStorage: mmap failed");
    }
    std::memcpy(buf.data(), addr, size);
    ::munmap(addr, size);
    ::close(fd);
    return buf;
  }
};

}  // namespace nano_vectordb
