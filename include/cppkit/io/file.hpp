#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace cppkit::io
{
  class File
  {
    std::filesystem::path _path;

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

    [[nodiscard]] long size() const;

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
  };
} // namespace cppkit::io