#include "../include/client/ChatClient.h"
#include <iostream>

void PrintWelcomeArt() {
    std::cout << "\033[96m";
    std::cout << "  _____  _     _  _______  ______  _____  ______  _     _  _______ _______ ______  \n";
    std::cout << "  |     \\ |     |  |       |_____/ |_____] |_____] |     |     |    |_____/ |     \\ \n";
    std::cout << "  |_____/ |_____|  |_____  |    \\_ |       |_____] |_____u     |    |    \\_ |_____/ \n";
    std::cout << "                                                                                   \n";
    std::cout << "          -- ENTERPRISE CLUSTER-ROUTED CHAT SYSTEM CLIENT --\n";
    std::cout << "\033[0m\n";
}

void PrintHelp() {
    std::cout << "\033[93m";
    std::cout << "Available Commands:\n";
    std::cout << "  /msg <username> <text>      - Send a private message to any user in the cluster\n";
    std::cout << "  /create <room>              - Register a new chat room\n";
    std::cout << "  /join <room>                - Join a chat room\n";
    std::cout << "  /roommsg <room> <text>      - Send a message to all users in a chat room\n";
    std::cout << "  /leave <room>               - Exit a chat room\n";
    std::cout << "  /broadcast <text>           - Broadcast a global message to all servers\n";
    std::cout << "  /help                       - Show this guide\n";
    std::cout << "  /logout                     - Disconnect from the server\n";
    std::cout << "  /exit                       - Close the application\n";
    std::cout << "\033[0m\n";
}

int main() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }

    PrintWelcomeArt();

    std::string host;
    std::cout << "Enter Server Host (default: 127.0.0.1): ";
    std::getline(std::cin, host);
    if (DistributedChat::Trim(host).empty()) {
        host = "127.0.0.1";
    }

    std::string portStr;
    std::cout << "Enter Server Port (default: 9001): ";
    std::getline(std::cin, portStr);
    int port = 9001;
    if (!DistributedChat::Trim(portStr).empty()) {
        try {
            port = std::stoi(portStr);
        } catch (...) {
            std::cout << "Invalid port, using 9001\n";
        }
    }

    DistributedChat::ChatClient client;
    std::cout << "\033[94mConnecting to server at " << host << ":" << port << "...\033[0m\n";
    
    if (!client.Connect(host, port)) {
        std::cout << "\033[91mFailed to connect to the server. Ensure it is online and running.\033[0m\n";
        return 1;
    }

    std::cout << "\033[92mConnected!\033[0m\n\n";

    std::string username;
    while (true) {
        std::cout << "Enter username: ";
        std::getline(std::cin, username);
        username = DistributedChat::Trim(username);
        if (!username.empty()) {
            break;
        }
    }

    if (!client.Login(username)) {
        std::cout << "\033[91mFailed to send login packet.\033[0m\n";
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (!client.IsConnected()) {
        std::cout << "\033[91mAuthentication unsuccessful or disconnected.\033[0m\n";
        return 1;
    }

    PrintHelp();

    std::string input;
    while (client.IsConnected()) {
        std::cout << ">>> " << std::flush;
        std::getline(std::cin, input);
        input = DistributedChat::Trim(input);

        if (input.empty()) continue;

        if (input[0] == '/') {
            size_t spacePos = input.find(' ');
            std::string cmd = input.substr(0, spacePos);
            std::string args = (spacePos != std::string::npos) ? input.substr(spacePos + 1) : "";
            args = DistributedChat::Trim(args);

            if (cmd == "/exit") {
                break;
            } else if (cmd == "/logout") {
                client.Logout();
                std::cout << "\033[93mLogged out successfully.\033[0m\n";
                break;
            } else if (cmd == "/help") {
                PrintHelp();
            } else if (cmd == "/msg") {
                size_t argSpace = args.find(' ');
                if (argSpace == std::string::npos || args.substr(0, argSpace).empty()) {
                    std::cout << "\033[91mUsage: /msg <username> <text>\033[0m\n";
                } else {
                    std::string rcvr = args.substr(0, argSpace);
                    std::string txt = args.substr(argSpace + 1);
                    client.SendPrivateMessage(rcvr, txt);
                }
            } else if (cmd == "/create") {
                if (args.empty()) {
                    std::cout << "\033[91mUsage: /create <room>\033[0m\n";
                } else {
                    client.CreateRoom(args);
                }
            } else if (cmd == "/join") {
                if (args.empty()) {
                    std::cout << "\033[91mUsage: /join <room>\033[0m\n";
                } else {
                    client.JoinRoom(args);
                }
            } else if (cmd == "/leave") {
                if (args.empty()) {
                    std::cout << "\033[91mUsage: /leave <room>\033[0m\n";
                } else {
                    client.LeaveRoom(args);
                }
            } else if (cmd == "/roommsg") {
                size_t argSpace = args.find(' ');
                if (argSpace == std::string::npos || args.substr(0, argSpace).empty()) {
                    std::cout << "\033[91mUsage: /roommsg <room> <text>\033[0m\n";
                } else {
                    std::string rm = args.substr(0, argSpace);
                    std::string txt = args.substr(argSpace + 1);
                    client.SendRoomMessage(rm, txt);
                }
            } else if (cmd == "/broadcast") {
                if (args.empty()) {
                    std::cout << "\033[91mUsage: /broadcast <text>\033[0m\n";
                } else {
                    client.SendBroadcastMessage(args);
                }
            } else {
                std::cout << "\033[91mUnknown command: " << cmd << ". Type /help to see guidelines.\033[0m\n";
            }
        } else {
            client.SendBroadcastMessage(input);
        }
    }

    client.Disconnect();
    std::cout << "\033[90mApplication closed.\033[0m\n";
    return 0;
}