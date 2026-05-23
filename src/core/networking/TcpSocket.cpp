#include "../../../include/core/networking/TcpSocket.h"

#include <iostream>
#include <thread>
#include <chrono>

namespace DistributedChat {

    static int s_wsaRefCount = 0;
    static std::mutex s_wsaMutex;

    void TcpSocket::InitializeWinsock() {
        std::lock_guard<std::mutex> lock(s_wsaMutex);

        if (s_wsaRefCount == 0) {
            WSADATA wsaData;

            int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

            if (result != 0) {
                std::cerr << "WSAStartup failed with error: "
                          << result << std::endl;
            }
        }

        s_wsaRefCount++;
    }

    void TcpSocket::CleanupWinsock() {
        std::lock_guard<std::mutex> lock(s_wsaMutex);

        s_wsaRefCount--;

        if (s_wsaRefCount == 0) {
            WSACleanup();
        }
    }

    std::string TcpSocket::GetLastErrorString() {
        int err = WSAGetLastError();

        // Simplified safe error handling
        return "Socket error code: " + std::to_string(err);
    }

    TcpSocket::TcpSocket()
        : m_socket(INVALID_SOCKET),
          m_isClosed(false) {

        InitializeWinsock();

        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (m_socket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: "
                      << GetLastErrorString() << std::endl;
        }
    }

    TcpSocket::TcpSocket(SOCKET s)
        : m_socket(s),
          m_isClosed(false) {

        InitializeWinsock();
    }

    TcpSocket::~TcpSocket() {
        Close();
        CleanupWinsock();
    }

    bool TcpSocket::Bind(int port) {
        if (m_socket == INVALID_SOCKET) {
            return false;
        }

        char optval = 1;

        setsockopt(
            m_socket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &optval,
            sizeof(optval)
        );

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(static_cast<USHORT>(port));

        if (bind(
                m_socket,
                reinterpret_cast<sockaddr*>(&addr),
                sizeof(addr)
            ) == SOCKET_ERROR) {

            std::cerr << "Bind failed: "
                      << GetLastErrorString() << std::endl;

            return false;
        }

        return true;
    }

    bool TcpSocket::Listen(int backlog) {
        if (m_socket == INVALID_SOCKET) {
            return false;
        }

        if (listen(m_socket, backlog) == SOCKET_ERROR) {

            std::cerr << "Listen failed: "
                      << GetLastErrorString() << std::endl;

            return false;
        }

        return true;
    }

    std::shared_ptr<ISocket> TcpSocket::Accept() {
        if (m_socket == INVALID_SOCKET) {
            return nullptr;
        }

        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);

        SOCKET s = accept(
            m_socket,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientAddrSize
        );

        if (s == INVALID_SOCKET) {
            return nullptr;
        }

        return std::make_shared<TcpSocket>(s);
    }

    bool TcpSocket::Connect(const std::string& host, int port) {
        if (m_socket == INVALID_SOCKET) {
            return false;
        }

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;

        int err = getaddrinfo(
            host.c_str(),
            std::to_string(port).c_str(),
            &hints,
            &result
        );

        if (err != 0) {
            return false;
        }

        bool connected = false;

        for (addrinfo* ptr = result;
             ptr != nullptr;
             ptr = ptr->ai_next) {

            if (connect(
                    m_socket,
                    ptr->ai_addr,
                    static_cast<int>(ptr->ai_addrlen)
                ) != SOCKET_ERROR) {

                connected = true;
                break;
            }
        }

        freeaddrinfo(result);

        return connected;
    }

    int TcpSocket::Send(const void* buf, int len) {
        if (m_socket == INVALID_SOCKET) {
            return -1;
        }

        return send(
            m_socket,
            reinterpret_cast<const char*>(buf),
            len,
            0
        );
    }

    int TcpSocket::Recv(void* buf, int len) {
        if (m_socket == INVALID_SOCKET) {
            return -1;
        }

        return recv(
            m_socket,
            reinterpret_cast<char*>(buf),
            len,
            0
        );
    }

    void TcpSocket::Close() {
        if (!m_isClosed) {

            if (m_socket != INVALID_SOCKET) {

                closesocket(m_socket);

                m_socket = INVALID_SOCKET;
            }

            m_isClosed = true;
        }
    }

    bool TcpSocket::SetNonBlocking(bool nonBlocking) {
        if (m_socket == INVALID_SOCKET) {
            return false;
        }

        u_long mode = nonBlocking ? 1 : 0;

        return ioctlsocket(
            m_socket,
            FIONBIO,
            &mode
        ) == 0;
    }

    bool TcpSocket::SendAll(const void* buf, int len) {

        const char* ptr =
            reinterpret_cast<const char*>(buf);

        int bytesLeft = len;

        while (bytesLeft > 0) {

            int sent = Send(ptr, bytesLeft);

            if (sent <= 0) {

                if (sent == SOCKET_ERROR &&
                    WSAGetLastError() == WSAEWOULDBLOCK) {

                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(5)
                    );

                    continue;
                }

                return false;
            }

            ptr += sent;
            bytesLeft -= sent;
        }

        return true;
    }

    bool TcpSocket::RecvAll(void* buf, int len) {

        char* ptr =
            reinterpret_cast<char*>(buf);

        int bytesLeft = len;

        while (bytesLeft > 0) {

            int recved = Recv(ptr, bytesLeft);

            if (recved <= 0) {

                if (recved == SOCKET_ERROR &&
                    WSAGetLastError() == WSAEWOULDBLOCK) {

                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(5)
                    );

                    continue;
                }

                return false;
            }

            ptr += recved;
            bytesLeft -= recved;
        }

        return true;
    }

}