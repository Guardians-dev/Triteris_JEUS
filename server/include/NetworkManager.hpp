#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "Event.hpp"

class NetworkManager {
private:
    int listenSocket;
    EventBus& eventBus;

public:
    NetworkManager(int port, EventBus& bus);
    ~NetworkManager();
    void setupEventHandlers();
    void acceptClient();
    void handleClientMessages(int playerId, int clientSocket);
    void broadcastGameState(const json& gameState);
    void sendToPlayer(int playerId, const std::string& message);
}; 