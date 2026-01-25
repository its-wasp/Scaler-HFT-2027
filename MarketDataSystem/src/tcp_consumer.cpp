#include <iostream>
#include <cstring>
#include <boost/asio.hpp>
#include <fmt/core.h>
#include <csignal>
#include <pthread.h>
#include <sched.h>
#include "../include/market_data.h"
#include "../include/utils.h"

using boost::asio::ip::tcp;

volatile sig_atomic_t running = 1;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        running = 0;
    }
}

// Pin thread to specific CPU core to reduce context switches
inline bool set_cpu_affinity(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
}

int main(int argc, char* argv[]) {
    int cpu_core = 3;  // Default: separate from others

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--cpu") == 0 && i + 1 < argc) {
            cpu_core = std::atoi(argv[++i]);
        }
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        fmt::print("Starting TCP Consumer...\n");

        if (set_cpu_affinity(cpu_core)) {
            fmt::print("CPU affinity set: Pinned to CPU {}\n", cpu_core);
        } else {
            fmt::print("Warning: Could not set CPU affinity\n");
        }

        boost::asio::io_context io_context;

        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("127.0.0.1", "8080");

        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        socket.set_option(tcp::no_delay(true));  // Disable Nagle's algorithm
        socket.set_option(boost::asio::socket_base::receive_buffer_size(65536));

        fmt::print("Connected to publisher at 127.0.0.1:8080\n");
        fmt::print("Consumer ready. Waiting for market data over TCP...\n");

        uint64_t message_count = 0;

        while (running) {
            boost::asio::streambuf buffer;
            boost::system::error_code ec;

            boost::asio::read_until(socket, buffer, '\n', ec);

            if (ec) {
                if (ec == boost::asio::error::eof) {
                    fmt::print("Connection closed by publisher\n");
                } else {
                    fmt::print("Error reading from socket: {}\n", ec.message());
                }
                break;
            }

            uint64_t receive_ts = utils::get_timestamp_ns();

            std::istream is(&buffer);
            std::string json_message;
            std::getline(is, json_message);

            MarketData data;
            if (utils::from_json(json_message, data)) {
                uint64_t latency_ns = receive_ts - data.timestamp_ns;

                fmt::print("[{}] {} BID={:.2f} ASK={:.2f} (latency: {} ns)\n",
                    utils::format_timestamp(receive_ts),
                    data.instrument,
                    data.bid,
                    data.ask,
                    latency_ns);

                message_count++;
            } else {
                fmt::print("Warning: Failed to parse JSON: {}\n", json_message);
            }
        }

        fmt::print("\nShutting down. Total messages received: {}\n", message_count);

    } catch (std::exception& e) {
        fmt::print("Error: {}\n", e.what());
        return 1;
    }

    return 0;
}
