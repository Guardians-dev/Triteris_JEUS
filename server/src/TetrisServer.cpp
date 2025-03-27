#include "TetrisServer.hpp"
#include <iostream>

TetrisServer::TetrisServer(int port) : 
    eventBus(GlobalEventBus::getInstance()),
    playerInfo(eventBus),
    gameManager(eventBus),
    networkManager(port, eventBus)
{
    setupEventHandlers();
}

void TetrisServer::setupEventHandlers() {
    // 클라이언트 연결 이벤트 구독
    eventBus.subscribe("client_connected", [this](const Event& event) {
        this->handleClientConnected(event);
    });
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

void TetrisServer::handleClientConnected(const Event& event) {
    int playerId = event.data["player_id"].intValue;
    int socket = event.data["socket"].intValue;
    
    std::cout << "TetrisServer: 클라이언트 연결 처리 중 (ID: " << playerId << ")" << std::endl;
    
    // PlayerInfo에 플레이어 추가
    playerInfo.addPlayer(playerId, socket);
    
    // GameManager에 플레이어 추가
    gameManager.addPlayer(playerId, socket);
    
    std::cout << "TetrisServer: 플레이어 추가 완료 (ID: " << playerId << ")" << std::endl;
} 