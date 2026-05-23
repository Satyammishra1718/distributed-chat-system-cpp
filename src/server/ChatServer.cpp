#include "../../include/server/ChatServer.h"
#include "../../include/protocol/MessageSerializer.h"
#include "../../include/core/logging/Logger.h"

namespace DistributedChat {
    ChatServer::ChatServer(int serverId, int port, size_t poolThreads)
        : m_serverId(serverId), m_port(port), m_running(false),
          m_userManager(serverId), m_replManager(*this), m_hbManager(*this),
          m_threadPool(poolThreads) {
        m_listenerSocket = std::make_shared<TcpSocket>();
    }

    ChatServer::~ChatServer() {
        Stop();
    }

    void ChatServer::AddPeerConfig(int peerId, const std::string& host, int port) {
        m_peerConfigs.push_back({peerId, {host, port}});
        // Create offline peer representation in ConnectionManager
        auto peerNode = std::make_shared<ServerNode>(peerId, host, port);
        m_connManager.AddPeerNode(peerNode);
    }

    bool ChatServer::Start() {
        Logger::Info("Starting Chat Server Node " + std::to_string(m_serverId) + " on port " + std::to_string(m_port), "SERVER");

        if (!m_listenerSocket->Bind(m_port)) {
            Logger::Error("Failed to bind listener socket to port " + std::to_string(m_port) + ": " + TcpSocket::GetLastErrorString(), "SERVER");
            return false;
        }

        if (!m_listenerSocket->Listen()) {
            Logger::Error("Failed to listen on socket: " + TcpSocket::GetLastErrorString(), "SERVER");
            return false;
        }

        m_running = true;

        m_hbManager.Start();

        m_listenerThread = std::thread(&ChatServer::ListenerLoop, this);
        m_peerConnectorThread = std::thread(&ChatServer::PeerConnectorLoop, this);

        Logger::Success("Chat Server Node " + std::to_string(m_serverId) + " is fully operational", "SERVER");
        return true;
    }

    void ChatServer::Stop() {
        if (!m_running) return;
        m_running = false;

        Logger::Info("Shutting down Chat Server Node " + std::to_string(m_serverId), "SERVER");

        m_hbManager.Stop();

        if (m_listenerSocket) {
            m_listenerSocket->Close();
        }

        // Close all client sessions
        auto sessions = m_connManager.GetAllSessions();
        for (const auto& session : sessions) {
            if (session->GetSocket()) {
                session->GetSocket()->Close();
            }
        }

        // Close peer node connections
        auto peers = m_connManager.GetAllPeerNodes();
        for (const auto& peer : peers) {
            if (peer->GetSocket()) {
                peer->GetSocket()->Close();
            }
        }

        if (m_listenerThread.joinable()) {
            m_listenerThread.join();
        }
        if (m_peerConnectorThread.joinable()) {
            m_peerConnectorThread.join();
        }

        Logger::Success("Chat Server Node " + std::to_string(m_serverId) + " shut down successfully", "SERVER");
    }

    void ChatServer::HandlePeerDisconnect(int peerServerId) {
        auto peer = m_connManager.GetPeerNode(peerServerId);
        if (peer && peer->IsConnected()) {
            Logger::Warn("Peer Server ID " + std::to_string(peerServerId) + " disconnected", "SERVER");
            
            if (peer->GetSocket()) {
                peer->GetSocket()->Close();
            }
            m_connManager.UnregisterPeerSocket(peerServerId);
            
            // Clean up states
            m_userManager.ClearServerUsers(peerServerId);
            m_roomManager.ClearServerMemberships(peerServerId);
            
            // Re-notify online local users
            auto localSessions = m_connManager.GetAllSessions();
            std::string onlineList = MessageSerializer::Pack(m_userManager.GetOnlineUsers());
            for (const auto& local : localSessions) {
                if (local->IsAuthenticated()) {
                    local->SendPacket(PacketType::STATUS_UPDATE, MessageSerializer::Pack({
                "STATUS",
                onlineList
            }));
                }
            }
        }
    }

