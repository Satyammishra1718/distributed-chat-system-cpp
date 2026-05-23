#pragma once
#include "../common/Common.h"

namespace DistributedChat {
    class ISocket {
    public:
        virtual ~ISocket() = default;
        virtual bool Bind(int port) = 0;
        virtual bool Listen(int backlog = SOMAXCONN) = 0;
        virtual std::shared_ptr<ISocket> Accept() = 0;
        virtual bool Connect(const std::string& host, int port) = 0;
        virtual int Send(const void* buf, int len) = 0;
        virtual int Recv(void* buf, int len) = 0;
        virtual bool SendAll(const void* buf, int len) = 0;
        virtual bool RecvAll(void* buf, int len) = 0;
        virtual void Close() = 0;
        virtual SOCKET GetHandle() const = 0;
        virtual bool SetNonBlocking(bool nonBlocking) = 0;
    };
}