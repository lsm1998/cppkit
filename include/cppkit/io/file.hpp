#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace cppkit::io
{

  class File
  {
  private:
    std::filesystem::path _path;

    bool checkPermission(std::filesystem::perms mask) const;

  public:
    explicit File(const std::string& path) : _path(std::filesystem::path(path)) {}

    File() = delete;

    File(const File&) = delete;

    File(File&&) noexcept = default;

    File& operator=(const File&) = delete;

    File& operator=(File&&) noexcept = default;

    ~File() = default;

    long size() const;

    bool canRead() const;

    bool canWrite() const;

    bool canExecute() const;

    bool createNewFile() const;

    bool deleteFile() const;

    bool deleteOnExit();

    bool exists() const;

    std::string getAbsolutePath() const;

    std::string getName() const;

    bool isDirectory() const;

    bool isFile() const;

    std::vector<File> listFiles() const;

    std::vector<std::string> fileList() const;

    bool mkdir() const;

    bool mkdirs() const;

    bool renameTo(const File& dest) const;
  };

}  // namespace cppkit::io