#pragma once

#include <cstdint>
#include <cstring>

// Market data message format
struct MarketData {
    char instrument[16];     // e.g., "RELIANCE"
    double bid;              // Bid price
    double ask;              // Ask price
    uint64_t timestamp_ns;   // Nanosecond timestamp

    MarketData() : bid(0.0), ask(0.0), timestamp_ns(0) {
        std::memset(instrument, 0, sizeof(instrument));
    }

    MarketData(const char* instr, double b, double a, uint64_t ts)
        : bid(b), ask(a), timestamp_ns(ts) {
        std::memset(instrument, 0, sizeof(instrument));
        std::strncpy(instrument, instr, sizeof(instrument) - 1);
    }
};

// Ensure the struct is standard-layout for shared memory
static_assert(std::is_standard_layout<MarketData>::value,
              "MarketData must be standard layout for shared memory");
