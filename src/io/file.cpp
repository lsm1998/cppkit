#include "cppkit/io/file.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace cppkit::io {

std::vector<std::filesystem::path> &deleteList() {
  static auto *v = new std::vector<std::filesystem::path>();
  return *v;
}

std::mutex &deleteListMutex() {
  static std::mutex *m = new std::mutex();
  return *m;
}

void registerExitHandler() {
  static bool registered = false;
  if (!registered) {
    std::atexit([]() {
      std::lock_guard<std::mutex> lock(deleteListMutex());
      for (const auto &p : deleteList()) {
        std::error_code ec;
        std::filesystem::remove(p, ec);
        if (ec) {
          std::cerr << "Failed to delete: " << p << " (" << ec.message()
                    << ")\n";
        }
      }
    });
    registered = true;
  }
}

long File::size() const {
  return isFile() ? std::filesystem::file_size(_path) : 0;
}

bool File::checkPermission(std::filesystem::perms mask) const {
  if (!exists()) {
    return false;
  }
  auto perms = std::filesystem::status(_path).permissions();
  return (perms & mask) != std::filesystem::perms::none;
}

bool File::canRead() const {
  using std::filesystem::perms;
  return checkPermission(perms::owner_read | perms::group_read |
                         perms::others_read);
}

bool File::canWrite() const {
  using std::filesystem::perms;
  return checkPermission(perms::owner_write | perms::group_write |
                         perms::others_write);
}

bool File::canExecute() const {
  using std::filesystem::perms;
  return checkPermission(perms::owner_exec | perms::group_exec |
                         perms::others_exec);
}

bool File::createNewFile() const {
  if (exists()) {
    return false;
  }
  std::ofstream ofs(_path);
  return ofs.good();
}

bool File::deleteFile() const {
  if (!exists()) {
    return false;
  }
  return std::filesystem::remove_all(_path) > 0;
}

bool File::deleteOnExit() {
  if (!exists()) {
    return false;
  }
  registerExitHandler();
  std::lock_guard<std::mutex> lock(deleteListMutex());
  deleteList().push_back(_path);
  return true;
}

bool File::exists() const { return std::filesystem::exists(_path); }

std::string File::getAbsolutePath() const {
  try {
    return std::filesystem::absolute(_path).string();
  } catch (const std::filesystem::filesystem_error &e) {
    return _path.string();
  }
}

std::string File::getName() const { return _path.filename(); }

bool File::isFile() const { return std::filesystem::is_regular_file(_path); }

bool File::isDirectory() const { return std::filesystem::is_directory(_path); }

std::vector<File> File::listFiles() const {
  if (!exists() || !isDirectory()) {
    return {};
  }
  std::vector<File> files;
  for (const auto &entry : std::filesystem::directory_iterator(_path)) {
    files.emplace_back(entry.path().string());
  }
  return files;
}

std::vector<std::string> File::fileList() const {
  if (!exists() || !isDirectory()) {
    return {};
  }
  std::vector<std::string> files;
  for (const auto &entry : std::filesystem::directory_iterator(_path)) {
    files.emplace_back(entry.path().string());
  }
  return files;
}

bool File::mkdir() const { return std::filesystem::create_directory(_path); }

bool File::mkdirs() const { return std::filesystem::create_directories(_path); }

bool File::renameTo(const File &dest) const {
  try {
    std::filesystem::rename(_path, dest._path);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace cppkit::io