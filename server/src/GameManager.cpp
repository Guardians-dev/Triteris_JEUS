#include "GameManager.hpp"
#include <iostream>
#include <algorithm>
#include <random>

GameManager::GameManager(EventBus& bus) : 
    gameStarted(false),
    eventBus(bus) {
    setupEventHandlers();
}

void GameManager::setupEventHandlers() {
    // 클라이언트 연결 이벤트 구독
    eventBus.subscribe("client_connected", [this](const Event& event) {
        int playerId = event.data["player_id"].intValue;
        int socket = event.data["socket"].intValue;
        
        this->addPlayer(playerId, socket);
    });
    
    // 클라이언트 연결 종료 이벤트 구독
    eventBus.subscribe("client_disconnected", [this](const Event& event) {
        int playerId = event.data["player_id"].intValue;
        this->removePlayer(playerId);
    });
    
    // 클라이언트 메시지 수신 이벤트 구독
    eventBus.subscribe("client_message_received", [this](const Event& event) {
        std::string type = event.data["type"].stringValue;
        int playerId = event.data["player_id"].intValue;
        
        if (type == "move_left") {
            handleMove(playerId, -1);
        }
        else if (type == "move_right") {
            handleMove(playerId, 1);
        }
        else if (type == "rotate") {
            handleRotate(playerId);
        }
        else if (type == "move_down") {
            handleMoveDown(playerId);
        }
        else if (type == "hard_drop") {
            handleHardDrop(playerId);
        }
        else {
            std::cout << "알 수 없는 메시지 타입: " << type << std::endl;
        }
    });
    
    // 게임 상태 요청 이벤트 구독
    eventBus.subscribe("request_game_state", [this](const Event& event) {
        MessageData gameState = this->getGameState();
        eventBus.publish("game_state_updated", gameState);
    });
    
    // 플레이어 정보 요청 이벤트 구독
    eventBus.subscribe("request_player_info", [this](const Event& event) {
        std::string action = event.data["action"].stringValue;
        
        if (action == "get_all_players") {
            MessageData playerData;
            for (const auto& [id, player] : players) {
                MessageData playerInfo;
                playerInfo["socket"] = player.getPlayerSocket(id);
                playerData[std::to_string(id)] = playerInfo;
            }
            
            MessageData response;
            response["action"] = "all_players_info";
            response["players"] = playerData;
            
            if (event.data.contains("game_state")) {
                response["game_state"] = event.data["game_state"];
            }
            
            eventBus.publish("player_info_response", response);
        }
        else if (action == "get_player_socket") {
            int playerId = event.data["player_id"].intValue;
            if (players.find(playerId) != players.end()) {
                MessageData response;
                response["action"] = "player_socket_info";
                response["player_id"] = playerId;
                response["socket"] = players[playerId].getPlayerSocket(playerId);
                
                if (event.data.contains("message")) {
                    response["message"] = event.data["message"];
                }
                
                eventBus.publish("player_info_response", response);
            }
        }
    });
}

void GameManager::addPlayer(int playerId, int socket) {
    // PlayerInfo 객체 생성 및 초기화
    players.emplace(std::piecewise_construct,
                   std::forward_as_tuple(playerId),
                   std::forward_as_tuple(eventBus));
    
    // 플레이어 정보 설정
    players[playerId].addPlayer(playerId, socket);
    
    // 새 조각 생성
    auto [piece, blockType] = generateNewPiece();
    
    // 플레이어 추가 완료 이벤트 발행
    MessageData playerAddedData;
    playerAddedData["player_id"] = playerId;
    eventBus.publish("player_added", playerAddedData);
}

void GameManager::removePlayer(int playerId) {
    players.erase(playerId);
    
    // 플레이어 제거 완료 이벤트 발행
    MessageData playerRemovedData;
    playerRemovedData["player_id"] = playerId;
    eventBus.publish("player_removed", playerRemovedData);
}

