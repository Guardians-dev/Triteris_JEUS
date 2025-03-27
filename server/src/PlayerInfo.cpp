#include "PlayerInfo.hpp"
#include <iostream>

PlayerInfo::PlayerInfo(EventBus& bus) : 
    eventBus(bus),
    socket(-1),
    score(0),
    isReady(false),
    board(20, std::vector<int>(10, 0)),
    currentPos({0, 5}),
    currentBlockType(0) {
    
    setupEventHandlers();
}

void PlayerInfo::setupEventHandlers() {
    // 플레이어 정보 요청 이벤트 구독
    eventBus.subscribe("request_player_info", [this](const Event& event) {
        std::string action = event.data["action"].stringValue;
        
        if (action == "get_player_socket") {
            int playerId = event.data["player_id"].intValue;
            if (hasPlayer(playerId)) {
                MessageData response;
                response["action"] = "player_socket_info";
                response["player_id"] = playerId;
                response["socket"] = getPlayerSocket(playerId);
                
                if (event.data.contains("message")) {
                    response["message"] = event.data["message"].stringValue;
                }
                
                eventBus.publish("player_info_response", response);
            }
        }
        else if (action == "get_all_players") {
            MessageData response;
            response["action"] = "all_players_info";
            response["players"] = getAllPlayers();
            
            if (event.data.contains("game_state")) {
                response["game_state"] = event.data["game_state"].stringValue;
            }
            
            eventBus.publish("player_info_response", response);
        }
    });
    
    // 클라이언트 연결 종료 이벤트 구독
    eventBus.subscribe("client_disconnected", [this](const Event& event) {
        int playerId = event.data["player_id"].intValue;
        removePlayer(playerId);
    });
}

void PlayerInfo::addPlayer(int playerId, int socket) {
    std::cout << "PlayerInfo: 플레이어 추가 중 (ID: " << playerId << ")" << std::endl;
    
    players[playerId] = {
        {"socket", socket},
        {"nickname", ""}
    };
    
    // 플레이어 추가 이벤트 발행
    eventBus.publish("player_added", {
        {"player_id", playerId}
    });
    
    std::cout << "PlayerInfo: 플레이어 추가 이벤트 발행 완료 (ID: " << playerId << ")" << std::endl;
}

void PlayerInfo::removePlayer(int playerId) {
    if (hasPlayer(playerId)) {
        players.erase(playerId);
        std::cout << "PlayerInfo: 플레이어 제거됨 (ID: " << playerId << ")" << std::endl;
    }
}

bool PlayerInfo::hasPlayer(int playerId) const {
    return players.find(playerId) != players.end();
}

int PlayerInfo::getPlayerSocket(int playerId) const {
    return players.at(playerId)["socket"].intValue;
}

void PlayerInfo::setPlayerNickname(int playerId, const std::string& nickname) {
    if (hasPlayer(playerId)) {
        players[playerId]["nickname"] = nickname;
    }
}

MessageData PlayerInfo::getAllPlayers() const {
    MessageData result;
    for (const auto& [id, playerData] : players) {
        result[std::to_string(id)] = playerData;
    }
    return result;
} 