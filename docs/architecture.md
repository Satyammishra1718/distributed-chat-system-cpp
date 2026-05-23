# Distributed Chat Cluster: System Architecture

This document describes the high-performance system architecture of our C++ Distributed Chat Application. The application is designed to emulate modern messaging protocols (like Discord or Slack), where a cluster of individual servers routes messages to users connected to different hosts.

## Core Topology

```mermaid
graph TD
    ClientA["Client Alice"] <-->|"Winsock TCP"| Server1["Server Node 1"]
    ClientB["Client Bob"] <-->|"Winsock TCP"| Server2["Server Node 2"]
    
    Server1 <-->|"Bi-directional Peer TCP Link"| Server2

# Internal Component Layout
Each server node is fully self-contained and coordinates its operations through specialized decoupled layers:

1. Winsock Abstraction Layer:

- ISocket: Interface defining socket semantics.
- TcpSocket: Concrete Winsock2 TCP implementation that manages handles and handles buffering.
- SocketUtils: Utility functions to configure TCP options (like SO_KEEPALIVE) and parse OS-level socket error strings.

2. Concurrency Layer:

- ThreadPool: Coordinates standard worker threads to execute message parsing and routing tasks without blocking the main event  loops.

3. Coordination Layer:

- UserManager: Maps globally connected usernames to their active hosting Server IDs.
- ConnectionManager: Tracks active client sessions and server-to-server peer nodes.
- ChatRoomManager: Tracks room subscriptions and maps memberships by local users and remote servers.

4. Distributed Protocol Layer:

- ReplicationManager: Synchronizes cluster-wide states (user logins/logouts, room joins) and coordinates inter-server message routing.
- HeartbeatManager: Runs an active background monitor thread sending heartbeat checks, executing peer death callbacks, and cleaning up state tables on connection dropouts.


# Concurrency Model

All shared state managers are thread-safe and serialize access using standard C++ synchronization primitives:

- std::mutex is used for resource serialization.
- std::unique_lock is used for conditional variables waiting (in ThreadPool).
- Deadlock-free programming is achieved by strict lock hierarchy and minimal lock holding scopes.