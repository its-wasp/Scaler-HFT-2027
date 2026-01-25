#include <iostream>
#include <memory>
#include <random>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include <fmt/core.h>
#include <pthread.h>
#include <sched.h>
#include "../include/market_data.h"
#include "../include/ring_buffer.h"
#include "../include/shm_helper.h"
#include "../include/utils.h"

using boost::asio::ip::tcp;

// Pin thread to specific CPU core to reduce context switches
inline bool set_cpu_affinity(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
}

// TCP Session - handles each client connection
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        socket_.set_option(tcp::no_delay(true));  // Disable Nagle's algorithm
        socket_.set_option(boost::asio::socket_base::send_buffer_size(65536));
        socket_.set_option(boost::asio::socket_base::keep_alive(true));
    }

    void send_data(const std::string& message) {
        auto self = shared_from_this();
        boost::asio::async_write(socket_,
            boost::asio::buffer(message + "\n"),
            [this, self](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    fmt::print("Error sending data: {}\n", ec.message());
                }
            });
    }

private:
    tcp::socket socket_;
};

// TCP Server - accepts client connections
class Server {
public:
    Server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        accept();
    }

    void broadcast(const std::string& message) {
        for (auto& session : sessions_) {
            session->send_data(message);
        }
    }

private:
    void accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    auto session = std::make_shared<Session>(std::move(socket));
                    sessions_.push_back(session);
                    session->start();
                    fmt::print("Client connected. Total clients: {}\n", sessions_.size());
                }
                accept();
            });
    }

    tcp::acceptor acceptor_;
    std::vector<std::shared_ptr<Session>> sessions_;
};

// Market Data Generator - generates simulated market data
class MarketDataGenerator {
public:
    MarketDataGenerator()
        : rng_(std::random_device{}()),
          price_dist_(2800.0, 2900.0),
          spread_dist_(0.25, 1.0) {}

    MarketData generate() {
        double mid_price = price_dist_(rng_);
        double spread = spread_dist_(rng_);

        MarketData data;
        std::strncpy(data.instrument, "RELIANCE", sizeof(data.instrument) - 1);
        data.bid = mid_price - spread / 2.0;
        data.ask = mid_price + spread / 2.0;
        data.timestamp_ns = utils::get_timestamp_ns();

        return data;
    }

private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> price_dist_;
    std::uniform_real_distribution<double> spread_dist_;
};

int main() {
    try {
        fmt::print("Starting Market Data Publisher...\n");

        // Pin main thread to CPU 0
        if (set_cpu_affinity(0)) {
            fmt::print("CPU affinity set: Main thread pinned to CPU 0\n");
        } else {
            fmt::print("Warning: Could not set CPU affinity\n");
        }

        // Create shared memory
        fmt::print("Creating shared memory...\n");
        RingBuffer* ring_buffer = shm::create_shm();

        // Start TCP server
        const short TCP_PORT = 8080;
        fmt::print("Starting TCP server on port {}...\n", TCP_PORT);

        boost::asio::io_context io_context;
        Server server(io_context, TCP_PORT);

        // Run io_context in separate thread (pinned to CPU 1)
        std::thread io_thread([&io_context]() {
            set_cpu_affinity(1);
            io_context.run();
        });

        MarketDataGenerator generator;
        fmt::print("Publisher ready. Generating market data...\n");

        uint64_t message_count = 0;
        while (true) {
            MarketData data = generator.generate();
            std::string json = utils::to_json(data);

            // Send via TCP
            server.broadcast(json);

            // Push to shared memory
            if (!ring_buffer->push(data)) {
                fmt::print("Warning: Shared memory ring buffer is full!\n");
            }

            message_count++;
            if (message_count % 100 == 0) {
                fmt::print("Published {} messages. Latest: {} BID={:.2f} ASK={:.2f}\n",
                    message_count, data.instrument, data.bid, data.ask);
            }

            // 10,000 updates/sec
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        io_context.stop();
        io_thread.join();
        shm::close_shm(ring_buffer);
        shm::cleanup_shm();

    } catch (std::exception& e) {
        fmt::print("Error: {}\n", e.what());
        return 1;
    }

    return 0;
}
