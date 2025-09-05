# Video Streaming Simulation using TCP/UDP and Multithreading

*This project was developed as Assignment 2 of the Computer Networks Lab (CS342).*

## Overview

This project implements a video streaming simulation service that demonstrates the performance differences between TCP and UDP protocols under various network conditions. The system consists of a multithreaded server capable of handling multiple concurrent clients with different scheduling policies, and a client that can stream video data using either TCP or UDP protocols.

## Features

### Server Features
- **Multithreaded Architecture**: Handles multiple clients concurrently
- **Dual Scheduling Policies**:
  - **FCFS (First-Come-First-Serve)**: Serves clients in arrival order
  - **RR (Round-Robin)**: Serves clients cyclically with time quantum allocation
- **Protocol Support**: Handles both TCP and UDP streaming modes
- **Performance Monitoring**: Tracks throughput, latency, and packet loss metrics

### Client Features
- **Dual Protocol Support**: Choose between TCP (reliable) or UDP (real-time) streaming
- **Multiple Resolutions**: Support for 480p, 720p, and 1080p video streaming
- **Performance Analysis**: Measures and reports streaming metrics
- **Bandwidth Efficiency Calculation**: Compares actual vs. expected performance

## Architecture

The communication follows a two-phase protocol:

### Phase 1: Connection Phase (TCP)
1. Client initiates TCP connection with server
2. Client sends Type 1 Request Message specifying resolution and mode
3. Server responds with Type 2 Response Message containing video info and bandwidth requirements
4. TCP connection is closed

### Phase 2: Video Streaming Phase
- **TCP Mode**: Reliable streaming with guaranteed delivery (no packet loss)
- **UDP Mode**: Real-time streaming with simulated packet loss (~5%)

## Video Specifications

| Resolution | Bandwidth | File Size |
|------------|-----------|-----------|
| 480p       | 2 Mbps    | 50 MB     |
| 720p       | 5 Mbps    | 120 MB    |
| 1080p      | 8 Mbps    | 200 MB    |

## Prerequisites

- **Compiler**: g++ with C++11 support
- **Operating System**: Linux/Unix-based system
- **Network**: TCP/UDP socket support

## Installation

1. **Clone the repository** (or extract files):
   ```bash
   git clone <repository-url>
   cd video-streaming-simulation
   ```

2. **Compile the project**:
   ```bash
   make all
   ```
   
   This will create two executables:
   - `video_server`
   - `video_client`

## Usage

### Starting the Server

```bash
./video_server <port> <scheduling_policy>
```

**Parameters:**
- `<port>`: Server port number (1-65535)
- `<scheduling_policy>`: Either `FCFS` or `RR`

**Examples:**
```bash
./video_server 8080 FCFS
./video_server 8080 RR
```

### Running the Client

```bash
./video_client <server_ip> <port> <mode> <resolution>
```

**Parameters:**
- `<server_ip>`: Server IP address
- `<port>`: Server port number
- `<mode>`: Protocol mode (`TCP` or `UDP`)
- `<resolution>`: Video quality (`480p`, `720p`, or `1080p`)

**Examples:**
```bash
./video_client 127.0.0.1 8080 TCP 1080p
./video_client 192.168.1.100 8080 UDP 720p
```

## Testing Scenarios

### Single Client Test
```bash
# Terminal 1 - Start server
./video_server 8080 FCFS

# Terminal 2 - Run client
./video_client 127.0.0.1 8080 TCP 1080p
```

### Multiple Clients Test (FCFS)
```bash
# Terminal 1 - Start FCFS server
./video_server 8080 FCFS

# Terminal 2-4 - Multiple clients
./video_client 127.0.0.1 8080 TCP 1080p
./video_client 127.0.0.1 8080 UDP 720p
./video_client 127.0.0.1 8080 TCP 480p
```

### Round-Robin Test
```bash
# Terminal 1 - Start RR server
./video_server 8080 RR

# Terminal 2-3 - Concurrent clients
./video_client 127.0.0.1 8080 UDP 1080p
./video_client 127.0.0.1 8080 TCP 720p
```

## Performance Metrics

The system measures and reports the following metrics:

### Client-side Metrics
- **Throughput**: Data transmission rate (Mbps)
- **Latency**: Time to first packet reception (ms)
- **Bandwidth Efficiency**: Actual vs. expected bandwidth utilization (%)
- **Packet Loss**: Estimated packet loss for UDP streams (%)

### Server-side Metrics
- **Processing Duration**: Total streaming time per client (ms)
- **Packet Loss Simulation**: 5% packet loss for UDP mode
- **Concurrent Client Handling**: Multiple client support with scheduling

## Scheduling Policies

### FCFS (First-Come-First-Serve)
- Clients are served in arrival order
- Each client gets complete service before the next begins
- Uses 4 worker threads for concurrent processing
- Best for scenarios where completion time matters

### Round-Robin (RR)
- Clients are served cyclically with 1-second time quantum
- Ensures fair resource allocation among clients
- Better for interactive streaming scenarios
- Prevents client starvation

## Build Commands

Available Makefile targets:

```bash
make all           # Build both server and client
make video_server  # Build server only  
make video_client  # Build client only
make clean         # Remove compiled files
make help          # Show help information
```

## Project Structure

```
video-streaming-simulation/
├── video_server.cpp    # Server implementation
├── video_client.cpp    # Client implementation  
├── Makefile           # Build configuration
└── README.md          # This file
```

## Key Implementation Details

- **Message Structure**: Fixed-size message format with type, length, and content fields
- **Socket Management**: Proper socket creation, binding, and cleanup
- **Thread Safety**: Mutex and condition variables for thread synchronization
- **Error Handling**: Comprehensive error checking and graceful failure handling
- **Performance Simulation**: Realistic packet loss and delay simulation

## Troubleshooting

### Common Issues

1. **Port Already in Use**:
   ```bash
   # Check for processes using the port
   netstat -tulpn | grep :8080
   # Kill process if necessary
   kill -9 <process_id>
   ```

2. **Connection Refused**:
   - Ensure server is running before starting clients
   - Check firewall settings
   - Verify correct IP address and port

3. **Compilation Errors**:
   - Ensure g++ supports C++11: `g++ --version`
   - Install build essentials: `sudo apt-get install build-essential`

### Performance Tips

- Use localhost (127.0.0.1) for testing to avoid network latency
- Monitor system resources during high-concurrency tests
- Adjust time quantum in RR scheduling for different workloads

## Authors

Developed for CS342 Computer Networks Lab - Assignment 2

## License

This project is developed for educational purposes as part of the Computer Networks Lab coursework.
