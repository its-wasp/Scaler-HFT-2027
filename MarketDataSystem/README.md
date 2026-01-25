# Market Data Publishing System

Low-latency market data publishing system implemented in modern C++17. The system simulates real-time exchange data distribution using two transport mechanisms: TCP loopback and lock-free shared memory.

## Architecture

The system consists of three independent processes:

### Process A - Publisher
- Generates realistic market data (bid/ask prices for RELIANCE)
- Publishes via TCP server (127.0.0.1:8080)
- Writes to lock-free shared memory ring buffer
- Supports multiple TCP clients

### Process B - Shared Memory Consumer
- Reads from shared memory using mmap
- Lock-free SPSC ring buffer with cache-line padding
- Logs messages with nanosecond timestamps
- Measures end-to-end latency

### Process C - TCP Consumer
- Connects to TCP server
- Receives JSON-formatted market data
- Logs messages with nanosecond timestamps
- Measures network latency

## Features

- **Lock-free communication**: SPSC ring buffer with atomic operations
- **Cache-line padding**: 64-byte alignment to prevent false sharing
- **Proper memory ordering**: acquire/release semantics
- **JSON serialization**: Human-readable message format
- **Nanosecond timestamps**: High-resolution timing
- **Latency measurement**: Per-message latency tracking

## Data Format

```json
{
  "instrument": "RELIANCE",
  "bid": 2850.25,
  "ask": 2850.75,
  "timestamp_ns": 1234567890123
}
```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

### Dependencies
- C++17 compiler (g++ or clang++)
- Boost.Asio (for networking)
- fmt library (auto-downloaded if not found)
- pthread (for threading)
- rt (for shared memory on Linux)

## Running

### Terminal 1: Start Publisher
```bash
./publisher
```

### Terminal 2: Start Shared Memory Consumer
```bash
./shm_consumer
```

### Terminal 3: Start TCP Consumer
```bash
./tcp_consumer
```

## Expected Output

**Publisher:**
```
Starting Market Data Publisher...
Creating shared memory...
Starting TCP server on port 8080...
Publisher ready. Generating market data...
Client connected. Total clients: 1
Published 100 messages. Latest: RELIANCE BID=2845.30 ASK=2846.05
```

**Consumers (both SHM and TCP):**
```
[12:34:56.123456789] RELIANCE BID=2850.25 ASK=2850.75 (latency: 1234 ns)
```

## Implementation Details

### Shared Memory Ring Buffer
- Capacity: 1024 messages
- Type: Lock-free SPSC (Single Producer Single Consumer)
- Atomics: `std::atomic<uint32_t>` with proper memory ordering
- Padding: 64-byte alignment on read/write indices

### TCP Server
- Boost.Asio async I/O
- Loopback interface (127.0.0.1)
- Port 8080
- Newline-delimited JSON messages

## Performance Characteristics

- Market data generation: ~10,000 updates/second
- Shared memory latency: < 1 microsecond (typical)
- TCP latency: < 100 microseconds (loopback)

## File Structure

```
MarketDataSystem/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── market_data.h      # Market data structure
│   ├── ring_buffer.h      # Lock-free SPSC ring buffer
│   ├── shm_helper.h       # Shared memory utilities
│   └── utils.h            # JSON, timestamps, formatting
└── src/
    ├── publisher.cpp      # Process A
    ├── shm_consumer.cpp   # Process B
    └── tcp_consumer.cpp   # Process C
```

## Notes

- Run publisher first to create shared memory
- Shared memory is cleaned up on publisher exit
- Press Ctrl+C for graceful shutdown of consumers
- All processes log to stdout using fmt library
