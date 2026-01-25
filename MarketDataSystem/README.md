# Market Data System

Low-latency market data publishing system with TCP and Shared Memory transports.

## Architecture

- **Publisher (Process A)**: Generates simulated market data, publishes via TCP and shared memory
- **SHM Consumer (Process B)**: Reads from shared memory ring buffer
- **TCP Consumer (Process C)**: Reads from TCP loopback socket

## Features

- Lock-free SPSC ring buffer with cache-line padding (`alignas(64)`)
- Shared memory using `mmap`/`shm_open`
- TCP with `TCP_NODELAY` (Nagle's algorithm disabled)
- CPU affinity support
- Nanosecond timestamp logging

## Build

```bash
cd MarketDataSystem
mkdir build && cd build
cmake ..
make
```

## Run

```bash
# Terminal 1: Start publisher
./publisher

# Terminal 2: Start SHM consumer
./shm_consumer              # Sleep mode
./shm_consumer --busy-wait  # Ultra-low latency mode

# Terminal 3: Start TCP consumer
./tcp_consumer
```

## Performance

| Transport | Latency (P50) |
|-----------|---------------|
| SHM (busy-wait) | ~1-3 μs |
| SHM (sleep) | ~30-40 μs |
| TCP loopback | ~16-20 μs |

Throughput: 10,000 messages/second
