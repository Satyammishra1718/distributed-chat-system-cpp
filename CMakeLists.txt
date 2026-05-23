cmake_minimum_required(VERSION 3.15)

project(DistributedChatSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
if(MSVC)
    add_compile_options(/W4 /EHsc)
else()
    add_compile_options(-Wall -Wextra)
endif()

# Core sources
set(CORE_SOURCES
    src/core/networking/TcpSocket.cpp
    src/core/networking/SocketUtils.cpp
    src/core/threading/ThreadPool.cpp
    src/core/logging/Logger.cpp
    src/core/config/ConfigManager.cpp
)

# Protocol sources
set(PROTOCOL_SOURCES
    src/protocol/Packet.cpp
    src/protocol/MessageSerializer.cpp
)

# Server sources
set(SERVER_SOURCES
    src/server/ServerNode.cpp
    src/server/ClientSession.cpp
    src/server/ConnectionManager.cpp
    src/server/UserManager.cpp
    src/server/ChatRoomManager.cpp
    src/server/ReplicationManager.cpp
    src/server/HeartbeatManager.cpp
    src/server/ChatServer.cpp
)

# Client sources
set(CLIENT_SOURCES
    src/client/ChatClient.cpp
)

include_directories(include)

# Main Server
add_executable(main_server
    src/main_server.cpp
    ${CORE_SOURCES}
    ${PROTOCOL_SOURCES}
    ${SERVER_SOURCES}
)

# Main Client
add_executable(main_client
    src/main_client.cpp
    ${CORE_SOURCES}
    ${PROTOCOL_SOURCES}
    ${CLIENT_SOURCES}
)

# Integration Test
add_executable(integration_test
    tests/integration/DistributedChatTest.cpp
    ${CORE_SOURCES}
    ${PROTOCOL_SOURCES}
    ${SERVER_SOURCES}
    ${CLIENT_SOURCES}
)

# Unit Tests
add_executable(packet_test
    tests/unit/PacketTest.cpp
)

add_executable(threadpool_test
    tests/unit/ThreadPoolTest.cpp
    ${CORE_SOURCES}
)

add_executable(serializer_test
    tests/unit/SerializerTest.cpp
    ${CORE_SOURCES}
    ${PROTOCOL_SOURCES}
)

# Windows socket libraries
if(WIN32)
    target_link_libraries(main_server ws2_32)
    target_link_libraries(main_client ws2_32)
    target_link_libraries(integration_test ws2_32)
    target_link_libraries(threadpool_test ws2_32)
    target_link_libraries(serializer_test ws2_32)
endif()