void GameManager::handleNewPiece(int playerId) {
    auto& player = players[playerId];
    auto [piece, blockType] = generateNewPiece();
    player.currentPiece = MessageData();  // 빈 객체로 초기화
    player.currentPiece["shape"] = piece;
    player.currentPiece["block_type"] = blockType;
    player.currentBlockType = blockType;
    
    int startX = (GRID_WIDTH / 2) - (piece["shape"][0].size() / 2);
    player.currentPos = {0, startX};
    
    // 게임 오버 체크 추가
    if (!isValidMove(player.board, player.currentPiece, player.currentPos)) {
        // 게임 오버 이벤트 발생
        MessageData gameOverData;
        gameOverData["player_id"] = playerId;
        gameOverData["score"] = player.score;
        eventBus.publish("game_over", gameOverData);
    }

    // 게임 상태 변경 시 이벤트 발행
    MessageData gameStateData;
    gameStateData["board"] = player.board;
    gameStateData["score"] = player.score;
    gameStateData["current_piece"] = player.currentPiece;
    gameStateData["player_id"] = playerId;
    eventBus.publish("game_state_changed", gameStateData);
}

void GameManager::handleMove(int playerId, int direction) {
    auto& player = players[playerId];
    vector<int> newPos = {
        player.currentPos[0],
        player.currentPos[1] + direction
    };
    
    if (isValidMove(player.board, player.currentPiece, newPos)) {
        player.currentPos = newPos;
    }

    // 게임 상태 변경 시 이벤트 발행
    MessageData gameStateData;
    gameStateData["board"] = player.board;
    gameStateData["score"] = player.score;
    gameStateData["current_piece"] = player.currentPiece;
    gameStateData["player_id"] = playerId;
    eventBus.publish("game_state_changed", gameStateData);
}

void GameManager::handleRotate(int playerId) {
    auto& player = players[playerId];
    MessageData rotatedShape = rotatePiece(player.currentPiece["shape"]);
    
    MessageData rotatedPiece;
    rotatedPiece["shape"] = rotatedShape;
    rotatedPiece["block_type"] = player.currentBlockType;
    
    if (isValidMove(player.board, rotatedPiece, player.currentPos)) {
        player.currentPiece = rotatedPiece;
    }

    // 게임 상태 변경 시 이벤트 발행
    MessageData gameStateData;
    gameStateData["board"] = player.board;
    gameStateData["score"] = player.score;
    gameStateData["current_piece"] = player.currentPiece;
    gameStateData["player_id"] = playerId;
    eventBus.publish("game_state_changed", gameStateData);
}

bool GameManager::isValidMove(const vector<vector<int>>& board, const MessageData& piece, const vector<int>& pos) {
    const MessageData& shape = piece["shape"];
    for (size_t y = 0; y < shape.arrayValue.size(); y++) {
        for (size_t x = 0; x < shape.arrayValue[y].arrayValue.size(); x++) {
            if (shape.arrayValue[y].arrayValue[x].intValue == 1) {
                int newY = pos[0] + y;
                int newX = pos[1] + x;
                
                // 음수 y 좌표도 체크에 포함
                if (newY < 0 || newY >= GRID_HEIGHT || newX < 0 || newX >= GRID_WIDTH ||
                    (newY >= 0 && board[newY][newX] != 0)) {
                    return false;
                }
            }
        }
    }
    return true;
}

std::pair<MessageData, int> GameManager::generateNewPiece() {
    static const std::vector<std::vector<std::vector<int>>> PIECES = {
        // I
        {
            {1, 1, 1, 1}
        },
        // J
        {
            {1, 0, 0},
            {1, 1, 1}
        },
        // L
        {
            {0, 0, 1},
            {1, 1, 1}
        },
        // O
        {
            {1, 1},
            {1, 1}
        },
        // S
        {
            {0, 1, 1},
            {1, 1, 0}
        },
        // T
        {
            {0, 1, 0},
            {1, 1, 1}
        },
        // Z
        {
            {1, 1, 0},
            {0, 1, 1}
        }
    };
    
    // 랜덤 조각 선택
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, PIECES.size() - 1);
    int blockType = dis(gen);
    
    // MessageData 객체 생성
    MessageData piece;
    piece["shape"] = PIECES[blockType];
    
    return {piece, blockType};
}