    void ChatServer::ListenerLoop() {
        Logger::Info("Listener select-loop thread running", "SERVER");
        
        while (m_running) {
            fd_set readfds;
            FD_ZERO(&readfds);

            SOCKET maxFd = m_listenerSocket->GetHandle();
            FD_SET(maxFd, &readfds);

            auto sessions = m_connManager.GetAllSessions();
            for (const auto& session : sessions) {
                SOCKET s = session->GetSocket()->GetHandle();
                FD_SET(s, &readfds);
                if (s > maxFd) maxFd = s;
            }

            auto peers = m_connManager.GetAllPeerNodes();
            for (const auto& peer : peers) {
                if (peer->IsConnected() && peer->GetSocket()) {
                    SOCKET s = peer->GetSocket()->GetHandle();
                    FD_SET(s, &readfds);
                    if (s > maxFd) maxFd = s;
                }
            }

            timeval timeout = {0, 200000}; // 200ms select timeout
            int activity = select(static_cast<int>(maxFd + 1), &readfds, nullptr, nullptr, &timeout);

            if (activity == SOCKET_ERROR) {
                if (m_running) {
                    Logger::Error("select() encountered error: " + TcpSocket::GetLastErrorString(), "SERVER");
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                continue;
            }

            if (activity == 0) continue; // select timed out with no activity

            // 1. Check for incoming connections on Listener Socket
            if (FD_ISSET(m_listenerSocket->GetHandle(), &readfds)) {
                auto clientSocket = m_listenerSocket->Accept();
                if (clientSocket) {
                    clientSocket->SetNonBlocking(true);
                    auto newSession = std::make_shared<ClientSession>(clientSocket);
                    m_connManager.AddSession(newSession);
                    Logger::Info("Accepted new raw connection from client handle: " + std::to_string(clientSocket->GetHandle()), "SERVER");
                }
            }

            // 2. Check client sessions for incoming data
            for (const auto& session : sessions) {
                SOCKET s = session->GetSocket()->GetHandle();
                if (FD_ISSET(s, &readfds)) {
                    RawPacket packet;
                    if (MessageSerializer::ReadPacket(*(session->GetSocket()), packet)) {
                        m_threadPool.Enqueue([this, session, packet]() {
                            this->HandleClientPacket(session, packet);
                        });
                    } else {
                        Logger::Info("Client disconnected or invalid packet. Handle: " + std::to_string(s), "SERVER");
                        std::string username = session->GetUsername();
                        if (!username.empty()) {
                            m_userManager.UnregisterUser(username);
                            m_replManager.ReplicateUserLogout(username);
                            
                            // Re-notify online users
                            auto remaining = m_connManager.GetAllSessions();
                            std::string onlineList = MessageSerializer::Pack(m_userManager.GetOnlineUsers());
                            for (const auto& rSession : remaining) {
                                if (rSession->IsAuthenticated() && rSession->GetSocket()->GetHandle() != s) {
                                    rSession->SendPacket(PacketType::STATUS_UPDATE, MessageSerializer::Pack({
                "STATUS",
                onlineList
            }));
                                }
                            }
                        }
                        m_connManager.RemoveSession(s);
                    }
                }
            }

            // 3. Check peer server sockets for incoming data
            for (const auto& peer : peers) {
                if (peer->IsConnected() && peer->GetSocket()) {
                    SOCKET s = peer->GetSocket()->GetHandle();
                    if (FD_ISSET(s, &readfds)) {
                        RawPacket packet;
                        if (MessageSerializer::ReadPacket(*(peer->GetSocket()), packet)) {
                            m_threadPool.Enqueue([this, peer, packet]() {
                                this->HandleServerPacket(peer, packet);
                            });
                        } else {
                            HandlePeerDisconnect(peer->GetServerId());
                        }
                    }
                }
            }
        }

        Logger::Info("Listener select-loop thread stopped", "SERVER");
    }

    void ChatServer::PeerConnectorLoop() {
        Logger::Info("Peer auto-connector thread running", "SERVER");
        
        while (m_running) {
            for (const auto& config : m_peerConfigs) {
                int peerId = config.first;
                std::string host = config.second.first;
                int port = config.second.second;

                // Attempt link ONLY if the peerId is smaller than ours to avoid dual active S2S connects,
                // OR if we are not connected and no socket is bound.
                auto peer = m_connManager.GetPeerNode(peerId);
                if (peer && !peer->IsConnected()) {
                    Logger::Info("Attempting to connect to peer Server ID " + std::to_string(peerId) + " at " + host + ":" + std::to_string(port), "SERVER");
                    
                    auto sock = std::make_shared<TcpSocket>();
                    if (sock->Connect(host, port)) {
                        sock->SetNonBlocking(true);
                        m_connManager.RegisterPeerSocket(peerId, sock);
                        peer->SetConnected(true);
                        peer->UpdateHeartbeat();

                        // Send handshake introduction
                        std::string payload = std::to_string(m_serverId);
                        std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::S2S_CONNECT, payload);
                        sock->SendAll(buffer.data(), static_cast<int>(buffer.size()));

                        Logger::Success("Successfully established connection to peer Server ID: " + std::to_string(peerId), "SERVER");
                    } else {
                        Logger::Debug("Could not connect to peer Server ID " + std::to_string(peerId) + " (server offline)", "SERVER");
                    }
                }
            }

            // Sleep for 10 seconds between reconnection cycles
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }

        Logger::Info("Peer auto-connector thread stopped", "SERVER");
    }

