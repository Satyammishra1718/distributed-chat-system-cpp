#include "../../include/server/UserManager.h"

namespace DistributedChat {
    UserManager::UserManager(int localServerId) : m_localServerId(localServerId) {}

    void UserManager::RegisterUser(const std::string& username, int serverId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_userLocations[username] = serverId;
    }

    void UserManager::UnregisterUser(const std::string& username) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_userLocations.erase(username);
    }

    int UserManager::GetUserLocation(const std::string& username) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_userLocations.find(username);
        if (it != m_userLocations.end()) {
            return it->second;
        }
        return -1;
    }

    bool UserManager::IsUserOnline(const std::string& username) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_userLocations.find(username) != m_userLocations.end();
    }

    void UserManager::ClearServerUsers(int serverId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_userLocations.begin(); it != m_userLocations.end(); ) {
            if (it->second == serverId) {
                it = m_userLocations.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::vector<std::string> UserManager::GetOnlineUsers() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::string> users;
        for (const auto& pair : m_userLocations) {
            users.push_back(pair.first);
        }
        return users;
    }
}