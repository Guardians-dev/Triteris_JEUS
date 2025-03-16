#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "GameManager.hpp"

class NetworkManager {
private:
    int listenSocket;
    GameManager& gameManager;

public:
    NetworkManager(int port, GameManager& gm);
    ~NetworkManager();
    void acceptClient();
    void handleClientMessages(int playerId);
    void handleClientMessage(int playerId, const std::string& message);
    void broadcastGameState();
    void sendToPlayer(int playerId, const std::string& message);
}; 