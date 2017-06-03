#include <iostream>
#include <chrono>
#include <random>
#include <cstdlib>

#include <scheduler.h>

using Clock = std::chrono::high_resolution_clock;
using Precision = std::chrono::microseconds;

void event(const std::string& message,
           const Clock::time_point& expected)
{
  static int count = 0;
  static int diffs;
  
  const auto tDiff = Clock::now() - expected;
  const auto microSeconds = std::chrono::duration_cast<Precision>(tDiff).count();

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

  schedule(scheduler, "hello, world (immediate)", std::chrono::microseconds(-404));
  schedule(scheduler, "hello, world", std::chrono::milliseconds(171));
  schedule(scheduler, "hello, world (3s)", std::chrono::seconds(3));
  schedule(scheduler, "hello, world (final)", std::chrono::seconds(21));

  std::random_device rd;  
  std::mt19937 gen(rd()); 
  std::uniform_int_distribution<> dis(0, 20*1000);
  for (int i=0; i<1000; ++i) {
    schedule(scheduler, "random event", std::chrono::milliseconds(dis(gen)));
  }

  std::cin.get();
}
