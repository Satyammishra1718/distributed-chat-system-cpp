#include "../../../include/core/networking/TcpSocket.h"

namespace DistributedChat {
    namespace SocketUtils {
        bool IsSocketValid(SOCKET s) {
            return s != INVALID_SOCKET;
        }
        
        bool SetKeepAlive(SOCKET s, bool enable) {
            int optval = enable ? 1 : 0;
            return setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&optval), sizeof(optval)) == 0;
        }
    }
}