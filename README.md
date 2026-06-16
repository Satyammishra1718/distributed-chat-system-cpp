# Enterprise Distributed Chat Cluster in C++

A robust, enterprise-grade, highly performant **Distributed Chat Server System** implemented in modern C++ (C++17) for Windows using Winsock2 and standard multithreading. It mirrors modern distributed messaging structures like Discord and Slack, routing traffic globally over peer server networks.

## Features
- **Distributed Multi-Server Architecture**: Multiple interconnected server nodes form a cluster, allowing users to connect to any server while communicating seamlessly across the network.
- **Private Messaging, Chat Rooms & Global Broadcasts**: Supports one-to-one messaging, group conversations through chat rooms, and cluster-wide announcements.
- **Cross-Server User Discovery & Message Routing**: Maintains distributed routing information to locate users across different server nodes and deliver messages transparently.
- **Fault Tolerance with Heartbeat Monitoring**: Periodic heartbeat checks detect node failures, enabling the cluster to identify unavailable servers and maintain system stability.
- **Automatic Peer Reconnection**: Server-to-server connections are automatically re-established after network interruptions, improving reliability and availability.
- **High-Concurrency Processing**: Uses multithreading, thread pools, and custom TCP packet framing to efficiently handle multiple clients and concurrent message traffic.

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
