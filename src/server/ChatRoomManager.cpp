#include "../../include/server/ChatRoomManager.h"

namespace DistributedChat {
    void ChatRoomManager::JoinRoomLocal(const std::string& room, const std::string& username) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rooms[room].localMembers.insert(username);
    }

    void ChatRoomManager::LeaveRoomLocal(const std::string& room, const std::string& username) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_rooms.find(room);
        if (it != m_rooms.end()) {
            it->second.localMembers.erase(username);
        }
    }

    void ChatRoomManager::JoinRoomRemote(const std::string& room, int serverId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rooms[room].remoteServers.insert(serverId);
    }

    void ChatRoomManager::LeaveRoomRemote(const std::string& room, int serverId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_rooms.find(room);
        if (it != m_rooms.end()) {
            it->second.remoteServers.erase(serverId);
        }
    }

    std::vector<std::string> ChatRoomManager::GetLocalMembers(const std::string& room) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_rooms.find(room);
        if (it != m_rooms.end()) {
            return std::vector<std::string>(it->second.localMembers.begin(), it->second.localMembers.end());
        }
        return {};
    }

    std::vector<int> ChatRoomManager::GetRemoteServers(const std::string& room) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_rooms.find(room);
        if (it != m_rooms.end()) {
            return std::vector<int>(it->second.remoteServers.begin(), it->second.remoteServers.end());
        }
        return {};
    }

    void ChatRoomManager::ClearServerMemberships(int serverId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_rooms) {
            pair.second.remoteServers.erase(serverId);
        }
    }

    std::vector<std::string> ChatRoomManager::GetRooms() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::string> rooms;
        for (const auto& pair : m_rooms) {
            rooms.push_back(pair.first);
        }
        return rooms;
    }
}