    void ChatServer::HandleClientPacket(const std::shared_ptr<ClientSession>& session, const RawPacket& packet) {
        if (!session->IsAuthenticated() && packet.type != PacketType::LOGIN && packet.type != PacketType::S2S_CONNECT) {
            session->SendPacket(
            PacketType::LOGIN_RESP,
            MessageSerializer::Pack({
            "0",
            "Authentication required"
    })
);
            return;
        }

        switch (packet.type) {
            case PacketType::LOGIN:
                HandleLogin(session, packet.payload);
                break;
            case PacketType::MSG_PRIVATE:
                HandlePrivateMsg(session, packet.payload);
                break;
            case PacketType::MSG_ROOM:
                HandleRoomMsg(session, packet.payload);
                break;
            case PacketType::MSG_BROADCAST:
                HandleBroadcastMsg(session, packet.payload);
                break;
            case PacketType::ROOM_JOIN:
                HandleRoomJoin(session, packet.payload);
                break;
            case PacketType::ROOM_LEAVE:
                HandleRoomLeave(session, packet.payload);
                break;
            case PacketType::ROOM_CREATE:
                HandleRoomCreate(session, packet.payload);
                break;
            case PacketType::S2S_CONNECT: {
                // Incoming socket from peer connector. Promote this session to a peer connection.
                int peerId = std::stoi(packet.payload);
                Logger::Info("Promoting incoming raw connection from socket " + 
                             std::to_string(session->GetSocket()->GetHandle()) + " to Peer Server ID: " + std::to_string(peerId), "SERVER");
                
                auto socket = session->GetSocket();
                m_connManager.RemoveSession(socket->GetHandle());
                m_connManager.RegisterPeerSocket(peerId, socket);
                
                auto peerNode = m_connManager.GetPeerNode(peerId);
                if (peerNode) {
                    peerNode->SetConnected(true);
                    peerNode->UpdateHeartbeat();
                }
                break;
            }
            default:
                Logger::Warn("Received unknown packet type from client session: " + std::to_string(static_cast<int>(packet.type)), "SERVER");
                break;
        }
    }

    void ChatServer::HandleServerPacket(const std::shared_ptr<ServerNode>& node, const RawPacket& packet) {
        node->UpdateHeartbeat(); // Refresh ping timestamp

        switch (packet.type) {
            case PacketType::S2S_HEARTBEAT:
                HandleS2SHeartbeat(node, packet.payload);
                break;
            case PacketType::S2S_HEARTBEAT_ACK:
                HandleS2SHeartbeatAck(node, packet.payload);
                break;
            case PacketType::S2S_REPLICATE_USER:
                HandleS2SReplicateUser(node, packet.payload);
                break;
            case PacketType::S2S_ROUTE_MSG:
                HandleS2SRouteMsg(node, packet.payload);
                break;
            case PacketType::S2S_ROUTE_ROOM_MSG:
                HandleS2SRouteRoomMsg(node, packet.payload);
                break;
            case PacketType::S2S_ROOM_MEMBER_SYNC:
                HandleS2SRoomMemberSync(node, packet.payload);
                break;
            default:
                Logger::Warn("Received unknown packet type from Peer Server ID " + std::to_string(node->GetServerId()) + ": " + std::to_string(static_cast<int>(packet.type)), "SERVER");
                break;
        }
    }

