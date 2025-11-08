#pragma once

#include "io.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <fstream>
#include <functional>

namespace cppkit::io
{
  class File
  {
    [[nodiscard]] bool checkPermission(std::filesystem::perms mask) const;

  public:
    explicit File(const std::string& path) : _path(std::filesystem::path(path))
    {
    }

    File() = delete;

    File(const File&) = delete;

    File(File&&) noexcept = default;

    File& operator=(const File&) = delete;

    File& operator=(File&&) noexcept = default;

    ~File() = default;

    [[nodiscard]] size_t size() const;

    [[nodiscard]] bool canRead() const;

    [[nodiscard]] bool canWrite() const;

    [[nodiscard]] bool canExecute() const;

    [[nodiscard]] bool createNewFile() const;

    [[nodiscard]] bool deleteFile() const;

    [[nodiscard]] bool deleteOnExit() const;

    [[nodiscard]] bool exists() const;

    [[nodiscard]] std::string getAbsolutePath() const;

    [[nodiscard]] std::string getName() const;

    [[nodiscard]] bool isDirectory() const;

    [[nodiscard]] bool isFile() const;

    [[nodiscard]] std::vector<File> listFiles() const;

    [[nodiscard]] std::vector<std::string> fileList() const;

    [[nodiscard]] bool mkdir() const;

    [[nodiscard]] bool mkdirs() const;

    [[nodiscard]] bool renameTo(const File& dest) const;

    size_t read(char* buffer, size_t size, size_t offset = 0) const;

    size_t write(const char* buffer, size_t size, size_t offset = 0, bool append = false) const;

    size_t read(const std::function<void(const char*, ssize_t)>& fun, size_t offset = 0, int chunk = BUFFER_SIZE) const;

  private:
    bool open() const;

    void close() const;

    std::filesystem::path _path;
    mutable std::ifstream _ifs;
    mutable std::ofstream _ofs;
    mutable bool _isOpen{false};
  };
} // namespace cppkit::io