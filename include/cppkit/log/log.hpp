#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <memory>
#include <fstream>
#include <iostream>
#include <cstdarg>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>

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

  class Logger
  {
  public:
    static Logger& instance()
    {
      static Logger inst;
      return inst;
    }

    // 初始化日志到文件（空字符串表示不打开文件，只输出到 stdout/stderr）
    bool init(const std::string& filename = "")
    {
      std::lock_guard lk(mtx_);
      if (!filename.empty())
      {
        ofs_ = std::make_unique<std::ofstream>(filename, std::ios::app);
        if (!ofs_->is_open())
        {
          ofs_.reset();
          return false;
        }
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

    // printf 风格的日志接口，自动附加时间/等级/位置
    void logf(const Level lvl, const char* file, const int line, const char* func, const char* fmt, ...) const
    {
      if (lvl < level_ || level_ == Level::Off)
        return;

      va_list ap;
      va_start(ap, fmt);
      std::string body = vformat(fmt, ap);
      va_end(ap);

      std::ostringstream oss;
      oss << "[" << current_time() << "]"
          << "[" << levelToString(lvl) << "]"
          << "[" << file << ":" << line << " " << func << "] "
          << body << "\n";

      const std::string out = oss.str();

      std::lock_guard lk(mtx_);
      write(out);
    }

    void flush() const
    {
      std::lock_guard<std::mutex> lk(mtx_);
      if (ofs_ && ofs_->is_open())
        ofs_->flush();
      if (to_stdout_)
        std::cout.flush();
    }

  private:
    Logger() : level_(Level::Info), to_stdout_(true)
    {
    }

    ~Logger() { flush(); }

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

    static std::string levelToString(Level l)
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

    static std::string current_time()
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

    // vsnprintf 格式化辅助
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
    std::unique_ptr<std::ofstream> ofs_;
    bool to_stdout_;
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
} // namespace cppkit::log