    // CLIENT PACKETS IMPLEMENTATION

    void ChatServer::HandleLogin(const std::shared_ptr<ClientSession>& session, const std::string& payload) {
    std::string username = Trim(payload);

    if (username.empty() ||
        username.find('\0') != std::string::npos ||
        username.find(' ') != std::string::npos) {

        session->SendPacket(
            PacketType::LOGIN_RESP,
            MessageSerializer::Pack({
                "0",
                "Invalid username"
            })
        );
        return;
    }

    if (m_userManager.IsUserOnline(username)) {
        session->SendPacket(
            PacketType::LOGIN_RESP,
            MessageSerializer::Pack({
                "0",
                "Username already online"
            })
        );
        return;
    }

    Logger::Success(
        "User " + username +
        " successfully authenticated on Server " +
        std::to_string(m_serverId),
        "SERVER"
    );

    m_userManager.RegisterUser(username, m_serverId);
    m_connManager.AuthenticateSession(session, username);

    session->SendPacket(
        PacketType::LOGIN_RESP,
        MessageSerializer::Pack({
            "1",
            "Welcome to Distributed Chat!"
        })
    );

    m_replManager.ReplicateUserLogin(username);

    auto sessions = m_connManager.GetAllSessions();

    std::string onlineList =
        MessageSerializer::Pack(
            m_userManager.GetOnlineUsers()
        );

    for (const auto& s : sessions) {
        if (s->IsAuthenticated()) {

            s->SendPacket(
                PacketType::STATUS_UPDATE,
                MessageSerializer::Pack({
                    "STATUS",
                    onlineList
                })
            );
        }
    }
}

   void ChatServer::HandlePrivateMsg(
    const std::shared_ptr<ClientSession>& session,
    const std::string& payload) {

    std::vector<std::string> fields =
        MessageSerializer::Unpack(payload);

    if (fields.size() < 2)
        return;

    std::string receiver = fields[0];
    std::string text = fields[1];
    std::string sender = session->GetUsername();

    int location =
        m_userManager.GetUserLocation(receiver);

    if (location == m_serverId) {

        auto targetSession =
            m_connManager.GetSessionByUsername(receiver);

        if (targetSession) {

            targetSession->SendPacket(
                PacketType::MSG_PRIVATE,
                MessageSerializer::Pack({
                    sender,
                    text
                })
            );

            session->SendPacket(
                PacketType::STATUS_UPDATE,
                MessageSerializer::Pack({
                    "SUCCESS",
                    "Message delivered to " + receiver
                })
            );
        }

    } else if (location > 0) {

        m_replManager.RoutePrivateMessage(
            sender,
            receiver,
            text,
            location
        );

        session->SendPacket(
            PacketType::STATUS_UPDATE,
            MessageSerializer::Pack({
                "SUCCESS",
                "Message routed to remote server"
            })
        );

    } else {

        session->SendPacket(
            PacketType::STATUS_UPDATE,
            MessageSerializer::Pack({
                "ERROR",
                "User offline"
            })
        );
    }
}

   void ChatServer::HandleRoomMsg(
    const std::shared_ptr<ClientSession>& session,
    const std::string& payload) {

    std::vector<std::string> fields =
        MessageSerializer::Unpack(payload);

    if (fields.size() < 2)
        return;

    std::string room = fields[0];
    std::string text = fields[1];
    std::string sender = session->GetUsername();

    if (!session->IsInRoom(room)) {

        session->SendPacket(
            PacketType::STATUS_UPDATE,
            MessageSerializer::Pack({
                "ERROR",
                "Join room first"
            })
        );

        return;
    }

    auto locals =
        m_roomManager.GetLocalMembers(room);

    for (const auto& local : locals) {

        auto s =
            m_connManager.GetSessionByUsername(local);

        if (s) {

            s->SendPacket(
                PacketType::MSG_ROOM,
                MessageSerializer::Pack({
                    sender,
                    room,
                    text
                })
            );
        }
    }

    m_replManager.RouteRoomMessage(
        sender,
        room,
        text
    );
}

