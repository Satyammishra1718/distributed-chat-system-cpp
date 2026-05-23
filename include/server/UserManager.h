#pragma once
#include "../core/common/Common.h"

namespace DistributedChat {
    class UserManager {
    private:
        std::unordered_map<std::string, int> m_userLocations; // username -> serverId
        int m_localServerId;
        mutable std::mutex m_mutex;

    public:
        explicit UserManager(int localServerId);
        ~UserManager() = default;

        void RegisterUser(const std::string& username, int serverId);
        void UnregisterUser(const std::string& username);
        int GetUserLocation(const std::string& username) const;
        bool IsUserOnline(const std::string& username) const;
        void ClearServerUsers(int serverId);
        std::vector<std::string> GetOnlineUsers() const;
    };
}