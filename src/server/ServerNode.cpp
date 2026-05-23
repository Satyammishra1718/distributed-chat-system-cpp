#include "../../include/server/ServerNode.h"

namespace DistributedChat {
    ServerNode::ServerNode(int serverId, const std::string& host, int port)
        : m_serverId(serverId), m_host(host), m_port(port), m_socket(nullptr),
          m_lastHeartbeatTime(std::chrono::steady_clock::now()), m_isConnected(false) {}

    void ServerNode::SetSocket(std::shared_ptr<ISocket> socket) {
        m_socket = socket;
        m_isConnected = (socket != nullptr);
        if (m_isConnected) {
            UpdateHeartbeat();
        }
    }
}