#pragma once
#include "../core/common/Common.h"
#include "../core/networking/ISocket.h"

namespace DistributedChat {
    class ServerNode {
    private:
        int m_serverId;
        std::string m_host;
        int m_port;
        std::shared_ptr<ISocket> m_socket;
        std::chrono::steady_clock::time_point m_lastHeartbeatTime;
        bool m_isConnected;

    public:
        ServerNode(int serverId, const std::string& host, int port);
        ~ServerNode() = default;

        int GetServerId() const { return m_serverId; }
        std::string GetHost() const { return m_host; }
        int GetPort() const { return m_port; }
        std::shared_ptr<ISocket> GetSocket() const { return m_socket; }
        void SetSocket(std::shared_ptr<ISocket> socket);
        
        bool IsConnected() const { return m_isConnected; }
        void SetConnected(bool connected) { m_isConnected = connected; }

        std::chrono::steady_clock::time_point GetLastHeartbeat() const { return m_lastHeartbeatTime; }
        void UpdateHeartbeat() { m_lastHeartbeatTime = std::chrono::steady_clock::now(); }
    };
}