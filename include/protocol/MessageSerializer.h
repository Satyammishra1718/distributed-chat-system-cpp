#pragma once
#include "Packet.h"
#include "../core/networking/ISocket.h"

namespace DistributedChat {
    class MessageSerializer {
    public:
        static const uint32_t MAX_PAYLOAD_SIZE = 1024 * 1024; // 1 MB limit for safety

        static std::vector<uint8_t> Serialize(PacketType type, const std::string& payload);
        static bool ReadPacket(ISocket& socket, RawPacket& outPacket);

        // Helper methods to serialize multi-field strings using '\0' delimiter
        static std::string Pack(const std::vector<std::string>& fields);
        static std::vector<std::string> Unpack(const std::string& packed);
    };
}