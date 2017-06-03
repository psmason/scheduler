#include <iostream>
#include <chrono>

#include <scheduler.h>

using Clock = std::chrono::steady_clock;

void event(const std::string& message,
           const Clock::time_point& expected)
{
  const auto tDiff = Clock::now() - expected;
  std::cout << message
            << " (delay="
            << std::chrono::duration_cast<std::chrono::microseconds>(tDiff).count()
            << " micro seconds)"
            << std::endl;
}

template <typename T>
void schedule(Scheduler& scheduler,
              const std::string& message,
              const T delay)
{
  const auto fn = std::bind(event,
                            message,
                            Clock::now() + delay);
  scheduler.scheduleFor(delay, fn);
}
           
int main() {
  Scheduler scheduler;
  schedule(scheduler, "hello, world (immediate)", std::chrono::milliseconds(-44));
  schedule(scheduler, "hello, world", std::chrono::milliseconds(171));
  schedule(scheduler, "hello, world (3s)", std::chrono::seconds(3));
  schedule(scheduler, "hello, world (11s)", std::chrono::seconds(11));

  std::cin.get();
}
