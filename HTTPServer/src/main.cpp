#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <numeric>
#include <queue>
#include <condition_variable>

using boost::asio::ip::tcp;
using json = nlohmann::json;
using namespace std;
using namespace std::chrono;

// -------------------------
// Structure for Benchmark Stats
// -------------------------
struct Stats {
    atomic<size_t> total_requests{0};
    atomic<size_t> failed_requests{0};
    vector<double> latencies;  // in milliseconds
    mutex m;
};

// -------------------------
// Target Connection Pool Class
// -------------------------
class TargetConnectionPool {
public:
    TargetConnectionPool(boost::asio::io_context &io_context,
                         const string &host,
                         unsigned short port,
                         size_t pool_size)
        : io_context_(io_context), host_(host), port_(port), pool_size_(pool_size) {
        initialize_pool();
    }

    boost::asio::io_context & get_io_context() {
        return io_context_;
    }

    // Acquire a socket from the pool.
    shared_ptr<tcp::socket> acquire() {
        unique_lock<mutex> lock(mtx_);
        while (sockets_.empty()) {
            cv_.wait(lock);
        }
        auto sock = sockets_.front();
        sockets_.pop();
        return sock;
    }

    // Release a socket back to the pool.
    void release(shared_ptr<tcp::socket> sock) {
        lock_guard<mutex> lock(mtx_);
        sockets_.push(sock);
        cv_.notify_one();
    }

private:
    void initialize_pool() {
        for (size_t i = 0; i < pool_size_; ++i) {
            auto sock = make_shared<tcp::socket>(io_context_);
            tcp::resolver resolver(io_context_);
            auto endpoints = resolver.resolve(host_, to_string(port_));
            boost::system::error_code ec;
            boost::asio::connect(*sock, endpoints, ec);
            if (ec) {
                cerr << "Error creating pooled connection: " << ec.message() << endl;
                continue;
            }
            sockets_.push(sock);
        }
    }

    boost::asio::io_context &io_context_;
    string host_;
    unsigned short port_;
    size_t pool_size_;
    queue<shared_ptr<tcp::socket>> sockets_;
    mutex mtx_;
    condition_variable cv_;
};

// -------------------------
// Modified doHttpRequestUsingSocket
// -------------------------
// This function uses an already connected socket to perform a GET request to the target.
// It writes a GET request, reads the response, and returns the measured latency.
double doHttpRequestUsingSocket(tcp::socket &socket, const string &host) {
    auto start = steady_clock::now();
    string req = "GET / HTTP/1.1\r\n"
                 "Host: " + host + "\r\n"
                 "Connection: keep-alive\r\n"
                 "\r\n";
    boost::asio::write(socket, boost::asio::buffer(req));
    
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n\r\n");
    boost::system::error_code ec;
    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), ec)) { }
    
    auto end = steady_clock::now();
    return duration_cast<microseconds>(end - start).count() / 1000.0;
}

// -------------------------
// Modified benchmarkWorker using connection pool
// -------------------------
void benchmarkWorker(const string &target_host, unsigned short target_port,
                     size_t requests_per_thread, Stats &stats, TargetConnectionPool &targetPool) {
    for (size_t i = 0; i < requests_per_thread; ++i) {
        try {
            // Acquire a socket from the pool
            auto sock = targetPool.acquire();

            // Create a new io_context and socket for each request
            boost::asio::io_context io_ctx;
            tcp::resolver resolver(io_ctx);
            auto endpoints = resolver.resolve(target_host, to_string(target_port));
            tcp::socket socket(io_ctx);
            boost::asio::connect(socket, endpoints);

            auto req_start = steady_clock::now();

            // Build and send a GET request.
            string req = "GET / HTTP/1.1\r\n"
                         "Host: " + target_host + "\r\n"
                         "Connection: close\r\n"
                         "\r\n";
            boost::asio::write(socket, boost::asio::buffer(req));

            // Read headers.
            boost::asio::streambuf response;
            boost::asio::read_until(socket, response, "\r\n\r\n");
            boost::system::error_code ec;
            while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), ec)) { }
            auto req_end = steady_clock::now();
            double latency = duration_cast<microseconds>(req_end - req_start).count() / 1000.0;

            {
                lock_guard<mutex> lock(stats.m);
                stats.latencies.push_back(latency);
            }
            stats.total_requests++;
            // Release the socket back to the pool
            targetPool.release(sock);
        } catch (std::exception &e) {
            stats.failed_requests++;
            cerr << "[Worker] Request " << i << " failed: " << e.what() << endl;
        }
    }
}

// -------------------------
// HTTPServer Class (Benchmark Handler)
// -------------------------
class HTTPServer {
public:
    HTTPServer(boost::asio::io_context &io_context, unsigned short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        cout << "Server listening on port " << port << endl;
        start_accept();
    }
private:
    void send_json_response(tcp::socket &socket, const json &data, int status_code = 200) {
        try {
            string json_str = data.dump();
            string response =
                "HTTP/1.1 " + to_string(status_code) + " OK\r\n"
                "Content-Type: application/json; charset=utf-8\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
                "Access-Control-Allow-Headers: Content-Type\r\n"
                "Content-Length: " + to_string(json_str.size()) + "\r\n"
                "Connection: close\r\n"
                "\r\n" +
                json_str;
            boost::asio::write(socket, boost::asio::buffer(response));
        } catch (exception &e) {
            cerr << "Error sending JSON response: " << e.what() << endl;
        }
    }

