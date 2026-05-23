# Distributed Chat System: Wire Framing & Protocol Specification

To prevent stream fragmentation and establish deterministic frame boundaries over raw TCP sockets, our application implements a custom network protocol.

## Wire Frame Layout

Each message sent over the TCP channel (both Client-to-Server and Server-to-Server) uses the following structured block layout:

+------------------------------------+--------------------------+------------------------------------+ | Payload Length (4 bytes, Big-E) | Packet Type (1 byte, raw) | Payload Content (N bytes) | +------------------------------------+--------------------------+------------------------------------+


### Protocol Fields
1. **Payload Length (uint32_t)**: Represents the size of the rest of the frame (1 byte for Packet Type + length of Payload). Transferred in network byte order (Big Endian). Enforces size safety checks (< 1MB) to protect against buffer overflow exploits.
2. **Packet Type (uint8_t)**: Single-byte integer indicating the packet type matching the `PacketType` enumeration.
3. **Payload Content**: Sequence of characters. For multiple arguments, strings are serialized as a single block separated by null characters (`\0`), making parsing extremely robust.

## Packet Types Reference
- **LOGIN = 1**: Client introducing username.
- **LOGIN_RESP = 2**: Success/Fail flag and welcome text.
- **MSG_PRIVATE = 3**: Private chat message: `receiver\0text\0`.
- **MSG_ROOM = 4**: Room message: `room\0text\0`.
- **MSG_BROADCAST = 5**: Global broadcast string.
- **ROOM_JOIN = 6**: Join a room.
- **ROOM_LEAVE = 7**: Leave a room.
- **ROOM_CREATE = 8**: Create a room.
- **STATUS_UPDATE = 9**: Delimited presence statuses.
- **S2S_CONNECT = 100**: Handshake introduction: `senderServerId`.
- **S2S_HEARTBEAT = 101**: Heartbeat ping.
- **S2S_HEARTBEAT_ACK = 102**: Heartbeat ACK.
- **S2S_REPLICATE_USER = 103**: User state sync: `username\0serverId\0loginStatus\0`.
- **S2S_ROUTE_MSG = 104**: Private message inter-node routing.
- **S2S_ROUTE_ROOM_MSG = 105**: Room multicast inter-node routing.
- **S2S_ROOM_MEMBER_SYNC = 106**: Remote room subscription tracking.