#pragma once
#include "GameManager.hpp"
#include "NetworkManager.hpp"

class TetrisServer {
private:
    GameManager gameManager;
    NetworkManager networkManager;

public:
    explicit TetrisServer(int port);  // explicit 추가
    void run();
}; 