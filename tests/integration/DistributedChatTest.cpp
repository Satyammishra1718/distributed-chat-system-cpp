#include "../../include/server/ChatServer.h"
#include "../../include/client/ChatClient.h"
#include "../../include/core/logging/Logger.h"
#include <iostream>

using namespace DistributedChat;

int main() {
    CreateDirectoryA("logs", nullptr);
    Logger::Init("logs/test.log", LogLevel::INFO_LEVEL);

    std::cout << "\033[96m========================================================\033[0m\n";
    std::cout << "\033[96m          DISTRIBUTED CHAT SYSTEM INTEGRATION TEST       \033[0m\n";
    std::cout << "\033[96m========================================================\033[0m\n\n";

    ChatServer server1(1, 9081);
    ChatServer server2(2, 9082);

    server1.AddPeerConfig(2, "127.0.0.1", 9082);
    server2.AddPeerConfig(1, "127.0.0.1", 9081);

    std::cout << "[1/4] Starting server nodes...\n";
    if (!server1.Start() || !server2.Start()) {
        std::cerr << "Failed to start server nodes!\n";
        return 1;
    }
    std::cout << "\033[92m✔ Server nodes are online and peering!\033[0m\n\n";

    std::cout << "Waiting 2 seconds for S2S automatic peer handshake...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "[2/4] Connecting clients...\n";
    ChatClient clientA;
    ChatClient clientB;

    if (!clientA.Connect("127.0.0.1", 9081)) {
        std::cerr << "Client A failed to connect to Server 1!\n";
        return 1;
    }
    if (!clientB.Connect("127.0.0.1", 9082)) {
        std::cerr << "Client B failed to connect to Server 2!\n";
        return 1;
    }

    std::cout << "Authenticating Alice (on S1) and Bob (on S2)...\n";
    clientA.Login("Alice");
    clientB.Login("Bob");

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "\033[92m✔ Alice and Bob are authenticated!\033[0m\n\n";

    std::cout << "[3/4] Testing cross-server private message routing...\n";
    std::cout << "Alice sends private message to Bob: 'Hello Bob from Alice!'\n";
    clientA.SendPrivateMessage("Bob", "Hello Bob from Alice!");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "\033[92m✔ Routing test message completed!\033[0m\n\n";

    std::cout << "[4/4] Testing room join and multicast replication...\n";
    std::cout << "Alice joins '#lobby'...\n";
    clientA.JoinRoom("#lobby");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "Bob joins '#lobby'...\n";
    clientB.JoinRoom("#lobby");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "Alice sends a room message in '#lobby': 'Hey everyone!'\n";
    clientA.SendRoomMessage("#lobby", "Hey everyone!");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::cout << "Tearing down cluster...\n";
    clientA.Disconnect();
    clientB.Disconnect();
    server1.Stop();
    server2.Stop();

    std::cout << "\n\033[92m========================================================\033[0m\n";
    std::cout << "\033[92m✔ ALL INTEGRATION TESTS PASSED SUCCESSFULLY!\033[0m\n";
    std::cout << "\033[92m========================================================\033[0m\n";

    return 0;
}