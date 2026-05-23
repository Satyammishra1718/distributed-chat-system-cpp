#include "../../include/server/ReplicationManager.h"
#include "../../include/server/ChatServer.h"
#include "../../include/protocol/MessageSerializer.h"
#include "../../include/core/logging/Logger.h"

namespace DistributedChat {
    ReplicationManager::ReplicationManager(ChatServer& server) : m_server(server) {}

    void ReplicationManager::BroadcastS2S(PacketType type, const std::string& payload) {
        auto peers = m_server.GetConnectionManager().GetAllPeerNodes();
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(type, payload);
        
        for (const auto& peer : peers) {
            if (peer->IsConnected() && peer->GetSocket()) {
                const char* ptr = reinterpret_cast<const char*>(buffer.data());
                int bytesLeft = static_cast<int>(buffer.size());
                bool sendFailed = false;
                
                while (bytesLeft > 0) {
                    int sent = peer->GetSocket()->Send(ptr, bytesLeft);
                    if (sent <= 0) {
                        if (sent == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(5));
                            continue;
                        }
                        sendFailed = true;
                        break;
                    }
                    ptr += sent;
                    bytesLeft -= sent;
                }
                
                if (sendFailed) {
                    Logger::Error("Failed to send replication packet of type " + 
                                  std::to_string(static_cast<int>(type)) + " to peer server ID: " + 
                                  std::to_string(peer->GetServerId()), "REPLICATION");
                    m_server.HandlePeerDisconnect(peer->GetServerId());
                }
            }
        }
    }

    void ReplicationManager::ReplicateUserLogin(const std::string& username) {
        std::string payload = MessageSerializer::Pack({
            username,
            std::to_string(m_server.GetServerId()),
            "1" // Login
        });
        Logger::Info("Replicating login of " + username + " to peer cluster", "REPLICATION");
        BroadcastS2S(PacketType::S2S_REPLICATE_USER, payload);
    }

    void ReplicationManager::ReplicateUserLogout(const std::string& username) {
        std::string payload = MessageSerializer::Pack({
            username,
            std::to_string(m_server.GetServerId()),
            "0" // Logout
        });
        Logger::Info("Replicating logout of " + username + " to peer cluster", "REPLICATION");
        BroadcastS2S(PacketType::S2S_REPLICATE_USER, payload);
    }

    void ReplicationManager::ReplicateRoomJoin(const std::string& room) {
        std::string payload = MessageSerializer::Pack({
            room,
            std::to_string(m_server.GetServerId()),
            "1" // Join
        });
        Logger::Info("Replicating room " + room + " join of Server " + std::to_string(m_server.GetServerId()) + " to peers", "REPLICATION");
        BroadcastS2S(PacketType::S2S_ROOM_MEMBER_SYNC, payload);
    }

    void ReplicationManager::ReplicateRoomLeave(const std::string& room) {
        std::string payload = MessageSerializer::Pack({
            room,
            std::to_string(m_server.GetServerId()),
            "0" // Leave
        });
        Logger::Info("Replicating room " + room + " leave of Server " + std::to_string(m_server.GetServerId()) + " to peers", "REPLICATION");
        BroadcastS2S(PacketType::S2S_ROOM_MEMBER_SYNC, payload);
    }

    void ReplicationManager::RoutePrivateMessage(const std::string& sender, const std::string& receiver, const std::string& text, int targetServerId) {
        auto peer = m_server.GetConnectionManager().GetPeerNode(targetServerId);
        if (!peer || !peer->IsConnected() || !peer->GetSocket()) {
            Logger::Error("Cannot route private message to offline Server ID: " + std::to_string(targetServerId), "REPLICATION");
            return;
        }

        std::string payload = MessageSerializer::Pack({ sender, receiver, text });
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::S2S_ROUTE_MSG, payload);

        Logger::Info("Routing private message from " + sender + " to remote receiver " + receiver + " on Server ID: " + std::to_string(targetServerId), "REPLICATION");

        const char* ptr = reinterpret_cast<const char*>(buffer.data());
        int bytesLeft = static_cast<int>(buffer.size());
        bool sendFailed = false;

        while (bytesLeft > 0) {
            int sent = peer->GetSocket()->Send(ptr, bytesLeft);
            if (sent <= 0) {
                if (sent == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    continue;
                }
                sendFailed = true;
                break;
            }
            ptr += sent;
            bytesLeft -= sent;
        }

        if (sendFailed) {
            Logger::Error("Failed to route message to peer Server ID: " + std::to_string(targetServerId), "REPLICATION");
            m_server.HandlePeerDisconnect(targetServerId);
        }
    }

    void ReplicationManager::RouteRoomMessage(const std::string& sender, const std::string& room, const std::string& text) {
        std::vector<int> targetServers = m_server.GetChatRoomManager().GetRemoteServers(room);
        if (targetServers.empty()) return;

        std::string payload = MessageSerializer::Pack({ sender, room, text });
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::S2S_ROUTE_ROOM_MSG, payload);

        for (int serverId : targetServers) {
            auto peer = m_server.GetConnectionManager().GetPeerNode(serverId);
            if (peer && peer->IsConnected() && peer->GetSocket()) {
                Logger::Info("Routing room message from " + sender + " in room " + room + " to remote Server ID: " + std::to_string(serverId), "REPLICATION");
                
                const char* ptr = reinterpret_cast<const char*>(buffer.data());
                int bytesLeft = static_cast<int>(buffer.size());
                bool sendFailed = false;

                while (bytesLeft > 0) {
                    int sent = peer->GetSocket()->Send(ptr, bytesLeft);
                    if (sent <= 0) {
                        if (sent == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(5));
                            continue;
                        }
                        sendFailed = true;
                        break;
                    }
                    ptr += sent;
                    bytesLeft -= sent;
                }

                if (sendFailed) {
                    Logger::Error("Failed to route room message to peer Server ID: " + std::to_string(serverId), "REPLICATION");
                    m_server.HandlePeerDisconnect(serverId);
                }
            }
        }
    }
}