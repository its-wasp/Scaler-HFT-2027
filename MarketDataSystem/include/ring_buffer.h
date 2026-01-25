#pragma once

#include <atomic>
#include <cstdint>
#include "market_data.h"

// Lock-free SPSC (Single Producer Single Consumer) Ring Buffer
// Uses cache-line padding to avoid false sharing

static constexpr uint32_t RING_BUFFER_CAPACITY = 1024;

struct RingBuffer {
    // Cache-line padding to avoid false sharing between producer and consumer
    alignas(64) std::atomic<uint32_t> pushPtr{0};
    alignas(64) std::atomic<uint32_t> popPtr{0};

    MarketData buffer[RING_BUFFER_CAPACITY];

    // Push data into the ring buffer (called by producer)
    bool push(const MarketData& data) {
        uint32_t push = pushPtr.load(std::memory_order_relaxed);
        uint32_t pop = popPtr.load(std::memory_order_acquire);

        uint32_t next = (push + 1) % RING_BUFFER_CAPACITY;
        if (next == pop) {
            return false;  // Buffer full
        }

        buffer[push] = data;
        pushPtr.store(next, std::memory_order_release);
        return true;
    }

    // Pop data from the ring buffer (called by consumer)
    bool pop(MarketData& data) {
        uint32_t pop = popPtr.load(std::memory_order_relaxed);
        uint32_t push = pushPtr.load(std::memory_order_acquire);

        if (pop == push) {
            return false;  // Buffer empty
        }

        data = buffer[pop];
        uint32_t next = (pop + 1) % RING_BUFFER_CAPACITY;
        popPtr.store(next, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return popPtr.load(std::memory_order_acquire)
            == pushPtr.load(std::memory_order_acquire);
    }

    bool full() const {
        uint32_t push = pushPtr.load(std::memory_order_acquire);
        uint32_t pop = popPtr.load(std::memory_order_acquire);
        return ((push + 1) % RING_BUFFER_CAPACITY) == pop;
    }

    uint32_t size() const {
        uint32_t push = pushPtr.load(std::memory_order_acquire);
        uint32_t pop = popPtr.load(std::memory_order_acquire);
        return (push + RING_BUFFER_CAPACITY - pop) % RING_BUFFER_CAPACITY;
    }
};

static_assert(std::is_standard_layout<RingBuffer>::value,
              "RingBuffer must be standard layout for shared memory");
