#pragma once

#include <cstdint>
#include <functional>

namespace cppkit::event
{

  using FileEventCallback = std::function<void(int fd, int mask)>;
  using TimeEventCallback = std::function<int64_t(int64_t id)>;

  constexpr int AE_READABLE = 1;
  constexpr int AE_WRITABLE = 2;
  constexpr int AE_NONE     = 0;

  struct FileEvent
  {
    int mask = AE_NONE;
    FileEventCallback rfileProc;
    FileEventCallback wfileProc;
  };

  struct TimeEvent
  {
    int64_t id;
    int64_t when_ms;  // epoch ms
    TimeEventCallback cb;
  };

  class EventLoop
  {
  public:
    EventLoop();
    ~EventLoop();

    // file events
    bool createFileEvent(int fd, int mask, FileEventCallback cb);
    void deleteFileEvent(int fd, int mask);
    int getFileEvents(int fd) const;

    // time events
    int64_t createTimeEvent(int64_t after_ms, TimeEventCallback cb);
    void deleteTimeEvent(int64_t id);

    // main loop
    void run();
    void stop();

  private:
    struct Impl;
    Impl* impl_;
  };

}  // namespace cppkit::event