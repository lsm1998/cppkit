#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <fstream>
#include <iostream>
#include <cstdarg>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>

namespace cppkit::log
{
  enum class Level
  {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    Off
  };

  enum class Rotation
  {
    None = 0,
    Size,
    Daily
  };

  class Logger
  {
  public:
    static Logger& instance()
    {
      static Logger inst;
      return inst;
    }

    bool init(const std::string& filename = "")
    {
      std::lock_guard lk(mtx_);
      base_filename_ = filename;
      current_open_path_.clear();
      if (!filename.empty())
      {
        try
        {
          if (const auto parent = std::filesystem::path(filename).parent_path(); !parent.empty())
            std::filesystem::create_directories(parent);
        }
        catch (...)
        {
        }

        try
        {
          ofs_ = std::make_unique<std::ofstream>(filename, std::ios::app);
          if (!ofs_->is_open())
          {
            ofs_.reset();
            return false;
          }
          current_open_path_ = filename;
        }
        catch (...)
        {
          ofs_.reset();
          return false;
        }

        current_date_ = fileDateString();
      }
      return true;
    }

    void setLevel(const Level lvl)
    {
      std::lock_guard lk(mtx_);
      level_ = lvl;
    }

    Level level() const
    {
      return level_;
    }

    void setToStdout(const bool on)
    {
      std::lock_guard lk(mtx_);
      to_stdout_ = on;
    }

    void setRotation(const Rotation r)
    {
      std::lock_guard lk(mtx_);
      rotation_ = r;
    }

    void setRotationSize(const std::uintmax_t bytes)
    {
      std::lock_guard lk(mtx_);
      rotation_size_ = bytes;
    }

    void setMaxFiles(const int max_files)
    {
      std::lock_guard lk(mtx_);
      max_files_ = std::max(1, max_files);
    }

    // 设置归档命名模式 pattern，支持的占位符：
    //   {date}      - 当前日期，格式 YYYY-MM-DD
    //   {year}      - 当前年份，格式 YYYY
    //   {month}     - 当前月份，格式 MM
    //   {day}       - 当前日，格式 DD
    //   {timestamp} - 当前时间戳
    void setFileNamePattern(const std::string& pattern)
    {
      std::lock_guard lk(mtx_);
      filename_pattern_ = pattern;
      current_open_path_.clear();
    }

    void logf(const Level lvl, const char* file, const int line, const char* func, const char* fmt, ...)
    {
      if (lvl < level_ || level_ == Level::Off)
        return;

      va_list ap;
      va_start(ap, fmt);
      const std::string body = vformat(fmt, ap);
      va_end(ap);

      std::ostringstream oss;
      oss << "[" << currentTime() << "]"
          << "[" << levelToString(lvl) << "]"
          << "[" << file << ":" << line << " " << func << "] "
          << body << "\n";

      const std::string out = oss.str();

      std::lock_guard lk(mtx_);

      // 是否需要基于日期切换文件
      if (!filename_pattern_.empty() && rotation_ == Rotation::Daily)
      {
        const std::string today = fileDateString();
        if (current_date_.empty())
          current_date_ = today;
        if (today != current_date_)
        {
          current_date_ = today;
          current_open_path_.clear();
        }
      }

      // 打开或切换到正确的目标文件（如果设置了 filename_pattern_ 则会直接打开展开后的目标）
      ensureLogFileOpenLocked();

      // 如果没有使用 pattern，则保持原来的轮转检查（Size / Daily）
      if (filename_pattern_.empty())
      {
        rotateIfNeededLocked();
      }
      else
      {
        // 使用 pattern 时，目前不对 pattern 文件执行 size 轮转
      }

      write(out);
    }

    void flush()
    {
      std::lock_guard<std::mutex> lk(mtx_);
      if (ofs_ && ofs_->is_open())
        ofs_->flush();
      if (to_stdout_)
        std::cout.flush();
    }

