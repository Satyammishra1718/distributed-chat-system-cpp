#pragma once
#include "../core/common/Common.h"

namespace DistributedChat {
    enum class PacketType : uint8_t {
        LOGIN = 1,
        LOGIN_RESP = 2,
        MSG_PRIVATE = 3,
        MSG_ROOM = 4,
        MSG_BROADCAST = 5,
        ROOM_JOIN = 6,
        ROOM_LEAVE = 7,
        ROOM_CREATE = 8,
        STATUS_UPDATE = 9,
        
        S2S_CONNECT = 100,
        S2S_HEARTBEAT = 101,
        S2S_HEARTBEAT_ACK = 102,
        S2S_REPLICATE_USER = 103,
        S2S_ROUTE_MSG = 104,
        S2S_ROUTE_ROOM_MSG = 105,
        S2S_ROOM_MEMBER_SYNC = 106
    };

    struct RawPacket {
        PacketType type;
        std::string payload;
    };
}