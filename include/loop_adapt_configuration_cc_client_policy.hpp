#ifndef POLICY_HPP
#define POLICY_HPP

#include <chrono>
#include <string>

class Policy
{
public:

  virtual ~Policy() {}

  virtual void start() = 0;

  virtual void stop() = 0;

  virtual double getRating() = 0;

  virtual std::string getMeasurementString() = 0;
};

class TimePolicy: public Policy
{
  std::chrono::time_point<std::chrono::steady_clock> start_t, stop_t;

public:
  void start()
  {
    start_t = std::chrono::steady_clock::now();
  }

  void stop()
  {
    stop_t = std::chrono::steady_clock::now();
  }

  double getRating()
  {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(stop_t - start_t).count();
  }

  std::string getMeasurementString()
  {
    return ("runtime," + std::to_string(getRating()));
  }
};

#endif // POLICY_HPP