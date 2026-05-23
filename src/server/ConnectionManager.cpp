#include "../../include/server/ConnectionManager.h"

namespace DistributedChat {
    void ConnectionManager::AddSession(const std::shared_ptr<ClientSession>& session) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessionsBySocket[session->GetSocket()->GetHandle()] = session;
    }

    void ConnectionManager::AuthenticateSession(const std::shared_ptr<ClientSession>& session, const std::string& username) {
        std::lock_guard<std::mutex> lock(m_mutex);
        session->SetUsername(username);
        session->SetAuthenticated(true);
        m_sessionsByUsername[username] = session;
    }

    void ConnectionManager::RemoveSession(SOCKET socket) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessionsBySocket.find(socket);
        if (it != m_sessionsBySocket.end()) {
            std::string username = it->second->GetUsername();
            if (!username.empty()) {
                m_sessionsByUsername.erase(username);
            }
            m_sessionsBySocket.erase(it);
        }
    }

    std::shared_ptr<ClientSession> ConnectionManager::GetSessionBySocket(SOCKET socket) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessionsBySocket.find(socket);
        if (it != m_sessionsBySocket.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::shared_ptr<ClientSession> ConnectionManager::GetSessionByUsername(const std::string& username) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessionsByUsername.find(username);
        if (it != m_sessionsByUsername.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<ClientSession>> ConnectionManager::GetAllSessions() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::shared_ptr<ClientSession>> sessions;
        for (const auto& pair : m_sessionsBySocket) {
            sessions.push_back(pair.second);
        }
        return sessions;
    }

    void ConnectionManager::AddPeerNode(const std::shared_ptr<ServerNode>& node) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_peerNodes[node->GetServerId()] = node;
    }

    void ConnectionManager::RegisterPeerSocket(int serverId, const std::shared_ptr<ISocket>& socket) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_peerNodes.find(serverId);
        if (it != m_peerNodes.end()) {
            it->second->SetSocket(socket);
            m_peerNodesBySocket[socket->GetHandle()] = it->second;
        }
    }

    void ConnectionManager::UnregisterPeerSocket(int serverId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_peerNodes.find(serverId);
        if (it != m_peerNodes.end()) {
            auto socket = it->second->GetSocket();
            if (socket) {
                m_peerNodesBySocket.erase(socket->GetHandle());
            }
            it->second->SetSocket(nullptr);
        }
    }

    std::shared_ptr<ServerNode> ConnectionManager::GetPeerNode(int serverId) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_peerNodes.find(serverId);
        if (it != m_peerNodes.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::shared_ptr<ServerNode> ConnectionManager::GetPeerNodeBySocket(SOCKET socket) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_peerNodesBySocket.find(socket);
        if (it != m_peerNodesBySocket.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<ServerNode>> ConnectionManager::GetAllPeerNodes() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::shared_ptr<ServerNode>> nodes;
        for (const auto& pair : m_peerNodes) {
            nodes.push_back(pair.second);
        }
        return nodes;
    }
}