  private:
    Logger()
      : level_(Level::Info),
        rotation_(Rotation::None),
        rotation_size_(10 * 1024 * 1024),
        max_files_(5),
        to_stdout_(true)
    {
    }

    ~Logger() { flush(); }

    // 确保当前打开的输出文件是按照 filename_pattern_ 或 base_filename_ 的目标文件
    // 需要在持有 mtx_ 的前提下调用
    void ensureLogFileOpenLocked()
    {
      if (!filename_pattern_.empty())
      {
        // 使用当前日期作为展开依据（避免使用 timestamp 导致频繁切换）
        const std::string date_for_name = current_date_.empty() ? fileDateString() : current_date_;
        const std::filesystem::path target = expandPattern(filename_pattern_, date_for_name, "");

        std::filesystem::path final_target = target;
        const std::filesystem::path base(base_filename_);
        if (!final_target.has_parent_path())
        {
          auto parent = base.parent_path();
          if (parent.empty())
            parent = ".";
          final_target = parent / final_target;
        }

        const std::string final_path = final_target.string();
        if (final_path != current_open_path_)
        {
          if (ofs_)
          {
            ofs_->flush();
            ofs_->close();
            ofs_.reset();
          }

          try
          {
            if (!final_target.parent_path().empty())
              std::filesystem::create_directories(final_target.parent_path());
          }
          catch (...)
          {
          }
          try { ofs_ = std::make_unique<std::ofstream>(final_path, std::ios::app); }
          catch (...) { ofs_.reset(); }
          current_open_path_ = ofs_ && ofs_->is_open() ? final_path : std::string{};
        }
      }
      else
      {
        if (base_filename_.empty())
          return;

        if (base_filename_ != current_open_path_)
        {
          if (ofs_)
          {
            ofs_->flush();
            ofs_->close();
            ofs_.reset();
          }
          try
          {
            if (auto parent = std::filesystem::path(base_filename_).parent_path(); !parent.empty())
              std::filesystem::create_directories(parent);
          }
          catch (...)
          {
          }
          try { ofs_ = std::make_unique<std::ofstream>(base_filename_, std::ios::app); }
          catch (...) { ofs_.reset(); }
          current_open_path_ = ofs_ && ofs_->is_open() ? base_filename_ : std::string{};
        }
      }
    }

    void write(const std::string& s) const
    {
      if (ofs_ && ofs_->is_open())
      {
        (*ofs_) << s;
      }
      if (to_stdout_)
      {
        std::cout << s;
      }
    }

    void rotateIfNeededLocked()
    {
      if (base_filename_.empty() || rotation_ == Rotation::None)
        return;

      try
      {
        if (rotation_ == Rotation::Size)
        {
          if (std::filesystem::exists(base_filename_))
          {
            if (const auto sz = std::filesystem::file_size(base_filename_);
              static_cast<std::uintmax_t>(sz) >= rotation_size_)
            {
              performRotationLocked();
            }
          }
        }
        else if (rotation_ == Rotation::Daily)
        {
          const std::string today = fileDateString();
          if (current_date_.empty())
            current_date_ = today;
          if (today != current_date_)
          {
            performRotationLocked();
            current_date_ = today;
          }
        }
      }
      catch (...)
      {
      }
    }

    static std::filesystem::path expandPattern(const std::string& pattern,
        const std::string& date,
        const std::string& ts)
    {
      std::string s = pattern;
      auto replace_all = [](std::string& str, const std::string& from, const std::string& to)
      {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos)
        {
          str.replace(pos, from.length(), to);
          pos += to.length();
        }
      };

      if (date.size() >= 10)
      {
        replace_all(s, "{date}", date);
        replace_all(s, "{year}", date.substr(0, 4));
        replace_all(s, "{month}", date.substr(5, 2));
        replace_all(s, "{day}", date.substr(8, 2));
      }
      else
      {
        replace_all(s, "{date}", date);
      }
      replace_all(s, "{timestamp}", ts);
      return s;
    }

