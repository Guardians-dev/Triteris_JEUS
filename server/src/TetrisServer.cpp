#include "TetrisServer.hpp"
#include <iostream>

TetrisServer::TetrisServer(int port) : 
    eventBus(GlobalEventBus::getInstance()),
    gameManager(eventBus),
    networkManager(port, eventBus)
{
}

void TetrisServer::run() {
    try {
        std::cout << "테트리스 서버가 시작되었습니다." << std::endl;
        
        // 서버 시작 이벤트 발행
        eventBus.publish("server_started");
        
        while (true) {
            networkManager.acceptClient();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "서버 오류: " << e.what() << std::endl;
        // 서버 오류 이벤트 발행
        eventBus.publish("server_error", {{"error", e.what()}});
    }
} 