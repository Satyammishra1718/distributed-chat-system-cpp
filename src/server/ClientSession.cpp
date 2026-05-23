#include "../../include/server/ClientSession.h"
#include "../../include/protocol/MessageSerializer.h"

namespace DistributedChat {
    ClientSession::ClientSession(std::shared_ptr<ISocket> socket)
        : m_socket(socket), m_isAuthenticated(false) {}

    std::string ClientSession::GetUsername() const {
        return m_username;
    }

    void ClientSession::SetUsername(const std::string& username) {
        m_username = username;
    }

    bool ClientSession::IsAuthenticated() const {
        return m_isAuthenticated;
    }

    void ClientSession::SetAuthenticated(bool authenticated) {
        m_isAuthenticated = authenticated;
    }

    void ClientSession::JoinRoom(const std::string& roomName) {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        m_joinedRooms.insert(roomName);
    }

    void ClientSession::LeaveRoom(const std::string& roomName) {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        m_joinedRooms.erase(roomName);
    }

    bool ClientSession::IsInRoom(const std::string& roomName) {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        return m_joinedRooms.find(roomName) != m_joinedRooms.end();
    }

    std::vector<std::string> ClientSession::GetJoinedRooms() {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        return std::vector<std::string>(m_joinedRooms.begin(), m_joinedRooms.end());
    }

    bool ClientSession::SendPacket(PacketType type, const std::string& payload) {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        if (!m_socket) return false;
        
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(type, payload);
        
        const char* ptr = reinterpret_cast<const char*>(buffer.data());
        int bytesLeft = static_cast<int>(buffer.size());
        while (bytesLeft > 0) {
            int sent = m_socket->Send(ptr, bytesLeft);
            if (sent <= 0) {
                if (sent == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    continue;
                }
                return false;
            }
            ptr += sent;
            bytesLeft -= sent;
        }
        return true;
    }
}