#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

struct calculation_result_t
{
  double milliseconds;
  double area;
};

constexpr int    error_exit_code         = 1;
constexpr int    arguments_without_seed  = 2;
constexpr int    arguments_with_seed     = 3;
constexpr int    output_precision        = 3;
constexpr double square_area_coefficient = 4.0;

static int printError(const char *message)
{
  std::cerr << message << '\n';
  return error_exit_code;
}

static calculation_result_t calculateCircleArea(const std::uint64_t tries, const std::uint64_t seed,
                                                const long long radius, const std::size_t thread_count)
{
  const auto                 start_time = std::chrono::steady_clock::now();
  std::vector<std::uint64_t> hits_by_thread(thread_count, 0);
  std::vector<std::thread>   threads;
  threads.reserve(thread_count);

  const std::uint64_t base_tries      = tries / thread_count;
  const std::uint64_t remaining_tries = tries % thread_count;
  const auto          double_radius   = static_cast<double>(radius);
  const double        radius_squared  = double_radius * double_radius;

  for (std::size_t thread_index = 0; thread_index < thread_count; ++thread_index)
  {
    const std::uint64_t current_thread_tries = base_tries + (thread_index < remaining_tries ? 1 : 0);

    threads.emplace_back(
      [&, thread_index, current_thread_tries]()
      {
        std::mt19937_64                generator(seed + thread_index);
        std::uniform_real_distribution distribution(-double_radius, double_radius);
        std::uint64_t                  hits = 0;

        for (std::uint64_t attempt = 0; attempt < current_thread_tries; ++attempt)
        {
          const double x = distribution(generator);
          const double y = distribution(generator);

          if (x * x + y * y <= radius_squared)
          {
            ++hits;
          }
        }

        hits_by_thread[thread_index] = hits;
      });
  }

  for (std::thread &thread : threads)
  {
    thread.join();
  }

  std::uint64_t total_hits = 0;

  for (const std::uint64_t hits : hits_by_thread)
  {
    total_hits += hits;
  }

  const double square_area  = square_area_coefficient * double_radius * double_radius;
  const double circle_area  = square_area * static_cast<double>(total_hits) / static_cast<double>(tries);
  const auto   finish_time  = std::chrono::steady_clock::now();
  const double milliseconds = std::chrono::duration<double, std::milli>(finish_time - start_time).count();

  return {milliseconds, circle_area};
}

int main(int argc, char *argv[])
{
  if (argc != arguments_without_seed && argc != arguments_with_seed)
  {
    return printError("Error: invalid number of command-line arguments.");
  }

  long long          tries_argument = 0;
  std::istringstream tries_input(argv[1]);
  if (!(tries_input >> tries_argument) || tries_argument <= 0)
  {
    return printError("Error: tries must be a positive integer.");
  }
  tries_input >> std::ws;
  if (!tries_input.eof())
  {
    return printError("Error: tries must be a positive integer.");
  }

  long long seed_argument = 0;
  if (argc == arguments_with_seed)
  {
    std::istringstream seed_input(argv[2]);
    if (!(seed_input >> seed_argument) || seed_argument < 0)
    {
      return printError("Error: seed must be a non-negative integer.");
    }
    seed_input >> std::ws;
    if (!seed_input.eof())
    {
      return printError("Error: seed must be a non-negative integer.");
    }
  }

  const auto tries = static_cast<std::uint64_t>(tries_argument);
  const auto seed  = static_cast<std::uint64_t>(seed_argument);
  std::cout << std::fixed << std::setprecision(output_precision);

  long long radius                = 0;
  long long thread_count_argument = 0;
  while (true)
  {
    if (!(std::cin >> radius))
    {
      if (std::cin.eof())
      {
        return 0;
      }
      return printError("Error: invalid input.");
    }

    if (!(std::cin >> thread_count_argument))
    {
      return printError("Error: invalid input.");
    }

    if (radius <= 0 || thread_count_argument <= 0)
    {
      return printError("Error: radius and thread count must be positive integers.");
    }

    const auto                 thread_count = static_cast<std::size_t>(thread_count_argument);
    const calculation_result_t result       = calculateCircleArea(tries, seed, radius, thread_count);
    std::cout << result.milliseconds << ' ' << result.area << '\n';
  }
}