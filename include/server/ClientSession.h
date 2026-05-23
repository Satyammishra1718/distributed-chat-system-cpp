#pragma once
#include "../core/common/Common.h"
#include "../core/networking/ISocket.h"
#include "../protocol/Packet.h"

namespace DistributedChat {
    class ClientSession : public std::enable_shared_from_this<ClientSession> {
    private:
        std::shared_ptr<ISocket> m_socket;
        std::string m_username;
        bool m_isAuthenticated;
        std::unordered_set<std::string> m_joinedRooms;
        std::mutex m_sessionMutex;

    public:
        explicit ClientSession(std::shared_ptr<ISocket> socket);
        ~ClientSession() = default;

        std::shared_ptr<ISocket> GetSocket() const { return m_socket; }
        std::string GetUsername() const;
        void SetUsername(const std::string& username);
        
        bool IsAuthenticated() const;
        void SetAuthenticated(bool authenticated);

        void JoinRoom(const std::string& roomName);
        void LeaveRoom(const std::string& roomName);
        bool IsInRoom(const std::string& roomName);
        std::vector<std::string> GetJoinedRooms();

        bool SendPacket(PacketType type, const std::string& payload);
    };
}