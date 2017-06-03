#include <scheduler.h>

#include <thread>

bool Scheduler::Cmp::operator()(const ScheduledEvent& lhs,
                                const ScheduledEvent& rhs) const
{
  return lhs.first > rhs.first;
}

Scheduler::Scheduler()
  : d_scheduledEvents()
{
  auto t = std::thread(&Scheduler::dispatch, this);
  t.detach();  
}

bool Scheduler::RequestQueue::waitFor(ScheduledEvent* event,
                                      const Precision& t)
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

void Scheduler::RequestQueue::add(const ScheduledEvent& event)
{
  std::unique_lock<std::mutex> l(d_queueMutex);
  d_queue.push_back(event);
  d_isNotEmpty.notify_one();
}

Scheduler::Clock::duration Scheduler::getWaitTime()
{
    std::lock_guard<std::mutex> lock(d_scheduledMutex);
    if (d_scheduledEvents.empty()) {
      return std::chrono::seconds(10);
    }
    return d_scheduledEvents.top().first - Clock::now();
}

void Scheduler::dispatch()
{
  while (true) {
    auto waitTime = std::chrono::duration_cast<Precision>(getWaitTime());
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
