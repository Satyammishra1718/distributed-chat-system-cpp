#pragma once
#include "../core/common/Common.h"

namespace DistributedChat {
    struct ChatRoom {
        std::unordered_set<std::string> localMembers; // local usernames
        std::unordered_set<int> remoteServers;        // remote serverIds containing members
    };

    class ChatRoomManager {
    private:
        std::unordered_map<std::string, ChatRoom> m_rooms;
        mutable std::mutex m_mutex;

    public:
        ChatRoomManager() = default;
        ~ChatRoomManager() = default;

        void JoinRoomLocal(const std::string& room, const std::string& username);
        void LeaveRoomLocal(const std::string& room, const std::string& username);
        void JoinRoomRemote(const std::string& room, int serverId);
        void LeaveRoomRemote(const std::string& room, int serverId);
        
        std::vector<std::string> GetLocalMembers(const std::string& room) const;
        std::vector<int> GetRemoteServers(const std::string& room) const;
        
        void ClearServerMemberships(int serverId);
        std::vector<std::string> GetRooms() const;
    };
}