#pragma once
#include "GameManager.hpp"
#include "NetworkManager.hpp"
#include "Event.hpp"
#include "PlayerInfo.hpp"

class TetrisServer {
private:
    EventBus& eventBus;
    PlayerInfo playerInfo;
    GameManager gameManager;
    NetworkManager networkManager;
    
    void setupEventHandlers();

public:
    TetrisServer(int port);
    void run();
    void handleClientConnected(const Event& event);
}; 