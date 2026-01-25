#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include "ring_buffer.h"

namespace shm {

static constexpr const char* SHM_NAME = "/market_data_shm";

// Create and initialize shared memory (for publisher)
inline RingBuffer* create_shm() {
    shm_unlink(SHM_NAME);

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        throw std::runtime_error("Failed to create shared memory: " + std::string(strerror(errno)));
    }

    if (ftruncate(fd, sizeof(RingBuffer)) == -1) {
        close(fd);
        shm_unlink(SHM_NAME);
        throw std::runtime_error("Failed to set shared memory size: " + std::string(strerror(errno)));
    }

    void* addr = mmap(nullptr, sizeof(RingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        shm_unlink(SHM_NAME);
        throw std::runtime_error("Failed to map shared memory: " + std::string(strerror(errno)));
    }

    close(fd);

    // Initialize ring buffer using placement new
    RingBuffer* ring = new (addr) RingBuffer();
    return ring;
}

// Open existing shared memory (for consumer)
inline RingBuffer* open_shm() {
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        throw std::runtime_error("Failed to open shared memory: " + std::string(strerror(errno)));
    }

    void* addr = mmap(nullptr, sizeof(RingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        throw std::runtime_error("Failed to map shared memory: " + std::string(strerror(errno)));
    }

    close(fd);
    return static_cast<RingBuffer*>(addr);
}

// Close shared memory mapping
inline void close_shm(RingBuffer* ring) {
    if (ring != nullptr) {
        munmap(ring, sizeof(RingBuffer));
    }
}

// Cleanup shared memory (for publisher on exit)
inline void cleanup_shm() {
    shm_unlink(SHM_NAME);
}

} // namespace shm
