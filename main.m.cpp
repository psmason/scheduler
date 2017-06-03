#include <iostream>
#include <chrono>
#include <random>
#include <cstdlib>
#include <thread>

#include <scheduler.h>

using ReportingClock     = std::chrono::high_resolution_clock;
using ReportingPrecision = std::chrono::microseconds;

void event(const std::string& message,
           const ReportingClock::time_point& expected)
{
  static int count = 0;
  static int diffs;
  
  const auto tDiff = ReportingClock::now() - expected;
  const auto microSeconds = std::chrono::duration_cast<ReportingPrecision>(tDiff).count();

  count += 1;
  diffs += abs(microSeconds);
    
  std::cout << message
            << " (delay="
            << microSeconds
            << " micro seconds; count="
            << count
            << "; average="
            << static_cast<double>(diffs)/count
            << ")"
            << std::endl;
}

template <typename PRECISION, typename T>
void schedule(Scheduler<PRECISION>& scheduler,
              const std::string& message,
              const T delay)
{
  const auto fn = std::bind(event,
                            message,
                            ReportingClock::now() + delay);
  scheduler.scheduleFor(delay, fn);
}

template <typename PRECISION>
void scheduleRandomly(Scheduler<PRECISION>& scheduler)
{
  std::random_device rd;  
  std::mt19937 gen(rd()); 
  std::uniform_int_distribution<> dis(0, 20*1000);
  for (int i=0; i<1000; ++i) {
    schedule(scheduler, "random event", std::chrono::milliseconds(dis(gen)));
  }
}
           
int main() {
  Scheduler<std::chrono::milliseconds> scheduler;

  schedule(scheduler, "hello, world (immediate)", std::chrono::microseconds(-404));
  schedule(scheduler, "hello, world", std::chrono::milliseconds(171));
  schedule(scheduler, "hello, world (3s)", std::chrono::seconds(3));
  schedule(scheduler, "hello, world (final)", std::chrono::seconds(21));

  for (int i=0; i<4; ++i) {
    std::thread t(scheduleRandomly<std::chrono::milliseconds>,
                  std::ref(scheduler));
    t.detach();
  }

  std::cin.get();
}
