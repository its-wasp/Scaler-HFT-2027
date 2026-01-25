#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstring>
#include <pthread.h>
#include <sched.h>
#include <fmt/core.h>
#include "../include/market_data.h"
#include "../include/ring_buffer.h"
#include "../include/shm_helper.h"
#include "../include/utils.h"

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
    bool busy_wait = false;
    int cpu_core = 2;  // Default: separate from publisher

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--busy-wait") == 0 || strcmp(argv[i], "-b") == 0) {
            busy_wait = true;
        } else if (strcmp(argv[i], "--cpu") == 0 && i + 1 < argc) {
            cpu_core = std::atoi(argv[++i]);
        }
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        fmt::print("Starting Shared Memory Consumer...\n");

        if (set_cpu_affinity(cpu_core)) {
            fmt::print("CPU affinity set: Pinned to CPU {}\n", cpu_core);
        } else {
            fmt::print("Warning: Could not set CPU affinity\n");
        }

        if (busy_wait) {
            fmt::print("Mode: BUSY-WAIT (ultra-low latency, high CPU usage)\n");
        } else {
            fmt::print("Mode: SLEEP (low CPU usage, ~1us added latency)\n");
        }

        fmt::print("Opening shared memory...\n");
        RingBuffer* ring_buffer = shm::open_shm();

        fmt::print("Consumer ready. Waiting for market data from shared memory...\n");

        uint64_t message_count = 0;
        MarketData data;

        while (running) {
            if (ring_buffer->pop(data)) {
                uint64_t receive_ts = utils::get_timestamp_ns();
                uint64_t latency_ns = receive_ts - data.timestamp_ns;

                fmt::print("[{}] {} BID={:.2f} ASK={:.2f} (latency: {} ns)\n",
                    utils::format_timestamp(receive_ts),
                    data.instrument,
                    data.bid,
                    data.ask,
                    latency_ns);

                message_count++;
            } else {
                if (!busy_wait) {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
                // Busy-wait mode: spin without sleeping for lowest latency
            }
        }

        fmt::print("\nShutting down. Total messages received: {}\n", message_count);
        shm::close_shm(ring_buffer);

    } catch (std::exception& e) {
        fmt::print("Error: {}\n", e.what());
        return 1;
    }

    return 0;
}
