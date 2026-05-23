#pragma once
#include "../core/common/Common.h"
#include "../core/networking/TcpSocket.h"
#include "../protocol/Packet.h"

namespace DistributedChat {
    class ChatClient {
    private:
        std::shared_ptr<TcpSocket> m_socket;
        std::string m_username;
        bool m_connected;
        bool m_running;
        std::thread m_receiveThread;

        void ReceiveLoop();
        void HandlePacket(const RawPacket& packet);

    public:
        ChatClient();
        ~ChatClient();

        bool Connect(const std::string& host, int port);
        void Disconnect();

        bool Login(const std::string& username);
        void SendPrivateMessage(const std::string& receiver, const std::string& message);
        void SendRoomMessage(const std::string& room, const std::string& message);
        void SendBroadcastMessage(const std::string& message);
        
        void JoinRoom(const std::string& room);
        void LeaveRoom(const std::string& room);
        void CreateRoom(const std::string& room);
        void Logout();

        bool IsConnected() const { return m_connected; }
        std::string GetUsername() const { return m_username; }
    };
}