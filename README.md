# Enterprise Distributed Chat Cluster in C++

A robust, enterprise-grade, highly performant **Distributed Chat Server System** implemented in modern C++ (C++17) for Windows using Winsock2 and standard multithreading. It mirrors modern distributed messaging structures like Discord and Slack, routing traffic globally over peer server networks.

## Features
- **Winsock2 TCP Wrappers**: Low-level abstractions with custom packet streaming framing, solving TCP boundary fragmentation.
- **Dynamic Cluster Routing**: Seamless global user lookup tables that map users across multiple nodes.
- **Single-Forward Room Multicast**: Cuts S2S link bandwidth by multicasting exactly once per target server.
- **Heartbeat & Self-Healing**: Liveness daemons that drop silent nodes and launch background auto-reconnection attempts.
- **Aesthetic Terminal Client**: Sleek console client with full ANSI colors, ASCII art title cards, and command routing loops.
- **Modular OOP Architecture**: Header-source decoupled classes honoring SOLID guidelines.

## Quick Start (Building the Project)

### Prerequisites
- Windows 10/11
- C++17 Compatible Compiler (MSVC / Visual Studio Build Tools)
- CMake 3.15+

### Build Steps
From the project root directory:
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release