
#### `docs/distributed-routing.md`
```markdown
# Distributed Chat Cluster: Routing & Synchronization Mechanics

This document outlines the distributed coordination and message-routing protocols that bind the server grid together.

## 1. Presence Sync & Global Routing Tables

Every server in the cluster maintains a **UserManager** routing registry mapping `username -> ServerId`.
- **Login**: When Alice authenticates on Server 1, Server 1 registers her in its local table and broadcasts `S2S_REPLICATE_USER` containing Alice's name and Server ID 1. All peer nodes insert Alice into their routing tables.
- **Logout / Disconnect**: On logout or socket dropout, Server 1 deletes her locally and broadcasts `S2S_REPLICATE_USER` with logout flags. Peers remove Alice from their maps.

## 2. Private Message Routing

Alice (on S1) ---> Server 1 --[S2S_ROUTE_MSG]--> Server 2 ---> Bob (on S2)


1. Alice sends `MSG_PRIVATE` to Bob.
2. Server 1 queries `UserManager::GetUserLocation("Bob")`:
   - If Bob is local, Server 1 writes directly to Bob's socket.
   - If Bob's location is Server 2, Server 1 wraps the packet as `S2S_ROUTE_MSG` and sends it to Server 2.
   - Server 2 receives `S2S_ROUTE_MSG`, extracts the original message, and delivers it to Bob.
   - If Bob is offline, Server 1 sends a fail notification back to Alice.

## 3. Distributed Chat Room Multicast (Single-Forward Optimization)

If a chat room #lobby has members across multiple servers, sending a separate inter-server packet for each remote user would cause excessive network overhead.
Our cluster utilizes the **Single-Forward Multicast Optimization**:
- **Room Sync**: When Server 1 gains its *first* local subscriber in Room R, it broadcasts `S2S_ROOM_MEMBER_SYNC` to notify peers.
- **Multicast Routing**: When Alice sends a message to `#lobby`:
  1. Server 1 delivers it locally to all local members in `#lobby`.
  2. Server 1 queries `ChatRoomManager::GetRemoteServers("#lobby")` to fetch the set of server IDs containing active members.
  3. Server 1 sends **exactly one** copy of the room message (`S2S_ROUTE_ROOM_MSG`) to each remote server node in the set.
  4. Remote servers receive `S2S_ROUTE_ROOM_MSG` and distribute it locally to their subscribers.
- **Room Cleanups**: When the last local subscriber on Server 1 leaves Room R, Server 1 sends `S2S_ROOM_MEMBER_SYNC` with leave flags. Peer servers remove Server 1 from their routing tables for Room R.

## 4. Failover & Reconnection Loops
- If a server node drops offline, peer heartbeat managers detect the missing ACKs.
- They immediately call `HandlePeerDisconnect()`, which clears all user locations and room memberships associated with that server ID.
- Reconnection threads start a 10-second background attempt loop to re-establish the S2S peer link.