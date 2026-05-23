#pragma once
#include "ClientSession.h"
#include "ServerNode.h"

namespace DistributedChat {
    class ConnectionManager {
    private:
        std::unordered_map<SOCKET, std::shared_ptr<ClientSession>> m_sessionsBySocket;
        std::unordered_map<std::string, std::shared_ptr<ClientSession>> m_sessionsByUsername;
        
        std::unordered_map<int, std::shared_ptr<ServerNode>> m_peerNodes;
        std::unordered_map<SOCKET, std::shared_ptr<ServerNode>> m_peerNodesBySocket;

        mutable std::mutex m_mutex;

    public:
        ConnectionManager() = default;
        ~ConnectionManager() = default;

        // Client sessions
        void AddSession(const std::shared_ptr<ClientSession>& session);
        void AuthenticateSession(const std::shared_ptr<ClientSession>& session, const std::string& username);
        void RemoveSession(SOCKET socket);
        std::shared_ptr<ClientSession> GetSessionBySocket(SOCKET socket) const;
        std::shared_ptr<ClientSession> GetSessionByUsername(const std::string& username) const;
        std::vector<std::shared_ptr<ClientSession>> GetAllSessions() const;

        // Peer server nodes
        void AddPeerNode(const std::shared_ptr<ServerNode>& node);
        void RegisterPeerSocket(int serverId, const std::shared_ptr<ISocket>& socket);
        void UnregisterPeerSocket(int serverId);
        std::shared_ptr<ServerNode> GetPeerNode(int serverId) const;
        std::shared_ptr<ServerNode> GetPeerNodeBySocket(SOCKET socket) const;
        std::vector<std::shared_ptr<ServerNode>> GetAllPeerNodes() const;
    };
}