#pragma once
#include <string>
#include "Event.hpp"
#include "SimpleMessagePack.hpp"

class NetworkManager {
private:
    int listenSocket;
    EventBus& eventBus;

    void handleClientMessages(int playerId, int clientSocket);
    void setupEventHandlers();

    std::string packMessage(const MessageData& message);
    MessageData unpackMessage(const std::string& data);

public:
    NetworkManager(int port, EventBus& bus);
    ~NetworkManager();
    void acceptClient();
    void broadcastGameState(const MessageData& gameState);
    void sendToPlayer(int playerId, const std::string& message);
}; 