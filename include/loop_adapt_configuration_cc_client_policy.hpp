#ifndef POLICY_HPP
#define POLICY_HPP

#include <chrono>

class Policy
{
public:

  virtual ~Policy() {}

  virtual void start() = 0;

  virtual void stop() = 0;

  virtual double get_rating() = 0;
};

class TimePolicy: public Policy
{
public:

  void start()
  {
    start_t = std::chrono::steady_clock::now();
  }

  void stop()
  {
    stop_t = std::chrono::steady_clock::now();
  }

  double get_rating()
  {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(stop_t - start_t).count();
  }

private:
  std::chrono::time_point<std::chrono::steady_clock> start_t, stop_t;
};

#endif