    void performRotationLocked()
    {
      if (ofs_)
      {
        ofs_->flush();
        ofs_->close();
        ofs_.reset();
      }

      // 清空当前打开记录（因为基文件可能被移动/重建）
      current_open_path_.clear();

      const std::string ts = timestampString();
      const std::filesystem::path base(base_filename_);
      std::filesystem::path arch_path;

      if (!filename_pattern_.empty())
      {
        const std::string date_for_name = current_date_.empty() ? fileDateString() : current_date_;
        arch_path = expandPattern(filename_pattern_, date_for_name, ts);

        if (!arch_path.has_parent_path())
        {
          auto parent = base.parent_path();
          if (parent.empty())
            parent = ".";
          arch_path = parent / arch_path;
        }

        try
        {
          if (!arch_path.parent_path().empty())
            std::filesystem::create_directories(arch_path.parent_path());
        }
        catch (...)
        {
        }
      }
      else
      {
        arch_path = std::filesystem::path(base_filename_ + "." + ts);
      }

      int idx = 0;
      std::filesystem::path try_path = arch_path;
      while (std::filesystem::exists(try_path))
      {
        ++idx;
        try_path = std::filesystem::path(arch_path.string() + "." + std::to_string(idx));
      }
      arch_path = try_path;

      try
      {
        if (std::filesystem::exists(base))
        {
          std::filesystem::rename(base, arch_path);
        }
      }
      catch (...)
      {
      }

      try
      {
        try
        {
          if (auto parent = base.parent_path(); !parent.empty())
            std::filesystem::create_directories(parent);
        }
        catch (...)
        {
        }
        ofs_ = std::make_unique<std::ofstream>(base_filename_, std::ios::app);
        current_open_path_ = ofs_ && ofs_->is_open() ? base_filename_ : std::string{};
      }
      catch (...)
      {
        ofs_.reset();
        current_open_path_.clear();
      }

      cleanupArchivesLocked(base, arch_path);
    }

    void cleanupArchivesLocked(const std::filesystem::path& base, const std::filesystem::path& sample_archive)
    {
      try
      {
        auto parent = sample_archive.parent_path();
        if (parent.empty())
          parent = ".";

        std::string prefix_name;
        if (!filename_pattern_.empty())
        {
          size_t p = filename_pattern_.find('{');
          std::string raw_prefix = (p == std::string::npos) ? filename_pattern_ : filename_pattern_.substr(0, p);
          std::filesystem::path pname(raw_prefix);
          std::string fname_prefix = pname.filename().string();
          if (fname_prefix.empty())
          {
            fname_prefix = base.filename().string() + ".";
          }
          prefix_name = fname_prefix;
        }
        else
        {
          prefix_name = base.filename().string() + ".";
        }

        std::vector<std::filesystem::directory_entry> entries;
        for (auto& e : std::filesystem::directory_iterator(parent))
        {
          const auto& p = e.path();
          const std::string fname = p.filename().string();
          if (fname.rfind(prefix_name, 0) == 0)
            entries.push_back(e);
        }

        std::sort(entries.begin(),
            entries.end(),
            [](const auto& a, const auto& b)
            {
              try { return a.last_write_time() > b.last_write_time(); }
              catch (...) { return a.path().string() > b.path().string(); }
            });

        for (size_t i = max_files_; i < entries.size(); ++i)
        {
          try { std::filesystem::remove(entries[i].path()); }
          catch (...)
          {
          }
        }
      }
      catch (...)
      {
      }
    }

    static std::string levelToString(const Level l)
    {
      switch (l)
      {
        case Level::Trace:
          return "TRACE";
        case Level::Debug:
          return "DEBUG";
        case Level::Info:
          return "INFO";
        case Level::Warn:
          return "WARN";
        case Level::Error:
          return "ERROR";
        case Level::Fatal:
          return "FATAL";
        default:
          return "UNKNOWN";
      }
    }

