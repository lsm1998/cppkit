#pragma once

#include "common.hpp"
#include <memory>
#include <string>
#include <mutex>
#include <fstream>
#include <iostream>
#include <cstdarg>
#include <vector>
#include <sstream>
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
    static Logger& instance();

    bool init(const std::string& filename = "");

    void setLevel(Level lvl);

    Level level() const;

    void setToStdout(bool on);

    void setRotation(Rotation r);

    void setRotationSize(std::uintmax_t bytes);

    void setMaxFiles(int max_files);

    // 设置归档命名模式 pattern，支持的占位符：
    //   {date}      - 当前日期，格式 YYYY-MM-DD
    //   {year}      - 当前年份，格式 YYYY
    //   {month}     - 当前月份，格式 MM
    //   {day}       - 当前日，格式 DD
    //   {timestamp} - 当前时间戳，格式 YYYY-MM-DD_HH-MM-SS.mmm
    void setFileNamePattern(const std::string& pattern);

    void logf(Level lvl, const char* file, int line, const char* func, const char* fmt, ...);

    void flush() const;

  private:
    Logger()
      : level_(Level::Info),
        rotation_(Rotation::None),
        rotation_size_(DEFAULT_LOG_ROTATION_SIZE),
        max_files_(DEFAULT_MAX_FILES),
        to_stdout_(true)
    {
    }

    ~Logger();

    void ensureLogFileOpenLocked();

    void write(const std::string& s) const;

    void rotateIfNeededLocked();

    static std::filesystem::path expandPattern(const std::string& pattern,
        const std::string& date,
        const std::string& ts);

    void performRotationLocked();

    void cleanupArchivesLocked(const std::filesystem::path& base, const std::filesystem::path& sample_archive);

    static std::string levelToString(Level l);

    static std::string currentTime();

    static std::string fileDateString();

    static std::string timestampString();

    static std::string vformat(const char* fmt, va_list ap);

    // 互斥锁 保证线程安全
    mutable std::mutex mtx_;

    // 日志级别
    Level level_;

    // 归档策略
    Rotation rotation_;

    // 归档大小阈值，单位字节
    std::uintmax_t rotation_size_;

    // 最多保留的归档文件数
    int max_files_;

    // 日志文件输出流
    std::unique_ptr<std::ofstream> ofs_;

    // 是否输出到标准输出
    bool to_stdout_;

    // 基础日志文件路径
    std::string base_filename_;

    // 归档文件名模式
    std::string filename_pattern_;

    // 当前日志日期（用于每日归档）
    std::string current_date_;

    // 当前打开的日志文件流路径
    std::string current_open_path_;
  };

  // Trace 级别日志
#define CK_LOG_TRACE(fmt, ...) cppkit::log::Logger::instance().logf(cppkit::log::Level::Trace, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

  // Debug 级别日志
#define CK_LOG_DEBUG(fmt, ...) cppkit::log::Logger::instance().logf(cppkit::log::Level::Debug, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

  // Info 级别日志
#define CK_LOG_INFO(fmt, ...)  cppkit::log::Logger::instance().logf(cppkit::log::Level::Info,  __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

  // Warn 级别日志
#define CK_LOG_WARN(fmt, ...)  cppkit::log::Logger::instance().logf(cppkit::log::Level::Warn,  __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

  // Error 级别日志
#define CK_LOG_ERROR(fmt, ...) cppkit::log::Logger::instance().logf(cppkit::log::Level::Error, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

  // Fatal 级别日志
#define CK_LOG_FATAL(fmt, ...) cppkit::log::Logger::instance().logf(cppkit::log::Level::Fatal, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)


  // 设置日志文件路径
#define CK_LOG_INIT_FILE(path) cppkit::log::Logger::instance().init(path)

  // 设置日志级别
#define CK_LOG_SET_LEVEL(lvl) cppkit::log::Logger::instance().setLevel(lvl)

  // 设置是否输出到标准输出
#define CK_LOG_SET_STDOUT(on) cppkit::log::Logger::instance().setToStdout(on)

  // 设置日志归档策略
#define CK_LOG_SET_ROTATION(r) cppkit::log::Logger::instance().setRotation(r)

  // 设置日志归档大小阈值，单位字节
#define CK_LOG_SET_ROTATION_SIZE(bytes) cppkit::log::Logger::instance().setRotationSize(bytes)

  // 设置最多保留的归档文件数
#define CK_LOG_SET_MAX_FILES(n) cppkit::log::Logger::instance().setMaxFiles(n)

  // 设置归档文件名模式
#define CK_LOG_SET_FILENAME_PATTERN(p) cppkit::log::Logger::instance().setFileNamePattern(p)
} // namespace cppkit::log