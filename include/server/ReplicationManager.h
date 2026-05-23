#pragma once
#include "../core/common/Common.h"
#include "../protocol/Packet.h"

namespace DistributedChat {
    class ChatServer; // Forward declaration
    class ConnectionManager;

    class ReplicationManager {
    private:
        ChatServer& m_server;

    public:
        explicit ReplicationManager(ChatServer& server);
        ~ReplicationManager() = default;

        void ReplicateUserLogin(const std::string& username);
        void ReplicateUserLogout(const std::string& username);
        void ReplicateRoomJoin(const std::string& room);
        void ReplicateRoomLeave(const std::string& room);

        void RoutePrivateMessage(const std::string& sender, const std::string& receiver, const std::string& text, int targetServerId);
        void RouteRoomMessage(const std::string& sender, const std::string& room, const std::string& text);
        void BroadcastS2S(PacketType type, const std::string& payload);
    };
}