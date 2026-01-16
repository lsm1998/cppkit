#include "cppkit/event/ae.hpp"

#include <iostream>

#if defined(__linux__)
#define AE_USE_EPOLL
#include <sys/epoll.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#define AE_USE_KQUEUE
#include <sys/event.h>
#else
#include <poll.h>
#endif
#include <unistd.h>
#include <utility>

namespace cppkit::event
{
  struct EventLoop::Impl
  {
    struct TimeEventCompare
    {
      bool operator()(const TimeEvent& a, const TimeEvent& b) const
      {
        return a.when_ms > b.when_ms;
      }
    };

    Impl() : stopFlag(false), timeId(0)
    {
    }

    std::atomic<bool> stopFlag;

    std::unordered_map<int, FileEvent> fevents;

    // time events stored in priority queue (min heap)
    std::priority_queue<TimeEvent, std::vector<TimeEvent>, TimeEventCompare> tevents;
    std::unordered_set<int64_t> deletedTimeEvents; // 惰性删除集合
    int64_t timeId;

#ifdef AE_USE_EPOLL
    int epfd = -1;
#elif defined(AE_USE_KQUEUE)
    int kq = -1;
#else
    // poll fallback
#endif
  };

  static int64_t mstime()
  {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
  }

  EventLoop::EventLoop() : impl_(std::make_unique<Impl>())
  {
#ifdef AE_USE_EPOLL
    impl_->epfd = epoll_create1(0);
    if (impl_->epfd < 0)
      throw std::runtime_error(std::string("epoll_create1: ") + strerror(errno));
#elif defined(AE_USE_KQUEUE)
    impl_->kq = kqueue();
    if (impl_->kq < 0)
      throw std::runtime_error(std::string("kqueue: ") + strerror(errno));
#endif
  }

  EventLoop::~EventLoop()
  {
#ifdef AE_USE_EPOLL
    if (impl_->epfd >= 0)
      close(impl_->epfd);
#elif defined(AE_USE_KQUEUE)
    if (impl_->kq >= 0)
      close(impl_->kq);
#endif
  }

