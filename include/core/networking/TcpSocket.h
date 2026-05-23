#pragma once
#include "ISocket.h"

namespace DistributedChat {
    class TcpSocket : public ISocket {
    private:
        SOCKET m_socket;
        bool m_isClosed;

    public:
        TcpSocket();
        explicit TcpSocket(SOCKET socket);
        ~TcpSocket() override;

        bool Bind(int port) override;
        bool Listen(int backlog = SOMAXCONN) override;
        std::shared_ptr<ISocket> Accept() override;
        bool Connect(const std::string& host, int port) override;
        int Send(const void* buf, int len) override;
        int Recv(void* buf, int len) override;
        void Close() override;
        SOCKET GetHandle() const override { return m_socket; }
        bool SetNonBlocking(bool nonBlocking) override;

        bool SendAll(const void* buf, int len);
        bool RecvAll(void* buf, int len);

        static void InitializeWinsock();
        static void CleanupWinsock();
        static std::string GetLastErrorString();
    };
}