  void ChatServer::HandleBroadcastMsg(
    const std::shared_ptr<ClientSession>& session,
    const std::string& payload) {

    std::string sender =
        session->GetUsername();

    auto locals =
        m_connManager.GetAllSessions();

    for (const auto& local : locals) {

        if (local->IsAuthenticated()) {

            local->SendPacket(
                PacketType::MSG_BROADCAST,
                MessageSerializer::Pack({
                    sender,
                    payload
                })
            );
        }
    }

    std::string s2sPayload =
        MessageSerializer::Pack({
            sender,
            payload
        });

    m_replManager.BroadcastS2S(
        PacketType::S2S_ROUTE_MSG,
        s2sPayload
    );
}

    void ChatServer::HandleRoomJoin(const std::shared_ptr<ClientSession>& session, const std::string& payload) {
        std::string room = Trim(payload);
        std::string username = session->GetUsername();

        if (room.empty()) return;

        session->JoinRoom(room);
        
        bool isFirstLocalInRoom = m_roomManager.GetLocalMembers(room).empty();
        m_roomManager.JoinRoomLocal(room, username);

        session->SendPacket(
    PacketType::STATUS_UPDATE,
    MessageSerializer::Pack({
        "SUCCESS",
        "Joined room " + room
    })
);

        // Notify room members
        auto locals = m_roomManager.GetLocalMembers(room);
        for (const auto& name : locals) {
            auto s = m_connManager.GetSessionByUsername(name);
            if (s) {
                s->SendPacket(PacketType::STATUS_UPDATE, MessageSerializer::Pack({
    "INFO",
    "User " + username + " entered " + room
}));
            }
        }

        // If this is the first local member joining the room, we notify peers that this server holds members for room R
        if (isFirstLocalInRoom) {
            m_replManager.ReplicateRoomJoin(room);
        }
    }

    void ChatServer::HandleRoomLeave(const std::shared_ptr<ClientSession>& session, const std::string& payload) {
        std::string room = Trim(payload);
        std::string username = session->GetUsername();

        if (room.empty() || !session->IsInRoom(room)) return;

        session->LeaveRoom(room);
        m_roomManager.LeaveRoomLocal(room, username);

        session->SendPacket(PacketType::STATUS_UPDATE, MessageSerializer::Pack({
    "SUCCESS",
    "Left room " + room
}));

        // Notify remaining members
        auto locals = m_roomManager.GetLocalMembers(room);
        for (const auto& name : locals) {
            auto s = m_connManager.GetSessionByUsername(name);
            if (s) {
                s->SendPacket(PacketType::STATUS_UPDATE, MessageSerializer::Pack({
    "INFO",
    "User " + username + " left " + room
}));
            }
        }

        // If no more local members are in this room, notify peers that this server no longer tracks room R
        if (locals.empty()) {
            m_replManager.ReplicateRoomLeave(room);
        }
    }

    void ChatServer::HandleRoomCreate(
    const std::shared_ptr<ClientSession>& session,
    const std::string& payload) {

    std::string room = Trim(payload);

    if (room.empty())
        return;

    session->SendPacket(
        PacketType::STATUS_UPDATE,
        MessageSerializer::Pack({
            "SUCCESS",
            "Room " + room +
            " created. Type /join " +
            room +
            " to enter."
        })
    );
}

    // SERVER-TO-SERVER PACKETS IMPLEMENTATION

    void ChatServer::HandleS2SHeartbeat(const std::shared_ptr<ServerNode>& node, const std::string& payload) {
        // Heartbeat request, send back ACK
        std::string ackPayload = std::to_string(m_serverId);
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::S2S_HEARTBEAT_ACK, ackPayload);
        