MessageData GameManager::rotatePiece(const MessageData& piece) {
    const MessageData& shape = piece["shape"];
    int n = shape.size();
    int m = shape[0].size();
    
    MessageData rotated = MessageData::array();
    // 90도 시계방향 회전
    for (int i = 0; i < m; i++) {
        MessageData row = MessageData::array();
        for (int j = n - 1; j >= 0; j--) {
            row.push_back(shape[j][i].intValue);
        }
        rotated.push_back(row);
    }
    return rotated;
}

MessageData GameManager::getGameState() const {
    MessageData state;
    state["players"] = MessageData();
    
    for (const auto& [id, player] : players) {
        MessageData playerData;
        // 플레이어 정보 설정
        // ...
        
        state["players"][std::to_string(id)] = playerData;
    }
    
    return state;
}

PlayerInfo& GameManager::getPlayer(int playerId) {
    return players.at(playerId);
}

void GameManager::handleHardDrop(int playerId) {
    auto& player = players[playerId];
    vector<int> newPos = player.currentPos;
    
    while (isValidMove(player.board, player.currentPiece, {newPos[0] + 1, newPos[1]})) {
        newPos[0]++;
    }
    
    player.currentPos = newPos;
    freezePiece(playerId);
    handleNewPiece(playerId);
}

void GameManager::freezePiece(int playerId) {
    auto& player = players[playerId];
    const MessageData& shape = player.currentPiece["shape"];
    
    for (size_t y = 0; y < shape.arrayValue.size(); y++) {
        for (size_t x = 0; x < shape.arrayValue[y].arrayValue.size(); x++) {
            if (shape.arrayValue[y].arrayValue[x].intValue == 1) {
                int boardY = player.currentPos[0] + y;
                int boardX = player.currentPos[1] + x;
                if (boardY >= 0 && boardY < GRID_HEIGHT && 
                    boardX >= 0 && boardX < GRID_WIDTH) {
                    player.board[boardY][boardX] = player.currentBlockType;
                }
            }
        }
    }
    checkLines(playerId);

    // 게임 상태 변경 시 이벤트 발행
    MessageData gameStateData;
    gameStateData["board"] = player.board;
    gameStateData["score"] = player.score;
    gameStateData["current_piece"] = player.currentPiece;
    gameStateData["player_id"] = playerId;
    eventBus.publish("game_state_changed", gameStateData);
}

void GameManager::checkLines(int playerId) {
    auto& player = players[playerId];
    vector<int> lines_to_clear;

    // 완성된 줄 찾기
    for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
        bool line_full = true;
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (player.board[y][x] == 0) {
                line_full = false;
                break;
            }
        }
        if (line_full) {
            lines_to_clear.push_back(y);
        }
    }

    // 줄 제거 및 점수 업데이트
    if (!lines_to_clear.empty()) {
        for (int line : lines_to_clear) {
            player.board.erase(player.board.begin() + line);
            player.board.insert(player.board.begin(), vector<int>(GRID_WIDTH, 0));
        }
        player.score += lines_to_clear.size() * 100;
    }

    // 게임 상태 변경 시 이벤트 발행
    MessageData gameStateData;
    gameStateData["board"] = player.board;
    gameStateData["score"] = player.score;
    gameStateData["current_piece"] = player.currentPiece;
    gameStateData["player_id"] = playerId;
    eventBus.publish("game_state_changed", gameStateData);
}

void GameManager::handleMoveDown(int playerId) {
    auto& player = players[playerId];
    vector<int> newPos = {
        player.currentPos[0] + 1,  // y 좌표 증가
        player.currentPos[1]       // x 좌표는 그대로
    };
    
    if (isValidMove(player.board, player.currentPiece, newPos)) {
        player.currentPos = newPos;
    } else {
        // 더 이상 내려갈 수 없으면 블록을 고정하고 새 블록 생성
        freezePiece(playerId);
        handleNewPiece(playerId);
    }
} 