    void handle_connection(tcp::socket socket) {
        try {
            boost::asio::streambuf buffer;
            boost::system::error_code ec;
            boost::asio::read_until(socket, buffer, "\r\n\r\n", ec);
            if (ec) {
                if (ec == boost::asio::error::eof) return;
                cerr << "Error reading headers: " << ec.message() << endl;
                return;
            }
            
            istream request_stream(&buffer);
            string request_line;
            getline(request_stream, request_line);
            
            // Parse headers for Content-Length.
            string header;
            size_t content_length = 0;
            while (getline(request_stream, header) && header != "\r") {
                if (header.find("Content-Length: ") == 0) {
                    content_length = stoul(header.substr(16));
                }
            }
            
            // Read the body if present.
            string body;
            if (content_length > 0) {
                body.resize(content_length);
                request_stream.read(&body[0], content_length);
            }
            
            istringstream request_line_stream(request_line);
            string method, path, version;
            request_line_stream >> method >> path >> version;
            
            // Handle OPTIONS for CORS.
            if (method == "OPTIONS") {
                string response =
                    "HTTP/1.1 200 OK\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
                    "Access-Control-Allow-Headers: Content-Type\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n"
                    "\r\n";
                boost::asio::write(socket, boost::asio::buffer(response));
                return;
            }
            
            // Handle POST /api/benchmark.
            if (method == "POST" && path == "/api/benchmark") {
                json request_data = json::parse(body);
                int num_threads = request_data["num_threads"];
                int requests_per_thread = request_data["requests_per_thread"];
                string target_host = request_data.value("target_host", "127.0.0.1");
                int target_port = request_data.value("target_port", 8080);
                
                // IMPORTANT: If your target server is the same as this benchmark server,
                // consider using a different port so they don't conflict.
                
                // Create a shared io_context for target requests.
                boost::asio::io_context bench_io_context;
                // Create a connection pool. For example, allocate 2 connections per benchmark thread.
                size_t pool_size = num_threads*num_threads ;
                TargetConnectionPool targetPool(bench_io_context, target_host, target_port, pool_size);
                
                Stats stats;
                vector<thread> threads;
                auto start_time = steady_clock::now();
                for (int i = 0; i < num_threads; ++i) {
                    threads.emplace_back([&]() {
                    benchmarkWorker(target_host, target_port, requests_per_thread, stats, targetPool);
                });

                }
                for (auto &t : threads) {
                    t.join();
                }
                auto end_time = steady_clock::now();
                double duration = duration_cast<milliseconds>(end_time - start_time).count() / 1000.0;
                
                size_t total = stats.total_requests.load();
                size_t failed = stats.failed_requests.load();
                double throughput = (duration > 0) ? (total / duration) : 0.0;
                
                vector<double> latencies;
                {
                    lock_guard<mutex> lock(stats.m);
                    latencies = stats.latencies;
                }
                sort(latencies.begin(), latencies.end());
                double avg_latency = latencies.empty() ? 0 : accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
                
                size_t p50_index = static_cast<size_t>(latencies.size() * 0.50);
                size_t p95_index = static_cast<size_t>(latencies.size() * 0.95);
                size_t p99_index = static_cast<size_t>(latencies.size() * 0.99);
                double p50 = latencies.empty() ? 0 : latencies[min(p50_index, latencies.size()-1)];
                double p95 = latencies.empty() ? 0 : latencies[min(p95_index, latencies.size()-1)];
                double p99 = latencies.empty() ? 0 : latencies[min(p99_index, latencies.size()-1)];
                
                json result;
                result["total_requests"]   = total;
                result["failed_requests"]  = failed;
                result["throughput"]       = throughput;
                result["avg_latency"]      = avg_latency;
                result["p50_latency"]      = p50;
                result["p95_latency"]      = p95;
                result["p99_latency"]      = p99;
                result["duration"]         = duration;
                
                send_json_response(socket, result, 200);
                boost::system::error_code ec2;
                // socket.shutdown(tcp::socket::shutdown_both, ec2);
                // socket.close();
                return;
            } else {
                json err;
                err["error"] = "Not Found";
                send_json_response(socket, err, 404);
                boost::system::error_code ec2;
                socket.shutdown(tcp::socket::shutdown_both, ec2);
                socket.close();
                return;
            }
        } catch (exception &e) {
            cerr << "Exception in connection handler: " << e.what() << endl;
        }
    }
    
    void start_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    thread([this, s = std::move(socket)]() mutable {
                        handle_connection(std::move(s));
                    }).detach();
                }
                start_accept();
            }
        );
    }
    
    tcp::acceptor acceptor_;
};

int main() {
    try {
        boost::asio::io_context io_context;
        HTTPServer server(io_context, 8080);
        io_context.run();
    } catch (exception &e) {
        cerr << "Server exception: " << e.what() << endl;
        return 1;
    }
    return 0;
}
