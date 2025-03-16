#include "NetworkManager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <thread>

NetworkManager::NetworkManager(int port, GameManager& gm) : gameManager(gm) {
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        throw std::runtime_error("소켓 생성 실패");
    }

    int opt = 1;
    if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("소켓 옵션 설정 실패");
    }

    sockaddr_in serverAddr{};  // zero initialization 추가
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(listenSocket);  // 실패 시 소켓 정리
        throw std::runtime_error("바인드 실패: " + std::string(strerror(errno)));  // 구체적인 오류 메시지
    }

    if (listen(listenSocket, SOMAXCONN) < 0) {
        throw std::runtime_error("리스닝 실패");
    }
}

NetworkManager::~NetworkManager() {
    close(listenSocket);
}

void NetworkManager::acceptClient() {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket < 0) {
        std::cout << "클라이언트 연결 수락 실패" << std::endl;
        return;
    }

    static int nextPlayerId = 1;
    int playerId = nextPlayerId++;

    gameManager.addPlayer(playerId, clientSocket);

    // 플레이어 ID 전송
    json response = {
        {"type", "connect_response"},
        {"player_id", playerId}
    };
    sendToPlayer(playerId, response.dump());
    
    // 게임 상태 브로드캐스트
    broadcastGameState();
    
    // 클라이언트 메시지 처리 스레드 시작
    std::thread([this, playerId]() {
        this->handleClientMessages(playerId);
    }).detach();
}

void NetworkManager::handleClientMessages(int playerId) {
    char buffer[4096];
    auto& player = gameManager.getPlayer(playerId);

    while (true) {
        int bytesRead = recv(player.socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            std::cout << "플레이어 " << playerId << " 연결 종료" << std::endl;
            gameManager.removePlayer(playerId);
            broadcastGameState();
            break;
        }

        buffer[bytesRead] = '\0';
        handleClientMessage(playerId, std::string(buffer));
    }
}

void NetworkManager::handleClientMessage(int playerId, const std::string& message) {
    try {
        json msg = json::parse(message);
        std::string msgType = msg["type"];

        if (msgType == "request_new_piece") {
            gameManager.handleNewPiece(playerId);
        }
        else if (msgType == "move_request") {
            gameManager.handleMove(playerId, msg["direction"]);
        }
        else if (msgType == "rotate_request") {
            gameManager.handleRotate(playerId);
        }
        else if (msgType == "hard_drop_request") {
            gameManager.handleHardDrop(playerId);
        }
        else if (msgType == "move_down_request") {
            gameManager.handleMoveDown(playerId);
        }

        broadcastGameState();
    }
    catch (const json::parse_error& e) {
        std::cout << "JSON 파싱 오류: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "메시지 처리 오류: " << e.what() << std::endl;
    }
}

void NetworkManager::broadcastGameState() {
    json gameState = gameManager.getGameState();
    std::string stateMsg = gameState.dump() + "\n";
    
    const auto& players = gameManager.getPlayers();  // const 참조로 받기
    for (const auto& [id, player] : players) {
        send(player.socket, stateMsg.c_str(), stateMsg.length(), 0);
    }
}

void NetworkManager::sendToPlayer(int playerId, const std::string& message) {
    auto& player = gameManager.getPlayer(playerId);
    std::string msg = message + "\n";
    send(player.socket, msg.c_str(), msg.length(), 0);
} 