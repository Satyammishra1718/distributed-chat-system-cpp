#include "../include/server/ChatServer.h"
#include "../include/core/config/ConfigManager.h"
#include "../include/core/logging/Logger.h"

int main(int argc, char* argv[]) {
    std::string configPath = "configs/server_1.cfg";
    if (argc > 1) {
        configPath = argv[1];
    }

    DistributedChat::ConfigManager config;
    if (!config.Load(configPath)) {
        std::cerr << "Failed to load config file: " << configPath << "\n";
        std::cerr << "Usage: main_server <config_file_path>\n";
        return 1;
    }

    int serverId = config.GetInt("server_id", 1);
    int port = config.GetInt("port", 9000 + serverId);

    // Ensure logs folder exists
    CreateDirectoryA("logs", nullptr);

    std::string logFile = "logs/server_" + std::to_string(serverId) + ".log";
    DistributedChat::Logger::Init(logFile, DistributedChat::LogLevel::INFO_LEVEL);

    DistributedChat::Logger::Info("Loaded server configuration from " + configPath, "MAIN");

    DistributedChat::ChatServer server(serverId, port);

    int peerCount = config.GetInt("peer_count", 0);
    for (int i = 1; i <= peerCount; ++i) {
        std::string prefix = "peer_" + std::to_string(i) + "_";
        int peerId = config.GetInt(prefix + "id", 0);
        std::string peerHost = config.GetString(prefix + "host", "127.0.0.1");
        int peerPort = config.GetInt(prefix + "port", 0);

        if (peerId > 0 && peerPort > 0) {
            server.AddPeerConfig(peerId, peerHost, peerPort);
            DistributedChat::Logger::Info("Added peer server config - ID: " + std::to_string(peerId) + 
                                          " Address: " + peerHost + ":" + std::to_string(peerPort), "MAIN");
        }
    }

    if (!server.Start()) {
        DistributedChat::Logger::Error("Failed to start server engine.", "MAIN");
        return 1;
    }

    std::cout << "\n=======================================================\n";
    std::cout << " Server " << serverId << " is active. Type 'exit' to shut down gracefully.\n";
    std::cout << "=======================================================\n\n";

    std::string cmd;
    while (true) {
        std::getline(std::cin, cmd);
        cmd = DistributedChat::Trim(cmd);
        if (cmd == "exit" || cmd == "quit") {
            break;
        }
    }

    server.Stop();
    return 0;
}