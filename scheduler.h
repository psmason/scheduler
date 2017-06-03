#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <queue>
#include <chrono>
#include <vector>
#include <deque>

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

  struct RequestQueue {
    std::deque<ScheduledEvent> d_queue;
    std::mutex                 d_queueMutex;
    std::condition_variable    d_isNotEmpty;

    bool waitFor(ScheduledEvent* event,
                 const PRECISION& t);
    void add(const ScheduledEvent& event);
  };
  
  // METHODS
  void dispatch();
  Clock::duration getWaitTime();
  
  // DATA
  std::mutex    d_scheduledMutex;
  ScheduleQueue d_scheduledEvents;
  RequestQueue  d_requestQueue;
};

template <typename PRECISION>
template <typename T>
void Scheduler<PRECISION>::scheduleFor(const T& duration,
                                       const std::function<void()>& fn)
{
  d_requestQueue.add({Clock::now() + duration, fn});
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
bool Scheduler<PRECISION>::RequestQueue::waitFor(ScheduledEvent* event,
                                                 const PRECISION& t)
{
  std::unique_lock<std::mutex> l(d_queueMutex);
  if (!d_isNotEmpty.wait_for(l, t,
                             [this](){
                               return !d_queue.empty();
                             })) {
    return false;
  }
  
  *event = d_queue.front();
  d_queue.pop_front();
  return true;
}

template <typename PRECISION>
void Scheduler<PRECISION>::RequestQueue::add(const ScheduledEvent& event)
{
  std::unique_lock<std::mutex> l(d_queueMutex);
  d_queue.push_back(event);
  d_isNotEmpty.notify_one();
}

template <typename PRECISION>
Scheduler<PRECISION>::Clock::duration Scheduler<PRECISION>::getWaitTime()
{
    std::lock_guard<std::mutex> lock(d_scheduledMutex);
    if (d_scheduledEvents.empty()) {
      return std::chrono::seconds(10);
    }
    return d_scheduledEvents.top().first - Clock::now();
}

template <typename PRECISION>
void Scheduler<PRECISION>::dispatch()
{
  while (true) {
    auto waitTime = std::chrono::duration_cast<PRECISION>(getWaitTime());
    ScheduledEvent event;
    const auto rc = d_requestQueue.waitFor(&event, waitTime);

    std::lock_guard<std::mutex> lock(d_scheduledMutex);
    if (rc) {
      d_scheduledEvents.push(event);
    }
    else {      
      // may have timed out in the waitFor(...) call.
      if (!d_scheduledEvents.empty()) {
        d_scheduledEvents.top().second();
        d_scheduledEvents.pop();
      }
    }
  }
}
