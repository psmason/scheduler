#include <scheduler.h>

bool Scheduler::Cmp::operator()(const ScheduledEvent& lhs,
                                const ScheduledEvent& rhs) const
{
  return lhs.first > rhs.first;
}

Scheduler::Scheduler()
  : d_scheduledEvents()
{
  d_dispatcher = std::thread(&Scheduler::dispatch, this);
  d_dispatcher.detach();  
}

bool Scheduler::RequestQueue::waitFor(Event* event,
                                      const std::chrono::milliseconds& t)
{
  std::unique_lock<std::mutex> l(d_queueMutex);
  const auto rc = d_isNotEmpty.wait_for(l, t,
                                        [this](){
                                          return !d_queue.empty();
                                        });
  if (!rc) {
    return false;
  }
  
  *event = d_queue.front();
  d_queue.pop_front();
  return true;
}

void Scheduler::RequestQueue::add(const Event& event)
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

void Scheduler::scheduleFor(const std::chrono::milliseconds& ms,
                            const std::function<void()>& fn)
{
  d_requestQueue.add({Clock::now() + ms, fn});
}

void Scheduler::dispatch()
{
  while (true) {
    auto waitTime = std::chrono::duration_cast<Precision>(getWaitTime());
    ScheduledEvent event;
    const auto rc = d_requestQueue.waitFor(&event, waitTime);
    if (rc) {
      std::lock_guard<std::mutex> lock(d_scheduledMutex);
      d_scheduledEvents.push(event);
    }
    else {      
      std::lock_guard<std::mutex> lock(d_scheduledMutex);
      // may have timed out in the waitFor(...) call.
      if (!d_scheduledEvents.empty()) {
        d_scheduledEvents.top().second();
        d_scheduledEvents.pop();
      }
    }
  }
}
