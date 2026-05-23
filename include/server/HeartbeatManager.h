#pragma once
#include "../core/common/Common.h"
#include <atomic>

namespace DistributedChat {
    class ChatServer; // Forward declaration

    class HeartbeatManager {
    private:
        ChatServer& m_server;
        std::thread m_thread;
        std::atomic<bool> m_running;

        void HeartbeatLoop();

    public:
        explicit HeartbeatManager(ChatServer& server);
        ~HeartbeatManager();

        void Start();
        void Stop();
    };
}