    static std::string currentTime()
    {
      using namespace std::chrono;
      const auto now = system_clock::now();
      auto t = system_clock::to_time_t(now);
      const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

      std::tm bt{};
#if defined(_WIN32) || defined(_WIN64)
      localtime_s(&bt, &t);
#else
      localtime_r(&t, &bt);
#endif
      std::ostringstream oss;
      oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << ms.count();
      return oss.str();
    }

    static std::string fileDateString()
    {
      using namespace std::chrono;
      const auto now = system_clock::now();
      auto t = system_clock::to_time_t(now);
      std::tm bt{};
#if defined(_WIN32) || defined(_WIN64)
      localtime_s(&bt, &t);
#else
      localtime_r(&t, &bt);
#endif
      std::ostringstream oss;
      oss << std::put_time(&bt, "%Y-%m-%d");
      return oss.str();
    }

    static std::string timestampString()
    {
      using namespace std::chrono;
      const auto now = system_clock::now();
      auto t = system_clock::to_time_t(now);
      const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
      std::tm bt{};
#if defined(_WIN32) || defined(_WIN64)
      localtime_s(&bt, &t);
#else
      localtime_r(&t, &bt);
#endif
      std::ostringstream oss;
      oss << std::put_time(&bt, "%Y-%m-%d_%H-%M-%S") << "." << std::setw(3) << std::setfill('0') << ms.count();
      return oss.str();
    }

    static std::string vformat(const char* fmt, va_list ap)
    {
      va_list ap_copy;
      va_copy(ap_copy, ap);
      const int len = vsnprintf(nullptr, 0, fmt, ap_copy);
      va_end(ap_copy);
      if (len < 0)
        return {fmt};

      std::vector<char> buf(len + 1);
      vsnprintf(buf.data(), buf.size(), fmt, ap);
      return {buf.data(), buf.data() + len};
    }

    mutable std::mutex mtx_;
    Level level_;
    Rotation rotation_;
    std::uintmax_t rotation_size_;
    int max_files_;
    std::unique_ptr<std::ofstream> ofs_;
    bool to_stdout_;
    std::string base_filename_;
    std::string filename_pattern_;
    std::string current_date_;
    std::string current_open_path_;
  };

#define CK_LOG_TRACE(fmt, ...) cppkit::log::Logger::instance().logf(cppkit::log::Level::Trace, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define CK_LOG_DEBUG(fmt, ...) cppkit::log::Logger::instance().logf(cppkit::log::Level::Debug, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define CK_LOG_INFO(fmt, ...)  cppkit::log::Logger::instance().logf(cppkit::log::Level::Info,  __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define CK_LOG_WARN(fmt, ...)  cppkit::log::Logger::instance().logf(cppkit::log::Level::Warn,  __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define CK_LOG_ERROR(fmt, ...) cppkit::log::Logger::instance().logf(cppkit::log::Level::Error, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define CK_LOG_FATAL(fmt, ...) cppkit::log::Logger::instance().logf(cppkit::log::Level::Fatal, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define CK_LOG_INIT_FILE(path) cppkit::log::Logger::instance().init(path)
#define CK_LOG_SET_LEVEL(lvl) cppkit::log::Logger::instance().setLevel(lvl)
#define CK_LOG_SET_STDOUT(on) cppkit::log::Logger::instance().setToStdout(on)
#define CK_LOG_SET_ROTATION(r) cppkit::log::Logger::instance().setRotation(r)
#define CK_LOG_SET_ROTATION_SIZE(bytes) cppkit::log::Logger::instance().setRotationSize(bytes)
#define CK_LOG_SET_MAX_FILES(n) cppkit::log::Logger::instance().setMaxFiles(n)
#define CK_LOG_SET_FILENAME_PATTERN(p) cppkit::log::Logger::instance().setFileNamePattern(p)
} // namespace cppkit::log