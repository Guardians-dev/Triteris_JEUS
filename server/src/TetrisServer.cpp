#include "TetrisServer.hpp"
#include <iostream>

TetrisServer::TetrisServer(int port) : 
    gameManager(),
    networkManager(port, gameManager)
{
}

void TetrisServer::run() {
    try {
        std::cout << "테트리스 서버가 시작되었습니다." << std::endl;
        
        while (true) {
            networkManager.acceptClient();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "서버 오류: " << e.what() << std::endl;
    }
} 