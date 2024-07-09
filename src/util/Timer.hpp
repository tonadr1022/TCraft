#pragma once

#include <chrono>

using namespace std::chrono;

namespace engine {

class Timer {
 public:
  Timer() { Start(); }
  ~Timer() { ElapsedMicro(); }

  void Start() { start_time_ = high_resolution_clock::now(); };

  double ElapsedSeconds() { return ElapsedMicro() * 0.000001; }

  double ElapsedMS() { return ElapsedMicro() * 0.001; }

  uint64_t ElapsedMicro() {
    auto end_time = high_resolution_clock::now();
    start_ = time_point_cast<microseconds>(start_time_).time_since_epoch().count();
    end_ = time_point_cast<microseconds>(end_time).time_since_epoch().count();
    return end_ - start_;
  }

  void Reset() { start_time_ = high_resolution_clock::now(); }

 private:
  uint64_t start_, end_;
  time_point<high_resolution_clock> start_time_;
};

}  // namespace engine
