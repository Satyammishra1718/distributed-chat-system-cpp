#include "../../include/protocol/MessageSerializer.h"

namespace DistributedChat {
    std::vector<uint8_t> MessageSerializer::Serialize(PacketType type, const std::string& payload) {
        uint32_t payloadLen = static_cast<uint32_t>(payload.size());
        uint32_t totalLen = 1 + payloadLen; // 1 byte type + payload

        std::vector<uint8_t> buffer(4 + totalLen);
        
        uint32_t netLen = htonl(totalLen);
        std::memcpy(buffer.data(), &netLen, sizeof(netLen));

        buffer[4] = static_cast<uint8_t>(type);

        if (payloadLen > 0) {
            std::memcpy(buffer.data() + 5, payload.data(), payloadLen);
        }

        return buffer;
    }

    bool MessageSerializer::ReadPacket(ISocket& socket, RawPacket& outPacket) {
        uint32_t netLen = 0;
        if (!socket.RecvAll(&netLen, sizeof(netLen))) {
            return false;
        }

        uint32_t totalLen = ntohl(netLen);
        if (totalLen < 1 || totalLen > MAX_PAYLOAD_SIZE) {
            return false;
        }

        uint8_t rawType = 0;
        if (!socket.RecvAll(&rawType, sizeof(rawType))) {
            return false;
        }
        outPacket.type = static_cast<PacketType>(rawType);

        uint32_t payloadLen = totalLen - 1;
        if (payloadLen > 0) {
            std::vector<char> payloadBuf(payloadLen);
            if (!socket.RecvAll(payloadBuf.data(), payloadLen)) {
                return false;
            }
            outPacket.payload.assign(payloadBuf.data(), payloadLen);
        } else {
            outPacket.payload.clear();
        }

        return true;
    }

    std::string MessageSerializer::Pack(const std::vector<std::string>& fields) {
        std::string packed;
        for (const auto& field : fields) {
            packed += field;
            packed += '\0';
        }
        return packed;
    }

    std::vector<std::string> MessageSerializer::Unpack(const std::string& packed) {
        std::vector<std::string> fields;
        std::string current;
        for (char c : packed) {
            if (c == '\0') {
                fields.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            fields.push_back(current);
        }
        return fields;
    }
}