        if (node->GetSocket()) {
            node->GetSocket()->SendAll(buffer.data(), static_cast<int>(buffer.size()));
        }
    }

    void ChatServer::HandleS2SHeartbeatAck(const std::shared_ptr<ServerNode>& node, const std::string& payload) {
        Logger::Debug("Heartbeat ACK received from peer Server ID: " + std::to_string(node->GetServerId()), "SERVER");
    }

    void ChatServer::HandleS2SReplicateUser(const std::shared_ptr<ServerNode>& node, const std::string& payload) {
        std::vector<std::string> fields = MessageSerializer::Unpack(payload);
        if (fields.size() < 3) return;

        std::string username = fields[0];
        int locationId = std::stoi(fields[1]);
        bool online = (fields[2] == "1");

        if (online) {
            m_userManager.RegisterUser(username, locationId);
            Logger::Info("Peer replicated User " + username + " online on Server ID: " + std::to_string(locationId), "SERVER");
        } else {
            m_userManager.UnregisterUser(username);
            Logger::Info("Peer replicated User " + username + " offline (logout)", "SERVER");
        }

        // Update local clients lists
        auto sessions = m_connManager.GetAllSessions();
        std::string onlineList = MessageSerializer::Pack(m_userManager.GetOnlineUsers());
        for (const auto& s : sessions) {
            if (s->IsAuthenticated()) {
                s->SendPacket(PacketType::STATUS_UPDATE, MessageSerializer::Pack({
                "STATUS",
                onlineList
            })
            );
            }
        }
    }

    void ChatServer::HandleS2SRouteMsg(
    const std::shared_ptr<ServerNode>& node,
    const std::string& payload) {

    std::vector<std::string> fields =
        MessageSerializer::Unpack(payload);

    // PRIVATE MESSAGE
    if (fields.size() == 3) {

        std::string sender = fields[0];
        std::string receiver = fields[1];
        std::string text = fields[2];

        auto session =
            m_connManager.GetSessionByUsername(receiver);

        if (session) {

            session->SendPacket(
                PacketType::MSG_PRIVATE,
                MessageSerializer::Pack({
                    sender,
                    text
                })
            );
        }
    }

    // GLOBAL BROADCAST
    else if (fields.size() == 2) {

        std::string sender = fields[0];
        std::string text = fields[1];

        auto sessions =
            m_connManager.GetAllSessions();

        for (const auto& session : sessions) {

            if (session->IsAuthenticated()) {

                session->SendPacket(
                    PacketType::MSG_BROADCAST,
                    MessageSerializer::Pack({
                        sender,
                        text
                    })
                );
            }
        }
    }
}

    void ChatServer::HandleS2SRouteRoomMsg(
    const std::shared_ptr<ServerNode>& node,
    const std::string& payload) {

    std::vector<std::string> fields =
        MessageSerializer::Unpack(payload);

    if (fields.size() < 3)
        return;

    std::string sender = fields[0];
    std::string room = fields[1];
    std::string text = fields[2];

    auto locals =
        m_roomManager.GetLocalMembers(room);

    for (const auto& local : locals) {

        auto session =
            m_connManager.GetSessionByUsername(local);

        if (session) {

            session->SendPacket(
                PacketType::MSG_ROOM,
                MessageSerializer::Pack({
                    sender,
                    room,
                    text
                })
            );
        }
    }
}

    void ChatServer::HandleS2SRoomMemberSync(const std::shared_ptr<ServerNode>& node, const std::string& payload) {
        std::vector<std::string> fields = MessageSerializer::Unpack(payload);
        if (fields.size() < 3) return;

        std::string room = fields[0];
        int peerServerId = std::stoi(fields[1]);
        bool active = (fields[2] == "1");

        if (active) {
            m_roomManager.JoinRoomRemote(room, peerServerId);
            Logger::Info("Peer replicated that Server ID: " + std::to_string(peerServerId) + " has members in room " + room, "SERVER");
        } else {
            m_roomManager.LeaveRoomRemote(room, peerServerId);
            Logger::Info("Peer replicated that Server ID: " + std::to_string(peerServerId) + " no longer has members in room " + room, "SERVER");
        }
    }
}