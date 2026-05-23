#include "../../include/server/HeartbeatManager.h"
#include "../../include/server/ChatServer.h"
#include "../../include/protocol/MessageSerializer.h"
#include "../../include/core/logging/Logger.h"

namespace DistributedChat {
    HeartbeatManager::HeartbeatManager(ChatServer& server)
        : m_server(server), m_running(false) {}

    HeartbeatManager::~HeartbeatManager() {
        Stop();
    }

    void HeartbeatManager::Start() {
        if (!m_running.exchange(true)) {
            m_thread = std::thread(&HeartbeatManager::HeartbeatLoop, this);
        }
    }

    void HeartbeatManager::Stop() {
        if (m_running.exchange(false)) {
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }
    }

    void HeartbeatManager::HeartbeatLoop() {
        Logger::Info("Heartbeat manager thread started", "HEARTBEAT");
        while (m_running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!m_running) break;

            auto peers = m_server.GetConnectionManager().GetAllPeerNodes();
            auto now = std::chrono::steady_clock::now();

            for (const auto& peer : peers) {
                if (peer->IsConnected()) {
                    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - peer->GetLastHeartbeat()).count();
                    if (duration >= 15) {
                        Logger::Warn("Peer Server ID " + std::to_string(peer->GetServerId()) + 
                                     " heartbeat timed out (" + std::to_string(duration) + "s since last contact). Disconnecting.", "HEARTBEAT");
                        m_server.HandlePeerDisconnect(peer->GetServerId());
                        continue;
                    }

                    std::string payload = std::to_string(m_server.GetServerId());
                    std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::S2S_HEARTBEAT, payload);

                    if (peer->GetSocket()) {
                        const char* ptr = reinterpret_cast<const char*>(buffer.data());
                        int bytesLeft = static_cast<int>(buffer.size());
                        bool sendFailed = false;

                        while (bytesLeft > 0) {
                            int sent = peer->GetSocket()->Send(ptr, bytesLeft);
                            if (sent <= 0) {
                                if (sent == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
                                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                                    continue;
                                }
                                sendFailed = true;
                                break;
                            }
                            ptr += sent;
                            bytesLeft -= sent;
                        }

                        if (sendFailed) {
                            Logger::Error("Failed to send heartbeat ping to peer Server ID: " + std::to_string(peer->GetServerId()), "HEARTBEAT");
                            m_server.HandlePeerDisconnect(peer->GetServerId());
                        } else {
                            Logger::Debug("Heartbeat ping sent to peer Server ID: " + std::to_string(peer->GetServerId()), "HEARTBEAT");
                        }
                    }
                }
            }
        }
        Logger::Info("Heartbeat manager thread stopped", "HEARTBEAT");
    }
}