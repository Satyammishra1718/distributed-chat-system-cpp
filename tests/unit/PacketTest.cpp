#include "../../include/protocol/Packet.h"
#include <iostream>
#include <cassert>

using namespace DistributedChat;

int main() {
    std::cout << "[PacketTest] Running packet structures tests...\n";
    
    RawPacket p;
    p.type = PacketType::LOGIN;
    p.payload = "Alice";
    
    assert(p.type == PacketType::LOGIN);
    assert(p.payload == "Alice");
    
    std::cout << "[PacketTest] All packet tests passed successfully!\n";
    return 0;
}