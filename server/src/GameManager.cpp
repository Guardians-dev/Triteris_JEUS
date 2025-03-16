#include "GameManager.hpp"
#include <random>
#include <chrono>

GameManager::GameManager() : gameStarted(false) {
}

void GameManager::addPlayer(int playerId, int socket) {
    PlayerInfo newPlayer;
    newPlayer.socket = socket;
    newPlayer.score = 0;
    newPlayer.isReady = false;
    
    auto [piece, blockType] = generateNewPiece();
    newPlayer.currentPiece = {
        {"shape", piece},
        {"block_type", blockType}
    };
    newPlayer.currentBlockType = blockType;
    newPlayer.currentPos = {0, 5};
    
    players[playerId] = newPlayer;
}

void GameManager::removePlayer(int playerId) {
    players.erase(playerId);
}

void GameManager::handleNewPiece(int playerId) {
    auto& player = players[playerId];
    auto [piece, blockType] = generateNewPiece();
    player.currentPiece = {
        {"shape", piece},
        {"block_type", blockType}
    };
    player.currentBlockType = blockType;
    
    // 블록의 시작 위치를 중앙으로 수정
    int startX = (GRID_WIDTH / 2) - (piece[0].size() / 2);  // 블록 너비의 절반을 고려하여 중앙 정렬
    player.currentPos = {0, startX};  // y는 0, x는 중앙
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
}

void GameManager::handleRotate(int playerId) {
    auto& player = players[playerId];
    json rotatedShape = rotatePiece(player.currentPiece["shape"]);
    
    json rotatedPiece = {
        {"shape", rotatedShape},
        {"block_type", player.currentBlockType}
    };
    
    if (isValidMove(player.board, rotatedPiece, player.currentPos)) {
        player.currentPiece = rotatedPiece;
    }
}

bool GameManager::isValidMove(const vector<vector<int>>& board, const json& piece, const vector<int>& pos) {
    const json& shape = piece["shape"];
    for (size_t y = 0; y < shape.size(); y++) {
        for (size_t x = 0; x < shape[y].size(); x++) {
            if (shape[y][x] == 1) {
                int newY = pos[0] + y;
                int newX = pos[1] + x;
                
                if (newY >= GRID_HEIGHT || newX < 0 || newX >= GRID_WIDTH ||
                    (newY >= 0 && board[newY][newX] != 0)) {
                    return false;
                }
            }
        }
    }
    return true;
}

pair<json, int> GameManager::generateNewPiece() {
    vector<vector<vector<int>>> shapes = {
        {{1, 1, 1, 1}},  // I
        {{1, 1}, {1, 1}},  // O
        {{1, 1, 1}, {0, 1, 0}},  // T
        {{1, 1, 1}, {1, 0, 0}},  // L
        {{1, 1, 1}, {0, 0, 1}},  // J
        {{1, 1, 0}, {0, 1, 1}},  // S
        {{0, 1, 1}, {1, 1, 0}}   // Z
    };
    
    static auto seed = chrono::system_clock::now().time_since_epoch().count();
    static mt19937 gen(seed);
    uniform_int_distribution<> dis(0, shapes.size() - 1);
    
    int randomIndex = dis(gen);
    return {json(shapes[randomIndex]), randomIndex + 1};
}

json GameManager::rotatePiece(const json& piece) {
    size_t rows = piece.size();
    size_t cols = piece[0].size();
    
    json rotated = json::array();
    for (size_t i = 0; i < cols; i++) {
        json newRow = json::array();
        for (size_t j = 0; j < rows; j++) {
            newRow.push_back(piece[rows - 1 - j][i]);
        }
        rotated.push_back(newRow);
    }
    
    return rotated;
}

json GameManager::getGameState() const {
    json gameState = {{"type", "game_state"}, {"players", json::object()}};
    
    for (const auto& [id, player] : players) {
        json currentPiece = {
            {"shape", player.currentPiece["shape"]},
            {"block_type", player.currentBlockType}
        };
        
        gameState["players"][to_string(id)] = {
            {"score", player.score},
            {"board", player.board},
            {"nickname", player.nickname},
            {"current_piece", currentPiece},
            {"position", player.currentPos}
        };
    }
    
    return gameState;
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
    const json& shape = player.currentPiece["shape"];
    
    for (size_t y = 0; y < shape.size(); y++) {
        for (size_t x = 0; x < shape[y].size(); x++) {
            if (shape[y][x] == 1) {
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