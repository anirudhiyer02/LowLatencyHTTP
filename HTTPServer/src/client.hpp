// benchmark_client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <nlohmann/json.hpp>
#include <memory>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <string>

// Use your chosen namespace (or fully qualify types)
using json = nlohmann::json;

// Forward declare your benchmarking function.
json runBenchmarkClient(const std::string &target_host, unsigned short target_port,
                        int num_threads, int requests_per_thread);

#endif // BENCHMARK_CLIENT_HPP
