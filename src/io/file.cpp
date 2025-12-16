#include "cppkit/io/file.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace cppkit::io
{
    std::vector<std::filesystem::path>& deleteList()
    {
        static auto* v = new std::vector<std::filesystem::path>();
        return *v;
    }

    std::mutex& deleteListMutex()
    {
        static auto* m = new std::mutex();
        return *m;
    }

    void registerExitHandler()
    {
        static bool registered = false;
        if (!registered)
        {
            std::atexit(
                []
                {
                    std::lock_guard lock(deleteListMutex());
                    for (const auto& p : deleteList())
                    {
                        std::error_code ec;
                        std::filesystem::remove(p, ec);
                        if (ec)
                        {
                            std::cerr << "Failed to delete: " << p << " (" << ec.message() << ")\n";
                        }
                    }
                });
            registered = true;
        }
    }

    size_t File::size() const
    {
        return isFile() ? std::filesystem::file_size(_path) : 0;
    }

    bool File::checkPermission(const std::filesystem::perms mask) const
    {
        if (!exists())
        {
            return false;
        }
        auto perms = std::filesystem::status(_path).permissions();
        return (perms & mask) != std::filesystem::perms::none;
    }

    bool File::canRead() const
    {
        using std::filesystem::perms;
        return checkPermission(perms::owner_read | perms::group_read | perms::others_read);
    }

    bool File::canWrite() const
    {
        using std::filesystem::perms;
        return checkPermission(perms::owner_write | perms::group_write | perms::others_write);
    }

    bool File::canExecute() const
    {
        using std::filesystem::perms;
        return checkPermission(perms::owner_exec | perms::group_exec | perms::others_exec);
    }

    bool File::createNewFile() const
    {
        if (exists())
        {
            return false;
        }
        const std::ofstream ofs(_path);
        return ofs.good();
    }

    bool File::deleteFile() const
    {
        if (!exists())
        {
            return false;
        }
        return std::filesystem::remove_all(_path) > 0;
    }

    bool File::deleteOnExit() const
    {
        if (!exists())
        {
            return false;
        }
        registerExitHandler();
        std::lock_guard lock(deleteListMutex());
        deleteList().push_back(_path);
        return true;
    }

    bool File::exists() const
    {
        return std::filesystem::exists(_path);
    }

    std::string File::getAbsolutePath() const
    {
        try
        {
            return std::filesystem::absolute(_path).string();
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            return e.what();
        }
    }

    std::string File::getName() const
    {
        return _path.filename();
    }

    std::string File::getParent() const
    {
        const std::filesystem::path parent = _path.parent_path();
        return parent.string();
    }

    bool File::isFile() const
    {
        return std::filesystem::is_regular_file(_path);
    }

    bool File::isDirectory() const
    {
        return std::filesystem::is_directory(_path);
    }

    std::vector<File> File::listFiles() const
    {
        if (!exists() || !isDirectory())
        {
            return {};
        }
        std::vector<File> files;
        for (const auto& entry : std::filesystem::directory_iterator(_path))
        {
            files.emplace_back(entry.path().string());
        }
        return files;
    }

    std::vector<std::string> File::fileList() const
    {
        if (!exists() || !isDirectory())
        {
            return {};
        }
        std::vector<std::string> files;
        for (const auto& entry : std::filesystem::directory_iterator(_path))
        {
            files.emplace_back(entry.path().string());
        }
        return files;
    }

    bool File::mkdir() const
    {
        return std::filesystem::create_directory(_path);
    }

    bool File::mkdirs() const
    {
        return std::filesystem::create_directories(_path);
    }

    bool File::renameTo(const File& dest) const
    {
        try
        {
            std::filesystem::rename(_path, dest._path);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    size_t File::read(char* buffer, const size_t size, const size_t offset) const
    {
        if (!this->open())
        {
            return -1;
        }

        const auto fileSize = this->size();

        if (offset >= fileSize)
        {
            return 0;
        }

        if (static_cast<size_t>(_ifs.tellg()) != offset)
        {
            _ifs.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        }

        _ifs.read(buffer, static_cast<std::streamsize>(std::min(fileSize - offset, size)));
        if (!_ifs && !_ifs.eof())
            return -1;

        return _ifs.gcount();
    }

    size_t File::write(const char* buffer, const size_t size, const size_t offset, const bool append) const
    {
        if (!this->open())
        {
            return -1;
        }
        if (append)
            _ofs.seekp(static_cast<std::streamsize>(offset), std::ios::end);
        else
            _ofs.seekp(static_cast<std::streamsize>(offset), std::ios::beg);
        _ofs.write(buffer, static_cast<std::streamsize>(size));
        if (!_ofs)
            return -1;
        _ofs.flush();
        return static_cast<ssize_t>(size);
    }

    size_t File::read(const std::function<void(const char*, ssize_t)>& fun, size_t offset, const int chunk) const
    {
        const int fd = ::open(this->getAbsolutePath().c_str(), O_RDONLY);
        if (fd < 0)
        {
            return fd;
        }
        struct stat st{};
        fstat(fd, &st);
        const size_t filesize = st.st_size;

        const long page_size = sysconf(_SC_PAGE_SIZE);
        size_t total{};

        while (offset < filesize)
        {
            const size_t bytes = std::min(static_cast<size_t>(chunk), filesize - offset);

            // 计算页对齐的偏移
            const size_t page_offset = offset & ~(page_size - 1);
            const size_t delta = offset - page_offset;

            // mmap 需要映射的大小 = delta + chunk
            const size_t map_length = delta + bytes;

            void* addr = mmap(nullptr, map_length, PROT_READ, MAP_PRIVATE, fd, static_cast<ssize_t>(page_offset));
            if (addr == MAP_FAILED)
            {
                perror("mmap fail");
                ::close(fd);
                return -1;
            }

            // 取真正需要的数据位置
            const char* data = static_cast<char*>(addr) + delta;
            fun(data, static_cast<ssize_t>(bytes));

            munmap(addr, map_length);
            offset += bytes;
            total += bytes;
        }
        ::close(fd);
        return total;
    }

    bool File::open() const
    {
        if (_isOpen) // 已经打开
        {
            return true;
        }
        _ifs.open(_path, std::ios::binary);
        _ofs.open(_path, std::ios::binary | std::ios::in | std::ios::out);

        _isOpen = _ifs.is_open() && _ofs.is_open();
        return _isOpen;
    }

    void File::close() const
    {
        if (_ifs.is_open())
            _ifs.close();
        if (_ofs.is_open())
            _ofs.close();
        _isOpen = false;
    }
} // namespace cppkit::io
