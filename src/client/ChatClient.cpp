#include "../../include/client/ChatClient.h"
#include "../../include/protocol/MessageSerializer.h"
#include <iostream>

namespace DistributedChat {
    ChatClient::ChatClient() : m_connected(false), m_running(false) {
        m_socket = std::make_shared<TcpSocket>();
    }

    ChatClient::~ChatClient() {
        Disconnect();
    }

    bool ChatClient::Connect(const std::string& host, int port) {
        if (m_connected) return false;
        
        if (!m_socket->Connect(host, port)) {
            return false;
        }

        m_connected = true;
        m_running = true;
        m_receiveThread = std::thread(&ChatClient::ReceiveLoop, this);
        return true;
    }

    void ChatClient::Disconnect() {
        if (!m_connected) return;
        m_running = false;
        if (m_socket) {
            m_socket->Close();
        }
        if (m_receiveThread.joinable()) {
            m_receiveThread.join();
        }
        m_connected = false;
    }

    bool ChatClient::Login(const std::string& username) {
        if (!m_connected) return false;
        m_username = username;
        
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::LOGIN, username);
        return m_socket->SendAll(buffer.data(), static_cast<int>(buffer.size()));
    }

    void ChatClient::SendPrivateMessage(const std::string& receiver, const std::string& message) {
        if (!m_connected) return;
        std::string payload = MessageSerializer::Pack({ receiver, message });
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::MSG_PRIVATE, payload);
        m_socket->SendAll(buffer.data(), static_cast<int>(buffer.size()));
    }

    void ChatClient::SendRoomMessage(const std::string& room, const std::string& message) {
        if (!m_connected) return;
        std::string payload = MessageSerializer::Pack({ room, message });
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::MSG_ROOM, payload);
        m_socket->SendAll(buffer.data(), static_cast<int>(buffer.size()));
    }

    void ChatClient::SendBroadcastMessage(const std::string& message) {
        if (!m_connected) return;
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::MSG_BROADCAST, message);
        m_socket->SendAll(buffer.data(), static_cast<int>(buffer.size()));
    }
    
    void ChatClient::JoinRoom(const std::string& room) {
        if (!m_connected) return;
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::ROOM_JOIN, room);
        m_socket->SendAll(buffer.data(), static_cast<int>(buffer.size()));
    }

    void ChatClient::LeaveRoom(const std::string& room) {
        if (!m_connected) return;
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::ROOM_LEAVE, room);
        m_socket->SendAll(buffer.data(), static_cast<int>(buffer.size()));
    }

    void ChatClient::CreateRoom(const std::string& room) {
        if (!m_connected) return;
        std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::ROOM_CREATE, room);
        m_socket->SendAll(buffer.data(), static_cast<int>(buffer.size()));
    }

    void ChatClient::Logout() {
        Disconnect();
    }

    void ChatClient::ReceiveLoop() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }

        while (m_running) {
            RawPacket packet;
            if (MessageSerializer::ReadPacket(*m_socket, packet)) {
                HandlePacket(packet);
            } else {
                if (m_running) {
                    std::cout << "\n\033[91m[SYSTEM] Connection to server lost.\033[0m\n>>> " << std::flush;
                    m_connected = false;
                    m_running = false;
                }
                break;
            }
        }
    }

    void ChatClient::HandlePacket(const RawPacket& packet) {
        switch (packet.type) {
            case PacketType::LOGIN_RESP: {
                std::vector<std::string> fields = MessageSerializer::Unpack(packet.payload);
                if (fields.size() >= 2) {
                    bool success = (fields[0] == "1");
                    std::string msg = fields[1];
                    if (success) {
                        std::cout << "\n\033[92m[LOGIN SUCCESS] " << msg << "\033[0m\n>>> " << std::flush;
                    } else {
                        std::cout << "\n\033[91m[LOGIN FAILED] " << msg << "\033[0m\n>>> " << std::flush;
                    }
                }
                break;
            }
            case PacketType::MSG_PRIVATE: {
                std::vector<std::string> fields = MessageSerializer::Unpack(packet.payload);
                if (fields.size() >= 2) {
                    std::string sender = fields[0];
                    std::string text = fields[1];
                    std::cout << "\n\033[96m[Private Message] " << sender << " -> " << text << "\033[0m\n>>> " << std::flush;
                }
                break;
            }
            case PacketType::MSG_ROOM: {
                std::vector<std::string> fields = MessageSerializer::Unpack(packet.payload);
                if (fields.size() >= 3) {
                    std::string sender = fields[0];
                    std::string room = fields[1];
                    std::string text = fields[2];
                    std::cout << "\n\033[94m[" << room << "] " << sender << ": " << text << "\033[0m\n>>> " << std::flush;
                }
                break;
            }
            case PacketType::MSG_BROADCAST: {
                std::vector<std::string> fields = MessageSerializer::Unpack(packet.payload);
                if (fields.size() >= 2) {
                    std::string sender = fields[0];
                    std::string text = fields[1];
                    std::cout << "\n\033[95m[Global Broadcast] " << sender << ": " << text << "\033[0m\n>>> " << std::flush;
                } else {
                    std::cout << "\n\033[95m[Global Broadcast]: " << packet.payload << "\033[0m\n>>> " << std::flush;
                }
                break;
            }
            case PacketType::STATUS_UPDATE: {
                std::vector<std::string> fields = MessageSerializer::Unpack(packet.payload);
                if (fields.empty()) return;

                std::string cat = fields[0];
                if (cat == "STATUS") {
                    std::cout << "\n\033[90m================ ONLINE USERS =================\033[0m\n";
                    for (size_t i = 1; i < fields.size(); ++i) {
                        std::cout << " \033[92m●\033[0m " << fields[i] << "\n";
                    }
                    std::cout << "\033[90m===============================================\033[0m\n>>> " << std::flush;
                } else if (cat == "SUCCESS") {
                    std::cout << "\n\033[92m[SUCCESS] " << fields[1] << "\033[0m\n>>> " << std::flush;
                } else if (cat == "INFO") {
                    std::cout << "\n\033[93m[INFO] " << fields[1] << "\033[0m\n>>> " << std::flush;
                } else if (cat == "ERROR") {
                    std::cout << "\n\033[91m[ERROR] " << fields[1] << "\033[0m\n>>> " << std::flush;
                }
                break;
            }
            default:
                break;
        }
    }
}