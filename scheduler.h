#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>

#include <functional>
#include <queue>
#include <chrono>
#include <vector>
#include <deque>

class Scheduler
{
public:
  Scheduler();
  void scheduleFor(const std::chrono::milliseconds& ms,
                   const std::function<void()>& fn);

private:
  // TYPES
  using Clock          = std::chrono::steady_clock;
  using Time           = Clock::time_point;
  using Precision      = std::chrono::milliseconds;
  using ScheduledEvent = std::pair<Time, std::function<void()>>;

  struct Cmp {
    bool operator() (const ScheduledEvent& lhs,
                     const ScheduledEvent& rhs) const;
  };
  using ScheduleQueue  = std::priority_queue<ScheduledEvent,
                                             std::vector<ScheduledEvent>,
                                             Cmp>;

  struct RequestQueue {
    using Event = Scheduler::ScheduledEvent;
    std::deque<Event>       d_queue;
    std::mutex              d_queueMutex;
    std::condition_variable d_isNotEmpty;

    bool waitFor(Event* event,
                 const std::chrono::milliseconds& t);
    void add(const Event& event);
  };
  
  // METHODS
  void dispatch();
  Clock::duration getWaitTime();
  
  // DATA
  std::thread   d_dispatcher;
  std::mutex    d_scheduledMutex;
  ScheduleQueue d_scheduledEvents;
  RequestQueue  d_requestQueue;
};

