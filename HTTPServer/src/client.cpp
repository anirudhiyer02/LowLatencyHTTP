#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <numeric>
#include <algorithm>
#include <sstream>
#include <nlohmann/json.hpp>
using boost::asio::ip::tcp;
using namespace std::chrono;
using namespace std;
using json = nlohmann::json;
struct BenchmarkStats {
    std::atomic<size_t> total_requests{0};
    std::atomic<size_t> failed_requests{0};
    std::vector<double> latencies;
    std::mutex m;
};
// This function creates a new socket, sends a POST request with a JSON payload to "/api/benchmark",
// reads the complete HTTP response, and then returns it as a string.
std::string send_request(const std::string &host, unsigned short port, const std::string &path = "/api/benchmark") {
    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(host, std::to_string(port));
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        // Create the JSON payload.
        json payload = {
            {"num_threads", 40},
            {"requests_per_thread", 1000},
            {"target_host", "127.0.0.1"},
            {"target_port", 8080}
        };
        std::string payload_str = payload.dump();
        // cout << "[DEBUG] Payload: " << payload_str 
        //      << " (size: " << payload_str.size() << " bytes)" << endl;

        // Build the HTTP POST request.
        std::string request = "POST " + path + " HTTP/1.1\r\n"
                              "Host: " + host + "\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: " + std::to_string(payload_str.size()) + "\r\n"
                              "Connection: close\r\n"  // We close the connection after each response.
                              "\r\n" + payload_str;
        //cout << "[DEBUG] Sending request:\n" << request << endl;

        boost::asio::write(socket, boost::asio::buffer(request));

        // Read response headers (until "\r\n\r\n").
        boost::asio::streambuf response_buffer;
        boost::asio::read_until(socket, response_buffer, "\r\n\r\n");

        // Extract the header block.
        std::string header_block(
            boost::asio::buffers_begin(response_buffer.data()),
            boost::asio::buffers_end(response_buffer.data())
        );
        //] Received headers:\n" << header_block << endl;

        // Find end of header block.
        size_t header_end = header_block.find("\r\n\r\n");
        if (header_end != std::string::npos)
            header_end += 4;
        else
            header_end = header_block.size();

        // Parse headers to get Content-Length.
        std::istringstream header_stream(header_block);
        std::string header_line;
        size_t content_length = 0;
        while (std::getline(header_stream, header_line)) {
            if (!header_line.empty() && header_line.back() == '\r') {
                header_line.pop_back();
            }
            if (header_line.empty())
                break;
            if (header_line.find("Content-Length:") == 0) {
                std::string len_str = header_line.substr(15);
                len_str.erase(0, len_str.find_first_not_of(" "));
                try {
                    content_length = std::stoul(len_str);
                } catch (...) {
                    content_length = 0;
                }
            }
        }
        //cout << "[DEBUG] Parsed Content-Length: " << content_length << endl;

        // Determine how many body bytes are already in the buffer.
        size_t already_read = 0;
        if (response_buffer.size() > header_end)
            already_read = response_buffer.size() - header_end;
        //cout << "[DEBUG] Bytes already read for body: " << already_read << endl;

        // Read the remaining body if necessary.
        if (content_length > 0 && already_read < content_length) {
            boost::asio::read(socket, response_buffer, boost::asio::transfer_exactly(content_length - already_read));
        }

        std::stringstream ss;
        ss << &response_buffer;
        std::string response = ss.str();
       // cout << "[DEBUG] Full response received (" << response.size() << " bytes): " << response << endl;

        return response;
    } catch (std::exception &e) {
        cerr << "send_request error: " << e.what() << endl;
        throw;
    }
}


// Worker thread that calls send_request repeatedly.
void worker_thread(const std::string &host, unsigned short port, size_t num_requests, BenchmarkStats &stats) {
    for (size_t i = 0; i < num_requests; ++i) {
        auto start = high_resolution_clock::now();
        try {
            std::string response = send_request(host, port);
            auto end = high_resolution_clock::now();
            double latency = duration_cast<microseconds>(end - start).count() / 1000.0;
            std::lock_guard<std::mutex> lock(stats.m);
            stats.latencies.push_back(latency);
            stats.total_requests++;
        } catch (std::exception &e) {
            std::lock_guard<std::mutex> lock(stats.m);
            stats.failed_requests++;
            cerr << "Request " << i << " failed: " << e.what() << endl;
        }
    }
}

void print_stats(const BenchmarkStats &stats, double duration) {
    if (stats.latencies.empty()) {
        cout << "No successful requests completed.\n";
        return;
    }
    double sum = std::accumulate(stats.latencies.begin(), stats.latencies.end(), 0.0);
    double avg = sum / stats.latencies.size();
    auto latencies = stats.latencies;
    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[latencies.size() * 0.5];
    double p95 = latencies[latencies.size() * 0.95];
    double p99 = latencies[latencies.size() * 0.99];
    cout << "\nBenchmark Results:\n"
         << "==================\n"
         << "Total Duration: " << duration << " seconds\n"
         << "Total Requests: " << stats.total_requests << "\n"
         << "Failed Requests: " << stats.failed_requests << "\n"
         << "Throughput: " << stats.total_requests / duration << " requests/second\n"
         << "Average Latency: " << avg << " ms\n"
         << "P50 Latency: " << p50 << " ms\n"
         << "P95 Latency: " << p95 << " ms\n"
         << "P99 Latency: " << p99 << " ms\n";
}

int main() {
    // try {
    //     const std::string host = "localhost";
    //     const unsigned short port = 8080;
    //     const size_t num_threads = 4;
    //     const size_t requests_per_thread = 1000;
    //     BenchmarkStats stats;
    //     std::vector<std::thread> threads;
    //     auto start_time = high_resolution_clock::now();
    //     for (size_t i = 0; i < num_threads; ++i) {
    //         threads.emplace_back(worker_thread, host, port, requests_per_thread, std::ref(stats));
    //     }
    //     for (auto &t : threads) {
    //         t.join();
    //     }
    //     auto end_time = high_resolution_clock::now();
    //     double duration = duration_cast<milliseconds>(end_time - start_time).count() / 1000.0;
    //     print_stats(stats, duration);
    // } catch (std::exception &e) {
    //     cerr << "Exception: " << e.what() << endl;
    //     return 1;
    // }
    return 0;
}
