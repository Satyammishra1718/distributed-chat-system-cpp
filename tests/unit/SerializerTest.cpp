#include "../../include/protocol/MessageSerializer.h"
#include <iostream>
#include <cassert>

using namespace DistributedChat;

int main() {
    std::cout << "[SerializerTest] Running serialization and unpacking tests...\n";

    std::vector<std::string> fields = { "Alice", "lobby", "hello world" };
    std::string packed = MessageSerializer::Pack(fields);
    std::vector<std::string> unpacked = MessageSerializer::Unpack(packed);

    assert(unpacked.size() == 3);
    assert(unpacked[0] == "Alice");
    assert(unpacked[1] == "lobby");
    assert(unpacked[2] == "hello world");

    std::string payload = "Hello world";
    std::vector<uint8_t> buffer = MessageSerializer::Serialize(PacketType::MSG_BROADCAST, payload);

    assert(buffer.size() == 4 + 1 + payload.size());
    assert(buffer[4] == static_cast<uint8_t>(PacketType::MSG_BROADCAST));

    std::cout << "[SerializerTest] All serializer tests completed successfully!\n";
    return 0;
}