  bool EventLoop::createFileEvent(int fd, const int mask, const FileEventCallback& cb)
  {
    if (fd < 0)
      return false;
    auto fe = impl_->fevents[fd];

    fe.mask |= mask;
    if (mask & AE_READABLE)
      fe.rfileProc = cb;
    if (mask & AE_WRITABLE)
      fe.wfileProc = cb;

    impl_->fevents[fd] = fe;

#ifdef AE_USE_EPOLL
    struct epoll_event ev{};
    ev.events = 0;
    if (fe.mask & AE_READABLE)
      ev.events |= EPOLLIN;
    if (fe.mask & AE_WRITABLE)
      ev.events |= EPOLLOUT;
    ev.data.fd = fd;
    int op = EPOLL_CTL_MOD;
    if (epoll_ctl(impl_->epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
      if (errno == ENOENT)
        op = EPOLL_CTL_ADD;
      else if (errno != 0)
      {
        /*fallthrough*/
      }
    }
    if (op == EPOLL_CTL_ADD)
    {
      if (epoll_ctl(impl_->epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
      {
        if (errno != EEXIST)
          return false;
      }
    }
#elif defined(AE_USE_KQUEUE)
    struct kevent kev[2];
    int n = 0;
    if (fe.mask & AE_READABLE)
    {
      EV_SET(&kev[n++], fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    }
    if (fe.mask & AE_WRITABLE)
    {
      EV_SET(&kev[n++], fd, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
    }
    if (n)
    {
      if (kevent(impl_->kq, kev, n, nullptr, 0, nullptr) == -1)
        return false;
    }
#else
    // poll fallback: we don't maintain kernel state; will use poll() with fevents
    // map
#endif
    return true;
  }

  void EventLoop::deleteFileEvent(int fd, int mask)
  {
    auto it = impl_->fevents.find(fd);
    if (it == impl_->fevents.end())
      return;
    FileEvent& fe = it->second;
    fe.mask &= ~mask;
    if (mask & AE_READABLE)
      fe.rfileProc = nullptr;
    if (mask & AE_WRITABLE)
      fe.wfileProc = nullptr;
    if (fe.mask == AE_NONE)
      impl_->fevents.erase(it);
#ifdef AE_USE_EPOLL
    if (impl_->fevents.find(fd) == impl_->fevents.end())
    {
      epoll_ctl(impl_->epfd, EPOLL_CTL_DEL, fd, nullptr);
    }
    else
    {
      struct epoll_event ev{};
      if (fe.mask & AE_READABLE)
        ev.events |= EPOLLIN;
      if (fe.mask & AE_WRITABLE)
        ev.events |= EPOLLOUT;
      ev.data.fd = fd;
      epoll_ctl(impl_->epfd, EPOLL_CTL_MOD, fd, &ev);
    }
#elif defined(AE_USE_KQUEUE)
    // kevent remove
    if (mask & AE_READABLE)
    {
      struct kevent kev{};
      EV_SET(&kev, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
      kevent(impl_->kq, &kev, 1, nullptr, 0, nullptr);
    }
    if (mask & AE_WRITABLE)
    {
      struct kevent kev{};
      EV_SET(&kev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
      kevent(impl_->kq, &kev, 1, nullptr, 0, nullptr);
    }
#endif
  }

  int EventLoop::getFileEvents(int fd) const
  {
    auto it = impl_->fevents.find(fd);
    if (it == impl_->fevents.end())
      return AE_NONE;
    return it->second.mask;
  }

  int64_t EventLoop::createTimeEvent(const int64_t after_ms, TimeEventCallback cb) const
  {
    const int64_t id = ++impl_->timeId;
    TimeEvent te;
    te.id = id;
    te.when_ms = mstime() + after_ms;
    te.cb = std::move(cb);
    impl_->tevents.push(te); // O(log n) - 直接插入优先队列
    return id;
  }

  void EventLoop::deleteTimeEvent(int64_t id) const
  {
    // 惰性删除：将id加入删除集合，实际处理时跳过
    impl_->deletedTimeEvents.insert(id);
  }

  void EventLoop::run()
  {
    impl_->stopFlag = false;
    constexpr int MAX_EVENTS = 64;
#ifdef AE_USE_EPOLL
    std::vector<struct epoll_event> events(MAX_EVENTS);
#elif defined(AE_USE_KQUEUE)
    std::vector<struct kevent> events(MAX_EVENTS);
#else
    throw std::runtime_error("poll fallback not implemented in run()");
#endif

    while (!impl_->stopFlag)
    {
      // compute poll timeout from next time event
      int timeout = -1; // -1 means block
      int64_t now = mstime();

      // 跳过已删除的事件，找到下一个有效事件
      while (!impl_->tevents.empty() &&
             impl_->deletedTimeEvents.count(impl_->tevents.top().id) > 0)
      {
        impl_->deletedTimeEvents.erase(impl_->tevents.top().id);
        impl_->tevents.pop();
      }

      if (!impl_->tevents.empty())
      {
        if (const int64_t when = impl_->tevents.top().when_ms; when <= now)
          timeout = 0;
        else
        {
          const int64_t diff = when - now;
          timeout = (diff > INT32_MAX) ? INT32_MAX : static_cast<int>(diff);
        }
      }

#ifdef AE_USE_EPOLL
      int nfds = epoll_wait(impl_->epfd, events.data(), (int) events.size(), timeout);
      if (nfds < 0)
      {
        if (errno == EINTR)
          continue;
        throw std::runtime_error(std::string("epoll_wait: ") + strerror(errno));
      }
      for (int i = 0; i < nfds; ++i)
      {
        int fd = events[i].data.fd;
        int mask = 0;
        if (events[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR))
          mask |= AE_READABLE;
        if (events[i].events & EPOLLOUT)
          mask |= AE_WRITABLE;
        auto it = impl_->fevents.find(fd);
        if (it == impl_->fevents.end())
          continue;
        FileEvent& fe = it->second;
        if ((mask & AE_READABLE) && fe.rfileProc)
          fe.rfileProc(fd, mask);
        if ((mask & AE_WRITABLE) && fe.wfileProc)
          fe.wfileProc(fd, mask);
      }
#elif defined(AE_USE_KQUEUE)
      timespec ts{};
      const timespec* tsp = nullptr;
      if (timeout >= 0)
      {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000000;
        tsp = &ts;
      }
      const int nf_ds = kevent(impl_->kq, nullptr, 0, events.data(), static_cast<int>(events.size()), tsp);
      if (nf_ds < 0)
      {
        if (errno == EINTR)
          continue;
        throw std::runtime_error(std::string("kevent: ") + strerror(errno));
      }
      for (int i = 0; i < nf_ds; ++i)
      {
        int fd = static_cast<int>(events[i].ident);
        int mask = 0;
        if (events[i].filter == EVFILT_READ)
          mask |= AE_READABLE;
        if (events[i].filter == EVFILT_WRITE)
          mask |= AE_WRITABLE;
        auto it = impl_->fevents.find(fd);
        if (it == impl_->fevents.end())
          continue;
        FileEvent& fe = it->second;

        if ((mask & AE_READABLE) && fe.rfileProc)
          fe.rfileProc(fd, mask);
        if ((mask & AE_WRITABLE) && fe.wfileProc)
          fe.wfileProc(fd, mask);
      }
#else
      throw std::runtime_error("no epoll or kqueue implementation");
#endif
      // process time events
      now = mstime();
      while (!impl_->tevents.empty() && impl_->tevents.top().when_ms <= now)
      {
        TimeEvent te = impl_->tevents.top();
        impl_->tevents.pop();

        // 跳过已删除的事件
        if (impl_->deletedTimeEvents.count(te.id) > 0)
        {
          impl_->deletedTimeEvents.erase(te.id);
          continue;
        }

        // 执行回调，如果返回值>0则重新调度
        if (const int64_t next = te.cb(te.id); next > 0)
        {
          te.when_ms = mstime() + next;
          impl_->tevents.push(te); // O(log n) - 重新插入优先队列
        }
      }
    }
  }

  void EventLoop::stop() const
  {
    impl_->stopFlag = true;
  }
} // namespace cppkit::event