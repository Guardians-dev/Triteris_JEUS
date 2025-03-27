#include "NetworkManager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <string.h> // strerror 사용을 위해 추가

NetworkManager::NetworkManager(int port, EventBus& bus) : eventBus(bus) {
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
    
    setupEventHandlers();
}

void NetworkManager::setupEventHandlers() {
    // 플레이어 추가 완료 이벤트 구독
    eventBus.subscribe("player_added", [this](const Event& event) {
        int playerId = event.data["player_id"];
        json response = {
            {"type", "connect_response"},
            {"player_id", playerId}
        };
        sendToPlayer(playerId, response.dump());
    });
    
    // 게임 상태 업데이트 이벤트 구독
    eventBus.subscribe("game_state_updated", [this](const Event& event) {
        broadcastGameState(event.data);
    });
    
    // 플레이어 정보 응답 이벤트 구독 (소켓 정보)
    eventBus.subscribe("player_info_response", [this](const Event& event) {
        std::string action = event.data["action"];
        
        if (action == "player_socket_info") {
            int playerId = event.data["player_id"];
            int socket = event.data["socket"];
            
            if (event.data.contains("message")) {
                std::string message = event.data["message"];
                std::string msg = message + "\n";
                send(socket, msg.c_str(), msg.length(), 0);
            }
        }
        else if (action == "all_players_info") {
            if (event.data.contains("game_state") && event.data.contains("players")) {
                json gameState = event.data["game_state"];
                std::string stateMsg = gameState.dump() + "\n";
                
                json players = event.data["players"];
                for (auto& [id, player] : players.items()) {
                    int socket = player["socket"];
                    send(socket, stateMsg.c_str(), stateMsg.length(), 0);
                }
            }
        }
    });
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

    // 클라이언트 연결 이벤트 발행
    eventBus.publish("client_connected", {
        {"player_id", playerId},
        {"socket", clientSocket}
    });
    
    // 클라이언트 메시지 처리 스레드 시작
    std::thread([this, playerId, clientSocket]() {
        this->handleClientMessages(playerId, clientSocket);
    }).detach();
}

void NetworkManager::handleClientMessages(int playerId, int clientSocket) {
    char buffer[4096];

    while (true) {
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            std::cout << "플레이어 " << playerId << " 연결 종료" << std::endl;
            // 클라이언트 연결 종료 이벤트 발행
            eventBus.publish("client_disconnected", {{"player_id", playerId}});
            break;
        }

        buffer[bytesRead] = '\0';
        
        // 클라이언트 메시지 수신 이벤트 발행
        try {
            json msg = json::parse(std::string(buffer));
            msg["player_id"] = playerId;
            eventBus.publish("client_message_received", msg);
        }
        catch (const json::parse_error& e) {
            std::cout << "JSON 파싱 오류: " << e.what() << std::endl;
        }
    }
}

void NetworkManager::broadcastGameState(const json& gameState) {
    std::string stateMsg = gameState.dump() + "\n";
    
    // 플레이어 정보 요청 이벤트 발행
    eventBus.publish("request_player_info", {
        {"action", "get_all_players"},
        {"game_state", gameState}
    });
}

void NetworkManager::sendToPlayer(int playerId, const std::string& message) {
    // 플레이어 소켓 요청 이벤트 발행
    eventBus.publish("request_player_info", {
        {"action", "get_player_socket"},
        {"player_id", playerId},
        {"message", message}
    });
} 