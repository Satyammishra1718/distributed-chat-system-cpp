#pragma once
#include "../core/common/Common.h"
#include "../core/networking/TcpSocket.h"
#include "../core/threading/ThreadPool.h"
#include "ConnectionManager.h"
#include "UserManager.h"
#include "ChatRoomManager.h"
#include "ReplicationManager.h"
#include "HeartbeatManager.h"
#include "../protocol/Packet.h"

namespace DistributedChat {
    class ChatServer {
    private:
        int m_serverId;
        int m_port;
        bool m_running;

        std::shared_ptr<ISocket> m_listenerSocket;
        ConnectionManager m_connManager;
        UserManager m_userManager;
        ChatRoomManager m_roomManager;
        ReplicationManager m_replManager;
        HeartbeatManager m_hbManager;
        ThreadPool m_threadPool;

        std::thread m_listenerThread;
        std::thread m_peerConnectorThread;

        std::vector<std::pair<int, std::pair<std::string, int>>> m_peerConfigs; // serverId -> (host, port)

        void ListenerLoop();
        void PeerConnectorLoop();

        // Packet handling dispatch
        void HandleClientPacket(const std::shared_ptr<ClientSession>& session, const RawPacket& packet);
        void HandleServerPacket(const std::shared_ptr<ServerNode>& node, const RawPacket& packet);

        // Individual client packet handlers
        void HandleLogin(const std::shared_ptr<ClientSession>& session, const std::string& payload);
        void HandlePrivateMsg(const std::shared_ptr<ClientSession>& session, const std::string& payload);
        void HandleRoomMsg(const std::shared_ptr<ClientSession>& session, const std::string& payload);
        void HandleBroadcastMsg(const std::shared_ptr<ClientSession>& session, const std::string& payload);
        void HandleRoomJoin(const std::shared_ptr<ClientSession>& session, const std::string& payload);
        void HandleRoomLeave(const std::shared_ptr<ClientSession>& session, const std::string& payload);
        void HandleRoomCreate(const std::shared_ptr<ClientSession>& session, const std::string& payload);

        // Individual server packet handlers
        void HandleS2SConnect(const std::shared_ptr<ServerNode>& node, const std::string& payload);
        void HandleS2SHeartbeat(const std::shared_ptr<ServerNode>& node, const std::string& payload);
        void HandleS2SHeartbeatAck(const std::shared_ptr<ServerNode>& node, const std::string& payload);
        void HandleS2SReplicateUser(const std::shared_ptr<ServerNode>& node, const std::string& payload);
        void HandleS2SRouteMsg(const std::shared_ptr<ServerNode>& node, const std::string& payload);
        void HandleS2SRouteRoomMsg(const std::shared_ptr<ServerNode>& node, const std::string& payload);
        void HandleS2SRoomMemberSync(const std::shared_ptr<ServerNode>& node, const std::string& payload);

    public:
        ChatServer(int serverId, int port, size_t poolThreads = 4);
        ~ChatServer();

        bool Start();
        void Stop();

        void AddPeerConfig(int peerId, const std::string& host, int port);

        // Accessors
        int GetServerId() const { return m_serverId; }
        ConnectionManager& GetConnectionManager() { return m_connManager; }
        UserManager& GetUserManager() { return m_userManager; }
        ChatRoomManager& GetChatRoomManager() { return m_roomManager; }
        ReplicationManager& GetReplicationManager() { return m_replManager; }

        void HandlePeerDisconnect(int peerServerId);
    };
}