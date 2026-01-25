#pragma once

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include "market_data.h"

namespace utils {

// Get current timestamp in nanoseconds
inline uint64_t get_timestamp_ns() {
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
    return nanos.time_since_epoch().count();
}

// Format timestamp for logging (HH:MM:SS.nanoseconds)
inline std::string format_timestamp(uint64_t timestamp_ns) {
    auto seconds = timestamp_ns / 1'000'000'000;
    auto nanos = timestamp_ns % 1'000'000'000;

    auto time = std::chrono::system_clock::from_time_t(seconds);
    auto time_t_val = std::chrono::system_clock::to_time_t(time);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_val), "%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(9) << nanos;

    return ss.str();
}

// Convert MarketData to JSON string
inline std::string to_json(const MarketData& data) {
    char buffer[256];
    int len = std::snprintf(buffer, sizeof(buffer),
        "{\"instrument\":\"%s\",\"bid\":%.2f,\"ask\":%.2f,\"timestamp_ns\":%lu}",
        data.instrument,
        data.bid,
        data.ask,
        data.timestamp_ns);
    return std::string(buffer, len);
}

// Parse JSON to MarketData
inline bool from_json(const std::string& json, MarketData& data) {
    char instrument[16];
    double bid, ask;
    unsigned long long timestamp_ns;

    int parsed = std::sscanf(json.c_str(),
        "{\"instrument\":\"%15[^\"]\",\"bid\":%lf,\"ask\":%lf,\"timestamp_ns\":%llu}",
        instrument,
        &bid,
        &ask,
        &timestamp_ns);

    if (parsed != 4) {
        return false;
    }

    std::memset(data.instrument, 0, sizeof(data.instrument));
    std::strncpy(data.instrument, instrument, sizeof(data.instrument) - 1);
    data.bid = bid;
    data.ask = ask;
    data.timestamp_ns = timestamp_ns;

    return true;
}

} // namespace utils
