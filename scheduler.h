#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <queue>
#include <chrono>
#include <vector>
#include <deque>
#include <algorithm>

/*
  PRECISION describes the granularity of scheduled events. 
  For example, using std::chrono::seconds dispatches events
  every second.  std::chrono::milliseconds is probably a good
  choice.
*/

template <typename PRECISION>
class Scheduler
{
public:
  Scheduler();

  template <typename T>
  void scheduleFor(const T& duration,
                   const std::function<void()>& fn);

private:
  // TYPES
  using Clock          = std::chrono::steady_clock;
  using Time           = Clock::time_point;
  using ScheduledEvent = std::pair<Time, std::function<void()>>;

  struct Cmp {
    bool operator() (const ScheduledEvent& lhs,
                     const ScheduledEvent& rhs) const;
  };
  using ScheduleQueue  = std::priority_queue<ScheduledEvent,
                                             std::vector<ScheduledEvent>,
                                             Cmp>;
  
  // METHODS
  void dispatch();
  
  // DATA
  std::mutex              d_mutex;
  std::condition_variable d_newWaitTime;
  ScheduleQueue           d_scheduledEvents;
};

template <typename PRECISION>
template <typename T>
void Scheduler<PRECISION>::scheduleFor(const T& duration,
                                       const std::function<void()>& fn)
{
   std::lock_guard<std::mutex> lock(d_mutex);
   const auto scheduledFor = Clock::now() + duration;
   d_scheduledEvents.push({scheduledFor, fn});
   if (scheduledFor <= d_scheduledEvents.top().first) {
     d_newWaitTime.notify_one();
   }
}

template <typename PRECISION>
bool Scheduler<PRECISION>::Cmp::operator()(const ScheduledEvent& lhs,
                                           const ScheduledEvent& rhs) const
{
  return lhs.first > rhs.first;
}

template <typename PRECISION>
Scheduler<PRECISION>::Scheduler()
  : d_scheduledEvents()
{
  auto t = std::thread(&Scheduler::dispatch, this);
  t.detach();  
}

template <typename PRECISION>
void Scheduler<PRECISION>::dispatch()
{
  while (true) {
    std::unique_lock<std::mutex> lock(d_mutex);
    if (d_scheduledEvents.empty()) {
      d_newWaitTime.wait(lock,
                         [this](){
                           return !d_scheduledEvents.empty();
                         });
    }
    else {
      while (Clock::now() < d_scheduledEvents.top().first) {
        const auto tDiff = d_scheduledEvents.top().first - Clock::now();
        d_newWaitTime.wait_for(lock,
                               std::max(PRECISION(1), // minimum sleep time
                                        std::chrono::duration_cast<PRECISION>(tDiff)));
      }

      const auto event = d_scheduledEvents.top();
      d_scheduledEvents.pop();
      lock.unlock();
      event.second(); // invoking callback
    }
  }    
}
