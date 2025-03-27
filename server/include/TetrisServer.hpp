#pragma once
#include "GameManager.hpp"
#include "NetworkManager.hpp"
#include "Event.hpp"

class TetrisServer {
private:
    EventBus& eventBus;
    GameManager gameManager;
    NetworkManager networkManager;

public:
    explicit TetrisServer(int port);  // explicit 추가
    void run();
}; 